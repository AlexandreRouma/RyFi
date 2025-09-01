#ifdef BUILD_USRP_SUPPORT
#include "usrp.h"
#include "flog/flog.h"

namespace dev {
    // TODO: Make more general

    USRPReceiver::USRPReceiver(USRPDriver* drv, uhd::usrp::multi_usrp::sptr dev) {
        // Save parameters
        this->drv = drv;
        this->dev = dev;
    }

    USRPReceiver::~USRPReceiver() {
        // Stop the device
        stop();

        // Close the device
        close();
    }

    void USRPReceiver::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double USRPReceiver::getBestSamplerate(double min) {
        // Prevent the rate from going under the minimum samplerate
        return std::max<int>(min, dev->get_rx_rates()[0].start());
    }

    void USRPReceiver::setSamplerate(double samplerate) {
        // Save the new samplerate
        this->samplerate = samplerate;

        // Set the samplerate
        dev->set_rx_rate(samplerate);

        // Set the bandwidth
        dev->set_rx_bandwidth(samplerate);
    }

    void USRPReceiver::tune(double freq) {
        // Set the receive frequency
        dev->set_rx_freq(freq);
    }

    void USRPReceiver::start() {
        // Select the receive antenna
        dev->set_rx_antenna("RX2");

        // Enable Auto Gain Control
        dev->set_rx_agc(true);

        // Configure the stream
        uhd::stream_args_t sargs;
        sargs.channels.clear();
        sargs.channels.push_back(0);
        sargs.cpu_format = "fc32";
        sargs.otw_format = "sc16";
        streamer = dev->get_rx_stream(sargs);

        // Start the stream
        streamer->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);

