#pragma once
#include <thread>
#include <libbladeRF.h>
#include "dsp/stream.h"
#include "dsp/types.h"
#include <stddef.h>

class BladeRF {
public:
    BladeRF(dsp::stream<dsp::complex_t>* in, double samplerate, double rxFrequency, double txFrequency);

    ~BladeRF();

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
    struct bladerf *dev = NULL;

    // Status
    bool running = false;
};