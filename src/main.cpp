#include "flog/flog.h"
#include "ryfi/transmitter.h"
#include "ryfi/receiver.h"
#include "dsp/loop/fast_agc.h"
#include "dsp/taps/low_pass.h"
#include "dsp/filter/fir.h"
#include <signal.h>
#include <fstream>
#include "bladerf.h"
#include "limesdr.h"
#include "dsp/sink/null_sink.h"
#include <stddef.h>
#include "tun.h"

#define SDR_SAMPLERATE  1.5e6

#define RX_BAUDRATE     720e3
#define RX_BANDWIDTH    800e3
#define RX_FREQ         2315e6

#define TX_BAUDRATE     720e3
#define TX_FREQ         435e6



volatile bool run = true;
void intHandler(int dummy) {
    run = false;
}

int iface;

void packetHandler(ryfi::Packet pkt) {
    write(iface, pkt.data(), pkt.size());
}

int main() {
    flog::info("RyFi Version 0.1.0");

    // Catch CTRL+C
    signal(SIGINT, intHandler);

    // Create the TUN interface
    iface = tun::open("ryfi0");
    if (!iface) {
        flog::error("Failed to create TUN interface");
        return -1;
    }

    // Intialize the TX DSP
    flog::info("Initialising the transmitter...");
    ryfi::Transmitter tx(TX_BAUDRATE, SDR_SAMPLERATE);
    dsp::loop::FastAGC<dsp::complex_t> agc(tx.out, 0.5, 1e6, 0.00001, 0.00001);

    // Initialize the SDR
    flog::info("Initialising the SDR...");
    //BladeRF sdr(&agc.out, SDR_SAMPLERATE, RX_FREQ, TX_FREQ);
    LimeSDR sdr(&agc.out, SDR_SAMPLERATE, RX_FREQ, TX_FREQ);

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
        tx.stop();
        agc.stop();
        lp.stop();
        rx.stop();
        ns.stop();
        return -1;
    }

    // Do nothing
    flog::info("Ready!");
    while (run) {
        // Read an IP packet
        uint8_t test[ryfi::Packet::MAX_CONTENT_SIZE];
        int ret = read(iface, test, ryfi::Packet::MAX_CONTENT_SIZE);
        if (ret <= 0) { continue; }

        // Send it
        tx.send(ryfi::Packet(test, ret));
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