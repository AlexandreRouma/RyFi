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

    class BladeRFDriver;

    class BladeRFReceiver : public Receiver {
    public:
        BladeRFReceiver(BladeRFDriver* drv, bladerf* dev, int channel);

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
        int channel;
        int bufferSize;

        std::thread workerThread;
    };

    class BladeRFTransmitter : public Transmitter {
    public:
        BladeRFTransmitter(dsp::stream<dsp::complex_t>* in, BladeRFDriver* drv, bladerf* dev, int channel);
        ~BladeRFTransmitter();

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

    protected:
        void worker();

        BladeRFDriver* drv;
        bladerf* dev;
        int channel;
        int bufferSize;

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

        /**
         * Open a device for transmit.
         * @param dev Identifier of the device to open.
         * @return Transmitter instance.
        */
        std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in);

        // Friend classes
        friend BladeRFReceiver;
        friend BladeRFTransmitter;

    private:
        bladerf* acquireContext(const std::string& identifier);
        void releaseContext(bladerf* dev);

        std::map<std::string, BladeRFContext> ctxs;
        std::vector<Info> devListCache;
    };
}
#endif