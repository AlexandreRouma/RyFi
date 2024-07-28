#pragma once
#ifdef BUILD_BLADERF_SUPPORT
#include "../device.h"
#include <libbladeRF.h>
#include <map>

#define BLADERF_DRIVER_NAME "bladerf"

namespace dev {
    struct BladeRFContext {
        bladerf* dev;
        int refCount;
    };

    class BladeRFReceiver : public Receiver {
    public:
        BladeRFReceiver(BladeRFDriver* drv, bladerf* dev);

        // Destructor
        ~BladeRFReceiver();

        /**
         * Close the device.
        */
        void close();

        /**
         * Get the best samplerate to use given a minimum required samplerate.
        */
        double getBestSamplerate(double min);

        /**
         * Set the samplerate.
         * @param samplerate Samplerate in Hz.
        */
        void setSamplerate(double samplerate);

        /**
         * Tune the device
         * @param freq Frequency to tune to in Hz.
        */
        void tune(double freq);

        /**
         * Start the device.
        */
        void start();

        /**
         * Stop the device.
        */
        void stop();

    private:
        void worker();

        BladeRFDriver* drv;
        bladerf* dev;

        std::thread workerThread;
    };

    class BladeRFDriver : public Driver {
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

        /**
         * Open a device for receive.
         * @param dev Identifier of the device to open.
         * @return Receiver instance.
        */
        std::shared_ptr<Receiver> openRX(const std::string& identifier);

        // /**
        //  * Open a device for transmit.
        //  * @param dev Identifier of the device to open.
        //  * @return Transmitter instance.
        // */
        // std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in);

        friend BladeRFReceiver;

    private:
        void releaseContext(bladerf* dev);

        std::map<std::string, BladeRFContext> ctxs;
    };
}
#endif