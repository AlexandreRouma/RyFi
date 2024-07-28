#pragma once
#ifdef BUILD_LIMESDR_SUPPORT
#include "../device.h"
#include <lime/LimeSuite.h>

#define LIMESDR_DRIVER_NAME "limesdr"

namespace dev {
    class LimeSDRDriver : public Driver {
    public:
        /**
         * Register the driver.
        */
        static void registerSelf();

        /**
         * List available devices.
         * @return List of available devices.
        */
        std::vector<Info> list();

        // /**
        //  * Open a device for receive.
        //  * @param dev Identifier of the device to open.
        //  * @return Receiver instance.
        // */
        // std::shared_ptr<Receiver> openRX(const std::string& identifier);

        // /**
        //  * Open a device for transmit.
        //  * @param dev Identifier of the device to open.
        //  * @return Transmitter instance.
        // */
        // std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in);
    private:
    };
}
#endif