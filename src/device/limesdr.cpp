#ifdef BUILD_LIMESDR_SUPPORT
#include "limesdr.h"
#include "flog/flog.h"

namespace dev {
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
            sprintf(serial, "%" PRIX64, dinfo->boardSerialNumber);
            
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

    // std::shared_ptr<Receiver> openRX(const std::string& identifier) {

    // }

    // std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in) {

    // }
}
#endif