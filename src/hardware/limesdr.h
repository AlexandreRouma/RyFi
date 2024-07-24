#pragma once
#include <thread>
#include "dsp/stream.h"
#include "dsp/types.h"
#include <stddef.h>
#include <lime/LimeSuite.h>

class LimeSDR {
public:
    LimeSDR(dsp::stream<dsp::complex_t>* in, double samplerate, double rxFrequency, double txFrequency);

    ~LimeSDR();

    void start();

    void stop();

    dsp::stream<dsp::complex_t> out;

private:
    void rxWorker();
    void txWorker();

    // Configuration
    dsp::stream<dsp::complex_t>* in;
    double samplerate;
    double rxFrequency;
    double txFrequency;

    // Workers
    std::thread rxWorkerThread;
    std::thread txWorkerThread;

    // Device
    lms_device_t* dev;
    lms_stream_t rxStream;
    lms_stream_t txStream;

    // Status
    bool running = false;
};