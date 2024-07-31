#pragma once
#ifdef BUILD_LIMESDR_SUPPORT
#include "../device.h"
#include <map>
#include <lime/LimeSuite.h>

#define LIMESDR_DRIVER_NAME "limesdr"

namespace dev {
    struct LimeSDRContext {
        lms_device_t* dev;
        int refCount;
    };

    class LimeSDRDriver;

    class LimeSDRReceiver : public Receiver {
    public:
        LimeSDRReceiver(LimeSDRDriver* drv, lms_device_t* dev, int channel);

        // Destructor
        ~LimeSDRReceiver();

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

        LimeSDRDriver* drv;
        lms_device_t* dev;
        lms_stream_t stream;
        int channel;
        int bufferSize;
        double samplerate;

        std::thread workerThread;
    };

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

        // Friend definitions
        friend LimeSDRReceiver;

    private:
        lms_device_t* acquireContext(const std::string& identifier);
        void releaseContext(lms_device_t* dev);

        std::map<std::string, LimeSDRContext> ctxs;
        std::vector<Info> devListCache;
    };
}
#endif