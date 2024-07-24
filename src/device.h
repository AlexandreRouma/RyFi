#include "dsp/stream.h"
#include "dsp/types.h"
#include <vector>

namespace device {
    enum Type {
        DEV_TYPE_RECEIVER       = (1 << 0),
        DEV_TYPE_TRANSMITTER    = (1 << 1)
    };

    class Transmitter {
    public:
        Transmitter(dsp::stream<dsp::complex_t>* in);
        virtual ~Transmitter();

        virtual void tune(double freq) = 0;
        virtual void start() = 0;
        virtual void stop() = 0;

    protected:
        dsp::stream<dsp::complex_t>* in;
        double txFreq;
        bool running = false;
    };

    class Receiver {
    public:
        virtual ~Receiver();

        virtual void tune(double freq) = 0;
        virtual void start() = 0;
        virtual void stop() = 0;

        dsp::stream<dsp::complex_t> out;
    
    protected:
        double rxFreq;
        bool running = false;
    };

    struct DeviceInfo {
        Type type;
        std::string identifier;
    };

    class Driver {

    };

    std::vector<std::string> listDrivers();

    std::vector<DeviceInfo> listDevices();

    void registerDriver(Driver* drv);
}

