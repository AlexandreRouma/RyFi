#ifdef BUILD_BLADERF_SUPPORT
#include "bladerf.h"
#include "flog/flog.h"
#include <format>
#include <algorithm>
#include "flog/flog.h"

#define USB_BUFFER_SIZE     8192

namespace dev {
    BladeRFReceiver::BladeRFReceiver(BladeRFDriver* drv, bladerf* dev, int channel) {
        // Save parameters
        this->drv = drv;
        this->dev = dev;
        this->channel = channel;

        // Enable the AGC
        int err = bladerf_set_gain_mode(dev, BLADERF_CHANNEL_RX(channel), BLADERF_GAIN_DEFAULT);
        if (err) { throw std::runtime_error("Failed to enable the AGC"); }
    }

    BladeRFReceiver::~BladeRFReceiver() {
        // Stop the device
        stop();

        // Close the device
        close();
    }

    void BladeRFReceiver::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double BladeRFReceiver::getBestSamplerate(double min) {
        // Get the supported samplerate range
        const bladerf_range* range;
        int err = bladerf_get_sample_rate_range(dev, BLADERF_CHANNEL_RX(channel), &range);
        if (err) { throw std::runtime_error("Failed to query samplerate range"); }

        // TODO: Don't just assume that the step is 1
        return std::clamp<double>(min, range->min, range->max);
    }

    void BladeRFReceiver::setSamplerate(double samplerate) {
        // Forbid samplerate changes while running
        if (running) { throw std::runtime_error("Cannot change the samplerate while the device is running"); }

        // Set the samplerate
        bladerf_sample_rate actualSr;
        int err = bladerf_set_sample_rate(dev, BLADERF_CHANNEL_RX(channel), samplerate, &actualSr);
        if (err) { throw std::runtime_error("Failed to set the samplerate"); }

        // Set the bandwidth appropriately (TODO: Likely wrong)
        err = bladerf_set_bandwidth(dev, BLADERF_CHANNEL_RX(channel), samplerate, NULL);
        if (err) { throw std::runtime_error("Failed to set the bandwidth"); }

        // Check that the samplerate was selected properly
        if (actualSr != round(samplerate)) {
            throw std::runtime_error(std::format("The selected samplerate does not match the actual samplerate. Expected {} got {}", samplerate, actualSr)); 
        }

        // Update the buffer size
        bufferSize = samplerate / 200.0f;
    }

    void BladeRFReceiver::tune(double freq) {
        // Send the tune command
        bladerf_set_frequency(dev, BLADERF_CHANNEL_RX(channel), freq);
    }

    void BladeRFReceiver::start() {
        // If already running, do nothing
        if (running) { return; }

        // Configure the stream
        bladerf_sync_config(dev, BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 16, USB_BUFFER_SIZE, 8, 3500);

        // Start streaming
        bladerf_enable_module(dev, BLADERF_CHANNEL_RX(channel), true);

        // Start the worker
        workerThread = std::thread(&BladeRFReceiver::worker, this);

        // Mark as running
        running = true;
    }

    void BladeRFReceiver::stop() {
        // If not running, do nothing
        if (!running) { return; }

        // Stop the worker
        out.stopWriter();
        if (workerThread.joinable()) { workerThread.join(); }
        out.clearWriteStop();

        // Stop the stream
        bladerf_enable_module(dev, BLADERF_CHANNEL_RX(channel), false);

        // Mark as not running
        running = false;
    }

    void BladeRFReceiver::worker() {
        // Allocate the sample buffers
        int16_t* samps = dsp::buffer::alloc<int16_t>(bufferSize*2);

        while (true) {
            // Receive the samples to the device
            bladerf_sync_rx(dev, samps, bufferSize, NULL, 3500);

            // Convert the samples to complex float
            volk_16i_s32f_convert_32f((float*)out.writeBuf, samps, 2048.0f, bufferSize*2);

            // Send off the samples
            if (!out.swap(bufferSize)) { break; }
        }

        // Free the sample buffer
        dsp::buffer::free(samps);
    }

    BladeRFTransmitter::BladeRFTransmitter(dsp::stream<dsp::complex_t>* in, BladeRFDriver* drv, bladerf* dev, int channel) {
        // Save parameters
        this->in = in;
        this->drv = drv;
        this->dev = dev;
        this->channel = channel;

        // Set the gain
        const bladerf_range* gr;
        int err = bladerf_get_gain_range(dev, BLADERF_CHANNEL_TX(channel), &gr);
        if (err) { throw std::runtime_error("Failed to get TX gain range"); }
        err = bladerf_set_gain(dev, BLADERF_CHANNEL_TX(channel), gr->max);
        if (err) { throw std::runtime_error("Failed to set TX gain"); }
    }

    BladeRFTransmitter::~BladeRFTransmitter() {
        // Stop the device
        stop();

        // Close the device
        close();
    }

    void BladeRFTransmitter::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double BladeRFTransmitter::getBestSamplerate(double min) {
        // Get the supported samplerate range
        const bladerf_range* range;
        int err = bladerf_get_sample_rate_range(dev, BLADERF_CHANNEL_TX(channel), &range);
        if (err) { throw std::runtime_error("Failed to query samplerate range"); }

        // TODO: Don't just assume that the step is 1
        return std::clamp<double>(min, range->min, range->max);
    }

