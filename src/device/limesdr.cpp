#ifdef BUILD_LIMESDR_SUPPORT
#include "limesdr.h"
#include "flog/flog.h"
#include <algorithm>

namespace dev {
    LimeSDRReceiver::LimeSDRReceiver(LimeSDRDriver* drv, lms_device_t* dev, int channel) {
        // Save parameters
        this->drv = drv;
        this->dev = dev;
        this->channel = channel;

        // Get available antennas
        lms_name_t antennas[16];
        int antCount = LMS_GetAntennaList(dev, false, channel, antennas);

        // Select the automatic antenna path
        LMS_SetAntenna(dev, false, channel, antCount-1);

        // Set the gain to a sane value (TODO: NO)
        LMS_SetGaindB(dev, false, channel, 30);
        
        // Enable the LPF
        LMS_SetLPF(dev, false, channel, true);
    }

    LimeSDRReceiver::~LimeSDRReceiver() {
        // Stop the device
        stop();

        // Close the device
        close();
    }

    void LimeSDRReceiver::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double LimeSDRReceiver::getBestSamplerate(double min) {
        return -1;
    }

    void LimeSDRReceiver::setSamplerate(double samplerate) {
        // Set the samplerate
        LMS_SetSampleRate(dev, samplerate, 0);

        // Set the filter bandwidth
        LMS_SetLPFBW(dev, false, channel, samplerate);

        // Update the samplerate
        this->samplerate = samplerate;
    }

    void LimeSDRReceiver::tune(double freq) {
        // Send tune command
        LMS_SetLOFrequency(dev, false, channel, freq);
    }

    void LimeSDRReceiver::start() {
        // If already running, do nothing
        if (running) { return; }

        // Enable the channel (TODO: Might need to be done right when opening)
        LMS_EnableChannel(dev, false, channel, true);

        // Run calibration with latest settings
        LMS_Calibrate(dev, false, channel, samplerate, 0);

        // Setup the stream
        stream.isTx = false;
        stream.channel = 0;
        stream.fifoSize =  1024*16; // Whatever the fuck this means
        stream.throughputVsLatency = 0.5f;
        stream.dataFmt = stream.LMS_FMT_F32;
        LMS_SetupStream(dev, &stream);

        // Start the streams
        int count = LMS_StartStream(&stream);
        if (count) { throw std::runtime_error("Failed to start RX stream"); }

        // Start the worker
        workerThread = std::thread(&LimeSDRReceiver::worker, this);

        // Mark as running
        running = true;
    }

    void LimeSDRReceiver::stop() {
        // If not running, do nothing
        if (!running) { return; }

        // Stop the worker
        out.stopWriter();
        if (workerThread.joinable()) { workerThread.join(); }
        out.clearWriteStop();

        // Stop the streams
        LMS_StopStream(&stream);

        // Disable the channels
        LMS_EnableChannel(dev, false, channel, false);

        // Mark as not running
        running = false;
    }

    void LimeSDRReceiver::worker() {
        int sampCount = samplerate / 200;
        lms_stream_meta_t meta;

        while (true) {
            LMS_RecvStream(&stream, out.writeBuf, sampCount, &meta, 1000);
            if (!out.swap(sampCount)) { break; }
        }
    }

    void LimeSDRDriver::registerSelf() {
        registerDriver(LIMESDR_DRIVER_NAME, std::make_unique<LimeSDRDriver>());
    }

    std::vector<Info> LimeSDRDriver::list() {
        // Create the list
        std::vector<Info> list;
        
        // Get the list of devices
        lms_info_str_t devList[16];
        int count = LMS_GetDeviceList(devList);
        if (count < 0) {
            flog::warn("Failed to list LimeSDR devices");
            return list;
        }

        // Go through each device
        for (int i = 0; i < count; i++) {
            // Open the device
            lms_device_t* dev = NULL;
            int err = LMS_Open(&dev, devList[i], NULL);
            if (err) {
                flog::warn("Failed to open LimeSDR device");
                continue;
            }

            // Get the device info
            const lms_dev_info_t* dinfo = LMS_GetDeviceInfo(dev);

            // Get the serial number
            char serial[256];
            sprintf(serial, "%016" PRIX64, dinfo->boardSerialNumber);
            
            // Close the device
            LMS_Close(dev);

            // Create the info struct
            Info info = { DEV_TYPE_RECEIVER | DEV_TYPE_TRANSMITTER, LIMESDR_DRIVER_NAME, serial };

            // Save the info struct
            list.push_back(info);
        }

        // Return the list
        return list;
    }

    std::shared_ptr<Receiver> LimeSDRDriver::openRX(const std::string& identifier) {
        // Create the device from the new context
        return std::make_shared<LimeSDRReceiver>(this, acquireContext(identifier), 0);
    }

    // std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {

    // }

    lms_device_t* LimeSDRDriver::acquireContext(const std::string& identifier) {
        // Check the identifier
        if (identifier.size() != 16) {
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

        // Attempt to open the device
        lms_device_t* dev;
        char buf[512];
        sprintf(buf, "serial=%s", identifier.c_str());
        int err = LMS_Open(&dev, buf, NULL);
        if (err) { throw std::runtime_error("Failed to open device"); }

        // Create and append a new context entry
        LimeSDRContext ctx = { dev, 1 };
        ctxs[identifier] = ctx;

        // Return the new context
        return dev;
    }

    void LimeSDRDriver::releaseContext(lms_device_t* dev) {
        // Find the context entry corresponding to it
        auto it = std::find_if(ctxs.begin(), ctxs.end(), [dev](const std::pair<std::string, LimeSDRContext>& p) { return p.second.dev == dev; });
        if (it == ctxs.end()) { throw std::runtime_error("Tried to release a context that doesn't exist"); }

        // Decrement the refcount and do nothing if it's greater than 0
        if (--(it->second.refCount)) { return; }

        // Remove the context entry from the list
        ctxs.erase(it);

        // Close the device
        LMS_Close(dev);
    }
}
#endif