        // Start the worker
        workerThread = std::thread(&USRPReceiver::worker, this);
    }

    void USRPReceiver::stop() {
        // Stop the worker
        out.stopWriter();
        if (workerThread.joinable()) { workerThread.join(); }
        out.clearWriteStop();

        // Stop the stream
        streamer.reset();
    }

    void USRPReceiver::worker() {
        // TODO: Select a better buffer size that will avoid bad timing
        int bufferSize = samplerate / 200;
        try {
            while (true) {
                uhd::rx_metadata_t meta;
                void* ptr[] = { out.writeBuf };
                uhd::rx_streamer::buffs_type buffers(ptr, 1);
                int len = streamer->recv(out.writeBuf, bufferSize, meta, 1.0);
                if (len < 0) { break; }
                if (len != bufferSize) {
                    printf("%d\n", len);
                }
                if (len) {
                    if (!out.swap(len)) { break; }
                }
            }
        }
        catch (const std::exception& e) {
            flog::error("Failed to receive samples: {}", e.what());
        }
    }

    USRPTransmitter::USRPTransmitter(dsp::stream<dsp::complex_t>* in, USRPDriver* drv, uhd::usrp::multi_usrp::sptr dev) {
        // Save parameters
        this->in = in;
        this->drv = drv;
        this->dev = dev;
    }

    USRPTransmitter::~USRPTransmitter() {
        // Stop the device
        stop();

        // Close the device
        close();
    }

    void USRPTransmitter::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double USRPTransmitter::getBestSamplerate(double min) {
        // Prevent the rate from going under the minimum samplerate
        return std::max<int>(min, dev->get_tx_rates()[0].start());
    }

    void USRPTransmitter::setSamplerate(double samplerate) {
        // Set the samplerate
        dev->set_tx_rate(samplerate);

        // Set the bandwidth
        dev->set_tx_bandwidth(samplerate);
    }

    void USRPTransmitter::tune(double freq) {
        // Set the frequency
        dev->set_tx_freq(freq);
    }

    void USRPTransmitter::start() {
        // Select the receive antenna
        dev->set_tx_antenna("TX/RX");

        // Enable Auto Gain Control
        dev->set_tx_gain(dev->get_tx_gain_range().stop());

        // Configure the stream
        uhd::stream_args_t sargs;
        sargs.channels.clear();
        sargs.channels.push_back(0);
        sargs.cpu_format = "fc32";
        sargs.otw_format = "sc16";
        streamer = dev->get_tx_stream(sargs);

        // Start the worker
        workerThread = std::thread(&USRPTransmitter::worker, this);
    }

    void USRPTransmitter::stop() {
        // Stop the worker
        in->stopReader();
        if (workerThread.joinable()) { workerThread.join(); }
        in->clearReadStop();

        // Stop the stream
        streamer.reset();
    }

    void USRPTransmitter::worker() {
        try {
            // Initialize metadata
            uhd::tx_metadata_t meta;
            memset(&meta, 0, sizeof(uhd::tx_metadata_t));
            
            while (true) {
                // Read samples
                int count = in->read();
                if (count <= 0) { break; }

                // Send the samples
                void* ptr[] = { in->readBuf };
                uhd::tx_streamer::buffs_type buffers(ptr, 1);
                streamer->send(buffers, count, meta, 1.0);

                // Flush the samples
                in->flush();
            }
        }
        catch (const std::exception& e) {
            flog::error("Failed to receive samples: {}", e.what());
        }
    }

    void USRPDriver::registerSelf() {
        // Create an instance of the driver and register it
        registerDriver(USRP_DRIVER_NAME, std::make_unique<USRPDriver>());
    }

    std::vector<Info> USRPDriver::list() {
        // If the cache is already populated, return it instead
        if (!devListCache.empty()) { return devListCache; }

        // Get the device list
        uhd::device_addr_t hint;
        uhd::device_addrs_t devList = uhd::device::find(hint);

        // Go through all devices
        char buf[1024];
        for (const auto& devAddr : devList) {
            std::string serial = devAddr["serial"];
            std::string model = devAddr.has_key("product") ? devAddr["product"] : devAddr["type"];
            sprintf(buf, "USRP %s [%s]", model.c_str(), serial.c_str());

            // Create the info struct
            Info info = { DEV_TYPE_RECEIVER | DEV_TYPE_TRANSMITTER, USRP_DRIVER_NAME, serial };

            // Save the info struct
            devListCache.push_back(info);
        }

        // Return the device list
        return devListCache;
    }

    std::shared_ptr<Receiver> USRPDriver::openRX(const std::string& identifier) {
        // Create the device from the new context
        return std::make_shared<USRPReceiver>(this, acquireContext(identifier));
    }

    std::shared_ptr<Transmitter> USRPDriver::openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {
        // Create the device from the new context
        return std::make_shared<USRPTransmitter>(in, this, acquireContext(identifier));
    }

    uhd::usrp::multi_usrp::sptr USRPDriver::acquireContext(const std::string& identifier) {
        // If the device is already open, return it
        auto it = ctxs.find(identifier);
        if (it != ctxs.end()) {
            // Increment the refcount
            it->second.refCount++;

            // Return the existing context
            return it->second.dev;
        }

        // Open the device via serial number
        uhd::device_addr_t addr;
        addr["serial"] = identifier;
        auto dev = uhd::usrp::multi_usrp::make(addr);

        // Create and append a new context entry
        USRPContext ctx = { dev, 1 };
        ctxs[identifier] = ctx;

        // Return the next device
        return dev;
    }
    
    void USRPDriver::releaseContext(uhd::usrp::multi_usrp::sptr dev) {
        // Find the context entry corresponding to it
        auto it = std::find_if(ctxs.begin(), ctxs.end(), [dev](const std::pair<std::string, USRPContext>& p) { return p.second.dev == dev; });
        if (it == ctxs.end()) { throw std::runtime_error("Tried to release a context that doesn't exist"); }

        // Decrement the refcount and do nothing if it's greater than 0
        if (--(it->second.refCount)) { return; }

        // Remove the context entry from the list
        ctxs.erase(it);

        // Close the device
        dev.reset();
    }
}
#endif