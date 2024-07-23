#include "flog/flog.h"
#include "ryfi/transmitter.h"
#include "ryfi/receiver.h"
#include "dsp/loop/fast_agc.h"
#include "dsp/taps/low_pass.h"
#include "dsp/filter/fir.h"
#include <libbladeRF.h>
#include <signal.h>
#include <fstream>
#include "bladerf.h"
#include "dsp/sink/null_sink.h"

#define SDR_SAMPLERATE  1.5e6

#define RX_BAUDRATE     720e3
#define RX_BANDWIDTH    800e3
#define RX_FREQ         435e6

#define TX_BAUDRATE     720e3
#define TX_FREQ         2315e6



volatile bool run = true;
void intHandler(int dummy) {
    run = false;
}

void packetHandler(ryfi::Packet pkt) {
    flog::debug("Got packet: '{}'", (char*)pkt.data());
}

int main() {
    flog::info("RyFi Version 0.1.0");

    // Catch CTRL+C
    signal(SIGINT, intHandler);

    // Intialize the TX DSP
    flog::info("Initialising the transmitter...");
    ryfi::Transmitter tx(TX_BAUDRATE, SDR_SAMPLERATE);
    dsp::loop::FastAGC<dsp::complex_t> agc(tx.out, 0.5, 1e6, 0.00001, 0.00001);

    // Initialize the SDR
    flog::info("Initialising the SDR...");
    BladeRF sdr(&agc.out, SDR_SAMPLERATE, RX_FREQ, TX_FREQ);

    // Intialize the RX DSP
    flog::info("Initialising the receiver...");
    dsp::tap lpTaps = dsp::taps::lowPass(RX_BANDWIDTH / 2.0, RX_BANDWIDTH / 20.0f, SDR_SAMPLERATE);
    dsp::filter::FIR<dsp::complex_t, float> lp(&sdr.out, lpTaps);
    ryfi::Receiver rx(&lp.out, RX_BAUDRATE, SDR_SAMPLERATE);
    rx.onPacket.bind(packetHandler);
    dsp::sink::Null<dsp::complex_t> ns(rx.softOut, NULL, NULL);

    // Start the DSP
    flog::info("Starting the DSP...");
    tx.start();
    agc.start();
    lp.start();
    rx.start();
    ns.start();

    // Start the SDR
    flog::info("Starting the SDR...");
    try {
        sdr.start();
    }
    catch (const std::exception& e) {
        flog::error("Failed to start SDR: {}", e.what());
        return -1;
    }

    // Do nothing
    flog::info("Ready!");
    while (run) {
        // Read a line
        printf("\n> ");
        char line[1024];
        scanf("%[^\n]", line);
        char dummy;
        fread(&dummy, 1, 1, stdin);

        // Send it including the null byte
        tx.send(ryfi::Packet((uint8_t*)line, strlen(line)+1));
    }

    // Stop the SDR
    flog::info("Stopping the SDR...");
    sdr.stop();

    // Stop the DSP
    flog::info("Stopping the DSP...");
    tx.stop();
    agc.stop();
    lp.stop();
    rx.stop();
    ns.stop();

    // Exit
    flog::info("All done!");
    return 0;
}