#include "cli/cli.h"
#include "flog/flog.h"
#include "ryfi/transmitter.h"
#include "ryfi/receiver.h"
#include "dsp/loop/fast_agc.h"
#include "dsp/taps/low_pass.h"
#include "dsp/filter/fir.h"
#include <signal.h>
#include <fstream>
#include "device/bladerf.h"
#include "device/limesdr.h"
#include "dsp/sink/null_sink.h"
#include <stddef.h>
#include <atomic>
#include "tun.h"
#include "version.h"

#define SDR_SAMPLERATE  1.5e6

#define RX_BAUDRATE     720e3
#define RX_BANDWIDTH    800e3
#define RX_FREQ         435e6

#define TX_BAUDRATE     720e3
#define TX_FREQ         2315e6

std::shared_ptr<TUN> tun;
std::atomic_bool run;

void intHandler(int dummy) {
    run = false;
}

void packetHandler(ryfi::Packet pkt) {
    // Send the received IP packet to the TUN interface
    tun->send(pkt.data(), pkt.size());
}

void sendWorker(ryfi::Transmitter& tx) {
    // Allocate the buffer
    uint8_t* buf = new uint8_t[TUN_MAX_IP_PACKET_SIZE];

    while (true) {
        // Receive an IP packet from the TUN interface
        int len = tun->recv(buf, TUN_MAX_IP_PACKET_SIZE);
        if (len <= 0) { break; }

        // Send the packet over the air
        tx.send(ryfi::Packet(buf, len));
    }

    // Free the buffer
    delete[] buf;
}

int main(int argc, char** argv) {
    // Define the command line interface
    cli::Interface cli;
    cli.arg("device",   'd', "ryfi0",       "TUN Device Name");
    cli.arg("config",   'c', "",            "Load parameters from config");
    cli.arg("rxdev",    'i', "",            "Receive Device");
    cli.arg("txdev",    'o', "",            "Transmit Device");
    cli.arg("rxfreq",   'r', 435e6,         "Receive Frequency");
    cli.arg("txfreq",   't', 2315e6,        "Transmit Frequency");
    cli.arg("baudrate", 'b', 720e3,         "Baudrate");
    cli.arg("udpdump",  'u', false,         "Export samples to UDP for monitoring");
    cli.arg("udphost",  'a', "localhost",   "UDP Host for RX Sample Export");
    cli.arg("udpport",  'p', 1234,          "UDP Port for RX Sample Export");

    // Parse the command line
    cli::Command cmd;
    try {
        cmd = cli::parse(cli, argc, argv);
    }
    catch (const std::exception& e) {
        flog::error("{}", e.what());
        return -1;
    }

    // Show info line
    flog::info("RyFi v" RYFI_VERSION " by Ryzerth ON5RYZ");

    // Catch CTRL+C
    signal(SIGINT, intHandler);

    // Create the TUN interface
    std::string iface = cmd["device"];
    flog::info("Creating the TUN interface '{}'...", iface);
    try {
        tun = std::make_shared<TUN>(iface);
    }
    catch (const std::exception& e) {
        flog::error("Failed to create TUN interface: {}", e.what());
    }

    // Intialize the TX DSP
    flog::info("Initialising the transmitter...");
    ryfi::Transmitter tx(TX_BAUDRATE, SDR_SAMPLERATE);
    dsp::loop::FastAGC<dsp::complex_t> agc(tx.out, 0.5, 1e6, 0.00001, 0.00001);

    // Initialize the SDR
    flog::info("Initialising the SDR...");
    BladeRF sdr(&agc.out, SDR_SAMPLERATE, cmd["rxfreq"], cmd["txfreq"]);
    //LimeSDR sdr(&agc.out, SDR_SAMPLERATE, cmd["rxfreq"], cmd["txfreq"]);

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

    // Start the sender thread
    std::thread sendThread(sendWorker, tx);

    // Do nothing
    flog::info("Ready! Press CTRL+C to stop.");
    while (run) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    // TODO: Stop sender thread
    if (sendThread.joinable()) { sendThread.join(); }

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