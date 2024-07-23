#include "limesdr.h"
#include <stdexcept>
#include "flog/flog.h"

LimeSDR::LimeSDR(dsp::stream<dsp::complex_t>* in, double samplerate, double rxFrequency, double txFrequency) {
    // Save the configuration
    this->in = in;
    this->samplerate = samplerate;
    this->rxFrequency = rxFrequency;
    this->txFrequency = txFrequency;
}

LimeSDR::~LimeSDR() {
    // Stop the device
    stop();
}

void LimeSDR::start() {
    // If already running, do nothing
    if (running) { return; }

    // Get available devices
    lms_info_str_t devList[16];
    int count = LMS_GetDeviceList(devList);
    if (count == 0) {
        throw std::runtime_error("No device found");
    }
    else if (count < 0) {
        throw std::runtime_error("Failed to list devices");
    }

    // Find the first device
    if (LMS_Open(&dev, devList[0], NULL)) {
        throw std::runtime_error("Failed to open device");
    }

    // Init the device
    if (LMS_Init(dev)) {
        throw std::runtime_error("Failed to initialize the device");
    }

    // List antennas
    lms_name_t rxAntennas[16];
    lms_name_t txAntennas[16];
    int rxAntCount = LMS_GetAntennaList(dev, false, 0, rxAntennas);
    int txAntCount = LMS_GetAntennaList(dev, true, 0, txAntennas);

    // Set the samplerate
    LMS_SetSampleRate(dev, samplerate, 0);

    // Set RX parameters
    LMS_EnableChannel(dev, false, 0, true);
    LMS_SetAntenna(dev, false, 0, rxAntCount-1); // Select the auto antenna
    LMS_SetLOFrequency(dev, false, 0, rxFrequency);
    LMS_SetGaindB(dev, false, 0, 30); // TODO: Adjust this some day
    LMS_SetLPFBW(dev, false, 0, samplerate);
    LMS_SetLPF(dev, false, 0, true);

    // Set TX parameters
    LMS_EnableChannel(dev, true, 0, true);
    LMS_SetAntenna(dev, true, 0, txAntCount-1); // Select the auto antenna
    LMS_SetLOFrequency(dev, true, 0, txFrequency);
    LMS_SetGaindB(dev, true, 0, 60); // TODO:Check that this is the max
    LMS_SetLPFBW(dev, true, 0, samplerate);
    LMS_SetLPF(dev, true, 0, true);

    // Calibrate
    LMS_Calibrate(dev, false, 0, samplerate, 0);
    LMS_Calibrate(dev, true, 0, samplerate, 0);

    // Setup RX stream
    rxStream.isTx = false;
    rxStream.channel = 0;
    rxStream.fifoSize =  1024*1024; // Whatever the fuck this means
    rxStream.throughputVsLatency = 0.5f;
    rxStream.dataFmt = rxStream.LMS_FMT_F32;
    LMS_SetupStream(dev, &rxStream);

    // Setup and start stream
    txStream.isTx = true;
    txStream.channel = 0;
    txStream.fifoSize =  1024*1024; // Whatever the fuck this means
    txStream.throughputVsLatency = 0.5f;
    txStream.dataFmt = txStream.LMS_FMT_F32;
    LMS_SetupStream(dev, &txStream);

    // Start the streams
    count = LMS_StartStream(&rxStream);
    if (count) { throw std::runtime_error("Failed to start RX stream"); }
    count = LMS_StartStream(&txStream);
    if (count) { throw std::runtime_error("Failed to start TX stream"); }

    // Start the workers
    rxWorkerThread = std::thread(&LimeSDR::rxWorker, this);
    txWorkerThread = std::thread(&LimeSDR::txWorker, this);

    // Set status
    running = true;
}

void LimeSDR::stop() {
    // If not running, do nothing
    if (!running) { return; }

    // Stop the workers
    in->stopReader();
    out.stopWriter();
    if (rxWorkerThread.joinable()) { rxWorkerThread.join(); }
    if (txWorkerThread.joinable()) { txWorkerThread.join(); }
    in->clearReadStop();
    out.clearWriteStop();

    // Stop the streams
    LMS_StopStream(&rxStream);
    LMS_StopStream(&txStream);

    // Disable the channels
    LMS_EnableChannel(dev, false, 0, false);
    LMS_EnableChannel(dev, true, 0, false);

    // Close the device
    LMS_Close(dev);

    // Set status
    running = false;
}

void LimeSDR::rxWorker() {
    int sampCount = samplerate / 200;
    lms_stream_meta_t meta;

    while (true) {
        LMS_RecvStream(&rxStream, out.writeBuf, sampCount, &meta, 1000);
        if (!out.swap(sampCount)) { break; }
    }
}

void LimeSDR::txWorker() {
    int sampCount = samplerate / 200;
    lms_stream_meta_t meta;

    while (true) {
        // Get the transmitter samples
        int count = in->read();
        if (count <= 0) { break; }

        // Send the samples
        LMS_SendStream(&txStream, in->readBuf, count, &meta, 1000);

        // Flush the modulator stream
        in->flush();
    }
}