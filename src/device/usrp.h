#pragma once
#ifdef BUILD_USRP_SUPPORT
#include "../device.h"
#include <uhd.h>
#include <uhd/device.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <map>

#define USRP_DRIVER_NAME "usrp"

namespace dev {
    struct USRPContext {
        uhd::usrp::multi_usrp::sptr dev;
        int refCount;
    };

    class USRPDriver;

    class USRPReceiver : public Receiver {
    public:
        /**
         * Create a USRP receiver instance.
         * @param drv Driver instance.
         * @param dev Device instance.
        */
        USRPReceiver(USRPDriver* drv, uhd::usrp::multi_usrp::sptr dev);

        // Destructor
        ~USRPReceiver();

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

        USRPDriver* drv;
        uhd::usrp::multi_usrp::sptr dev;
        uhd::rx_streamer::sptr streamer;
        std::thread workerThread;

        double samplerate;
    };

    class USRPTransmitter : public Transmitter {
    public:
        /**
         * Create a USRP transmitter instance.
         * @param in Input sample stream.
         * @param drv Driver instance.
         * @param dev Device instance.
        */
        USRPTransmitter(dsp::stream<dsp::complex_t>* in, USRPDriver* drv, uhd::usrp::multi_usrp::sptr dev);

        // Destructor
        ~USRPTransmitter();

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

        USRPDriver* drv;
        uhd::usrp::multi_usrp::sptr dev;
        uhd::tx_streamer::sptr streamer;
        std::thread workerThread;

        double samplerate;
    };

    class USRPDriver : public Driver {
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

        // Friend definitions
        friend USRPReceiver;
        friend USRPTransmitter;

    private:
        uhd::usrp::multi_usrp::sptr acquireContext(const std::string& identifier);
        void releaseContext(uhd::usrp::multi_usrp::sptr dev);

        std::map<std::string, USRPContext> ctxs;
        std::vector<Info> devListCache;
    };
}
#endif