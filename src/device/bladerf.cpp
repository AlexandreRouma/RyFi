#ifdef BUILD_BLADERF_SUPPORT
#include "bladerf.h"
#include "flog/flog.h"

namespace dev {
    BladeRFReceiver::BladeRFReceiver(BladeRFDriver* drv, bladerf* dev) {
        // Save parameters
        this->drv = drv;
        this->dev = dev;
    }

    BladeRFReceiver::~BladeRFReceiver() {
        // The the device
        close();
    }

    void BladeRFReceiver::close() {
        // Release the device context if it exits
        if (dev) { drv->releaseContext(dev); }

        // Null out the device context
        dev = NULL;
    }

    double BladeRFReceiver::getBestSamplerate(double min) {

    }

    void BladeRFReceiver::setSamplerate(double samplerate) {

    }

    void BladeRFReceiver::tune(double freq) {
        bladerf_set_frequency(dev, BLADERF_CHANNEL_RX(0), freq);
    }

    void BladeRFReceiver::start() {

    }

    void BladeRFReceiver::stop() {

    }

    void BladeRFDriver::registerSelf() {
        registerDriver(BLADERF_DRIVER_NAME, std::make_unique<BladeRFDriver>());
    }

    std::vector<Info> BladeRFDriver::list() {
        // Create the list
        std::vector<Info> list;
        
        // Get the list of devices
        bladerf_devinfo* devList = NULL;
        int count = bladerf_get_device_list(&devList);
        if (count < 0 && count != BLADERF_ERR_NODEV) {
            flog::warn("Failed to list BladeRF devices");
            return list;
        }

        // Go through each device
        for (int i = 0; i < count; i++) {
            // Get the serial number
            std::string serial = devList[i].serial;

            // Create the info struct
            Info info = { DEV_TYPE_RECEIVER | DEV_TYPE_TRANSMITTER, BLADERF_DRIVER_NAME, serial };

            // Save the info struct
            list.push_back(info);
        }

        // Free the device list
        if (devList) { bladerf_free_device_list(devList); }

        // Return the list
        return list;
    }

    std::shared_ptr<Receiver> BladeRFDriver::openRX(const std::string& identifier) {
        // Check the identifier
        if (identifier.size() != sizeof(bladerf_devinfo::serial) - 1) {
            throw std::runtime_error("Invalid device identifier");
        }

        // If the device is already open
        auto it = ctxs.find(identifier);
        if (it != ctxs.end()) {
            // Increment the refcount
            it->second.refCount++;

            // Create the device from an existing context
            return std::make_shared<BladeRFReceiver>(it->second.dev);
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

        // Create the device from the new context
        return std::make_shared<BladeRFReceiver>(dev);
    }

    // std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {

    // }
}
#endif