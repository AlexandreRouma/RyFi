#pragma once
#include "dsp/stream.h"
#include "dsp/types.h"
#include <vector>
#include <memory>

namespace dev {
    enum Type {
        DEV_TYPE_RECEIVER       = (1 << 0),
        DEV_TYPE_TRANSMITTER    = (1 << 1)
    };
    inline constexpr Type operator|(Type a, Type b) { return (Type)((int)a | (int)b); }

    struct Info {
        // Type of device.
        Type type;

        // Name of the driver.
        std::string driver;

        // Identifier of the d
        std::string identifier;
    };

    class Receiver {
    public:
        // Destructor
        virtual ~Receiver();

        /**
         * Close the device.
        */
        virtual void close() = 0;

        /**
         * Get the best samplerate to use given a minimum required samplerate.
        */
        virtual double getBestSamplerate(double min) = 0;

        /**
         * Set the samplerate.
         * @param samplerate Samplerate in Hz.
        */
        virtual void setSamplerate(double samplerate) = 0;

        /**
         * Tune the device
         * @param freq Frequency to tune to in Hz.
        */
        virtual void tune(double freq) = 0;

        /**
         * Start the device.
        */
        virtual void start() = 0;

        /**
         * Stop the device.
        */
        virtual void stop() = 0;

        // Output stream
        dsp::stream<dsp::complex_t> out;
    
    protected:
        double rxFreq;
        bool running = false;
    };

    class Transmitter {
    public:
        Transmitter(dsp::stream<dsp::complex_t>* in);
        virtual ~Transmitter();

        /**
         * Close the device.
        */
        virtual void close() = 0;

        /**
         * Get the best samplerate to use given a minimum required samplerate.
        */
        virtual double getBestSamplerate(double min) = 0;

        /**
         * Set the samplerate.
         * @param samplerate Samplerate in Hz.
        */
        virtual void setSamplerate(double samplerate) = 0;

        /**
         * Tune the device
         * @param freq Frequency to tune to in Hz.
        */
        virtual void tune(double freq) = 0;

        /**
         * Start the device.
        */
        virtual void start() = 0;

        /**
         * Stop the device.
        */
        virtual void stop() = 0;

    protected:
        dsp::stream<dsp::complex_t>* in;
        double txFreq;
        bool running = false;
    };

    class Driver {
    public:
        /**
         * List available devices.
         * @return List of available devices.
        */
        virtual std::vector<Info> list() = 0;

        /**
         * Open a device for receive.
         * @param dev Identifier of the device.
         * @return Receiver instance.
        */
        virtual std::shared_ptr<Receiver> openRX(const std::string& identifier);

        /**
         * Open a device for transmit.
         * @param dev Identifier of the device.
         * @return Transmitter instance.
        */
        virtual std::shared_ptr<Transmitter> openTX(const std::string& identifier, dsp::stream<dsp::complex_t>* in);
    };

    /**
     * Register a device driver.
     * @param name Name of the driver.
     * @param driver Driver instance.
    */
    void registerDriver(const std::string& name, std::unique_ptr<Driver> driver);

    /**
     * List registered device drivers.
     * @return List of all registered device drivers.
    */
    std::vector<std::string> listDrivers();

    /**
     * List available devices.
     * @return List of available devices.
    */
    std::vector<Info> list();

    /**
     * Open a device for receive.
     * @param driver Driver to which the device belongs.
     * @param identifier Identifier of the device.
     * @return Receiver instance.
    */
    std::shared_ptr<Receiver> openRX(const std::string& driver, const std::string& identifier);

    /**
     * Open a device for transmit.
     * @param driver Driver to which the device belongs.
     * @param identifier Identifier of the device.
     * @return Transmitter instance.
    */
    std::shared_ptr<Transmitter> openTX(const std::string& driver, const std::string& identifier, dsp::stream<dsp::complex_t>* in);
}