    void BladeRFTransmitter::setSamplerate(double samplerate) {
        // Forbid samplerate changes while running
        if (running) { throw std::runtime_error("Cannot change the samplerate while the device is running"); }

        // Set the samplerate
        bladerf_sample_rate actualSr;
        int err = bladerf_set_sample_rate(dev, BLADERF_CHANNEL_TX(channel), samplerate, &actualSr);
        if (err) { throw std::runtime_error("Failed to set the samplerate"); }

        // Set the bandwidth appropriately (TODO: Likely wrong)
        err = bladerf_set_bandwidth(dev, BLADERF_CHANNEL_TX(channel), samplerate, NULL);
        if (err) { throw std::runtime_error("Failed to set the bandwidth"); }

        // Check that the samplerate was selected properly
        if (actualSr != round(samplerate)) {
            throw std::runtime_error(std::format("The selected samplerate does not match the actual samplerate. Expected {} got {}", samplerate, actualSr)); 
        }

        // Update the buffer size
        bufferSize = samplerate / 200.0f;
    }

    void BladeRFTransmitter::tune(double freq) {
        // Send the tune command
        bladerf_set_frequency(dev, BLADERF_CHANNEL_TX(channel), freq);
    }

    void BladeRFTransmitter::start() {
        // If already running, do nothing
        if (running) { return; }

        // Configure the stream
        bladerf_sync_config(dev, BLADERF_TX_X1, BLADERF_FORMAT_SC16_Q11, 16, USB_BUFFER_SIZE, 8, 3500);

        // Start streaming
        bladerf_enable_module(dev, BLADERF_CHANNEL_TX(channel), true);

        // Start the worker
        workerThread = std::thread(&BladeRFTransmitter::worker, this);

        // Mark as running
        running = true;
    }

    void BladeRFTransmitter::stop() {
        // If not running, do nothing
        if (!running) { return; }

        // Stop the worker
        in->stopReader();
        if (workerThread.joinable()) { workerThread.join(); }
        in->clearReadStop();

        // Stop the stream
        bladerf_enable_module(dev, BLADERF_CHANNEL_TX(channel), false);

        // Mark as not running
        running = false;
    }

    void BladeRFTransmitter::worker() {
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

    void BladeRFDriver::registerSelf() {
        registerDriver(BLADERF_DRIVER_NAME, std::make_unique<BladeRFDriver>());
    }

    std::vector<Info> BladeRFDriver::list() {
        // If the cache is already populated, return it instead
        if (!devListCache.empty()) { return devListCache; }

        // Get the list of devices
        bladerf_devinfo* devList = NULL;
        int count = bladerf_get_device_list(&devList);
        if (count < 0 && count != BLADERF_ERR_NODEV) {
            flog::warn("Failed to list BladeRF devices");
            return devListCache;
        }

        // Go through each device
        for (int i = 0; i < count; i++) {
            // Get the serial number
            std::string serial = devList[i].serial;

            // Create the info struct
            Info info = { DEV_TYPE_RECEIVER | DEV_TYPE_TRANSMITTER, BLADERF_DRIVER_NAME, serial };

            // Save the info struct
            devListCache.push_back(info);
        }

        // Free the device list
        if (devList) { bladerf_free_device_list(devList); }

        // Return the list
        return devListCache;
    }

    std::shared_ptr<Receiver> BladeRFDriver::openRX(const std::string& identifier) {
        // Create the device from the new context
        return std::make_shared<BladeRFReceiver>(this, acquireContext(identifier), 0);
    }

    std::shared_ptr<Transmitter> BladeRFDriver::openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {
        // Create the device from the new context
        return std::make_shared<BladeRFTransmitter>(in, this, acquireContext(identifier), 0);
    }

    bladerf* BladeRFDriver::acquireContext(const std::string& identifier) {
        // Check the identifier
        if (identifier.size() != sizeof(bladerf_devinfo::serial) - 1) {
            throw std::runtime_error("Invalid device identifier");
        }

        // If the device is already open
        auto it = ctxs.find(identifier);
        if (it != ctxs.end()) {
            // Increment the refcount
            it->second.refCount++;

            // Return the existing context
            return it->second.dev;
        }

        // Create a devinfo corresponding to its serial number
        bladerf_devinfo info;
        bladerf_init_devinfo(&info);
        strcpy(info.serial, identifier.c_str());

        // Attempt to open the device
        bladerf* dev;
        int err = bladerf_open_with_devinfo(&dev, &info);
        if (err) { throw std::runtime_error("Failed to open BladeRF device"); }

        // Create and append a new context entry
        BladeRFContext ctx = { dev, 1 };
        ctxs[identifier] = ctx;

        // Return the new context
        return dev;
    }

    void BladeRFDriver::releaseContext(bladerf* dev) {
        // Find the context entry corresponding to it
        auto it = std::find_if(ctxs.begin(), ctxs.end(), [dev](const std::pair<std::string, BladeRFContext>& p) { return p.second.dev == dev; });
        if (it == ctxs.end()) { throw std::runtime_error("Tried to release a context that doesn't exist"); }

        // Decrement the refcount and do nothing if it's greater than 0
        if (--(it->second.refCount)) { return; }

        // Remove the context entry from the list
        ctxs.erase(it);

        // Close the device
        bladerf_close(dev);
    }
}
#endif