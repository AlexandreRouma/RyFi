#include "bladerf.h"
#include <stdexcept>
#include "flog/flog.h"

#define BUFFER_SIZE     8192*2

BladeRF::BladeRF(dsp::stream<dsp::complex_t>* in, double samplerate, double rxFrequency, double txFrequency) {
    // Save the configuration
    this->in = in;
    this->samplerate = samplerate;
    this->rxFrequency = rxFrequency;
    this->txFrequency = txFrequency;
}

BladeRF::~BladeRF() {
    // Stop the device
    stop();
}

void BladeRF::start() {
    // If already running, do nothing
    if (running) { return; }

    // Open device
    struct bladerf_devinfo dev_info;
    bladerf_init_devinfo(&dev_info);
    int err = bladerf_open_with_devinfo(&dev, &dev_info);
    if (err) {
        throw std::runtime_error("Failed to open device");
    }

    // Set RX parameters
    bladerf_sample_rate sr;
    err = bladerf_set_sample_rate(dev, BLADERF_CHANNEL_RX(0), samplerate, &sr);
    if (err || sr != samplerate) { throw std::runtime_error("Failed to set RX samplerate"); }
    err = bladerf_set_frequency(dev, BLADERF_CHANNEL_RX(0), rxFrequency);
    if (err) { throw std::runtime_error("Failed to set RX frequency"); }
    err = bladerf_set_bandwidth(dev, BLADERF_CHANNEL_RX(0), samplerate, NULL);
    if (err) { throw std::runtime_error("Failed to set RX bandwidth"); }
    err = bladerf_set_gain_mode(dev, BLADERF_CHANNEL_RX(0), bladerf_gain_mode::BLADERF_GAIN_DEFAULT);
    if (err) { throw std::runtime_error("Failed to set RX gain"); }

    // Set TX parameters
    err = bladerf_set_sample_rate(dev, BLADERF_CHANNEL_TX(0), samplerate, &sr);
    if (err || sr != samplerate) { throw std::runtime_error("Failed to set TX samplerate"); }
    err = bladerf_set_frequency(dev, BLADERF_CHANNEL_TX(0), txFrequency);
    if (err) { throw std::runtime_error("Failed to set TX frequency"); }
    err = bladerf_set_bandwidth(dev, BLADERF_CHANNEL_TX(0), samplerate, NULL);
    if (err) { throw std::runtime_error("Failed to set TX bandwidth"); }
    const bladerf_range* gr;
    err = bladerf_get_gain_range(dev, BLADERF_CHANNEL_TX(0), &gr);
    if (err) { throw std::runtime_error("Failed to get TX gain range"); }
    err = bladerf_set_gain(dev, BLADERF_CHANNEL_TX(0), gr->max);
    if (err) { throw std::runtime_error("Failed to set TX gain"); }

    // Start synchronous streaming
    bladerf_sync_config(dev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 16, BUFFER_SIZE, 8, 3500);
    bladerf_sync_config(dev, BLADERF_TX_X1, BLADERF_FORMAT_SC16_Q11, 16, BUFFER_SIZE, 8, 3500);
    bladerf_enable_module(dev, BLADERF_CHANNEL_RX(0), true);
    bladerf_enable_module(dev, BLADERF_CHANNEL_TX(0), true);

    // Start the workers
    rxWorkerThread = std::thread(&BladeRF::rxWorker, this);
    txWorkerThread = std::thread(&BladeRF::txWorker, this);

    // Mark as running
    running = true;
}

void BladeRF::stop() {
    // If not running, do nothing
    if (!running) { return; }

    // Stop the workers
    in->stopReader();
    out.stopWriter();
    if (rxWorkerThread.joinable()) { rxWorkerThread.join(); }
    if (txWorkerThread.joinable()) { txWorkerThread.join(); }
    in->clearReadStop();
    out.clearWriteStop();

    // Stop the device
    bladerf_enable_module(dev, BLADERF_CHANNEL_RX(0), false);
    bladerf_enable_module(dev, BLADERF_CHANNEL_TX(0), false);

    // Close the device
    bladerf_close(dev);

    // Mark as not running
    running = false;
}

void BladeRF::rxWorker() {
    int sampleCount = samplerate / 200;

    // Allocate the sample buffers
    int16_t* samps = dsp::buffer::alloc<int16_t>(sampleCount*2);

    while (true) {
        // Receive the samples to the device
        bladerf_sync_rx(dev, samps, sampleCount, NULL, 3500);

        // Convert the samples to complex float
        volk_16i_s32f_convert_32f((float*)out.writeBuf, samps, 2048.0f, sampleCount*2);

        // Send off the samples
        if (!out.swap(sampleCount)) { break; }
    }

    // Free the sample buffer
    dsp::buffer::free(samps);
}

void BladeRF::txWorker() {
    // Allocate the sample buffers
    int16_t* samps = dsp::buffer::alloc<int16_t>(STREAM_BUFFER_SIZE*2);

    while (true) {
        // Get the transmitter samples
        int count = in->read();
        if (count <= 0) { break; }

        // Convert the samples to 16bit PCM
        volk_32f_s32f_convert_16i(samps, (float*)in->readBuf, 2048.0f, count*2);

        // Flush the modulator stream
        in->flush();

        // Send the samples to the device
        bladerf_sync_tx(dev, samps, count, NULL, 3500);
    }

    // Free the sample buffer
    dsp::buffer::free(samps);
}