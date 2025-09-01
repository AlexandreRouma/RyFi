#include "cli/cli.h"
#include "flog/flog.h"
#include "ryfi/transmitter.h"
#include "ryfi/receiver.h"
#include "dsp/loop/fast_agc.h"
#include "dsp/taps/low_pass.h"
#include "dsp/filter/fir.h"
#include <signal.h>
#include <fstream>
#include "dsp/sink/null_sink.h"
#include <stddef.h>
#include <atomic>
#include "tun.h"
#include "version.h"
#include "device.h"
#include "device/bladerf.h"
#include "device/limesdr.h"
#include "device/usrp.h"

//#define SDR_SAMPLERATE  1.5e6

#define SDR_SAMPLERATE  1.5*720e3

#define RX_BAUDRATE     720e3
#define RX_BANDWIDTH    800e3
#define RX_FREQ         435e6

#define TX_BAUDRATE     720e3
#define TX_FREQ         2315e6

std::shared_ptr<TUN> tun;
std::atomic_bool run = true;

void intHandler(int dummy) {
    run = false;
}

void packetHandler(ryfi::Packet pkt) {
    // Send the received IP packet to the TUN interface
    tun->send(pkt.data(), pkt.size());
}

void sendWorker(ryfi::Transmitter* tx) {
    // Allocate the buffer
    uint8_t* buf = new uint8_t[TUN_MAX_IP_PACKET_SIZE];

    while (true) {
        // Receive an IP packet from the TUN interface
        int len = tun->recv(buf, TUN_MAX_IP_PACKET_SIZE);
        if (len <= 0) { break; }

        // Send the packet over the air
        tx->send(ryfi::Packet(buf, len));
    }

    // Free the buffer
    delete[] buf;
}

const char* identString = "Identifier";
const char* types[] = { " -INV- ", "RX    ", "    TX", "RX / TX" };

int displayDeviceList() {
    // Get the device list
    auto list = dev::list();

    // If no device was found, notify the user and exit
    if (list.empty()) {
        printf("No device found\n\n");
        return 0;
    }

    // Find the longest device name
    int maxLen = strlen(identString);
    for (const auto& d : list) {
        int len = d.driver.size() + d.identifier.size() + 1;
        if (len > maxLen) { maxLen = len; }
    }

    // Prepare format string depending on longest name
    char fmt[128];
    sprintf(fmt, "%%-%ds | Direction\n", maxLen);

    // Print the title line
    int titleLen = printf(fmt, identString);

    // Print separator
    for (int i = 1; i < titleLen; i++) { printf("-"); }
    printf("\n");

    // Update the formatting
    sprintf(fmt, "%%-%ds | %%s\n", maxLen);

    // Display all of them
    for (const auto& d : list) {
        char buf[512];
        sprintf(buf, "%s:%s", d.driver.c_str(), d.identifier.c_str());
        printf(fmt, buf, types[(int)d.type]);
    }
    printf("\n");

    // Return successfully
    return 0;
}

int displayDriverList() {
    // Get the driver list
    auto drivers = dev::listDrivers();

    // Check if no driver is available
    if (drivers.empty()) {
        printf("RyFi was compiled with no drivers, this is stupid...\n\n");
        return 0;
    }

    // Print beginning of line
    printf("Available drivers:\n");

    // Print each driver's name
    for (const auto& name : drivers) {
        printf(" * %s\n", name.c_str());
    }

    // Add line feeds to space things out
    printf("\n");

    // Return successfully
    return 0;
}

int main(int argc, char** argv) {
    try {
        // Register device drivers
#ifdef BUILD_BLADERF_SUPPORT
        dev::BladeRFDriver::registerSelf();
#endif
#ifdef BUILD_LIMESDR_SUPPORT
        dev::LimeSDRDriver::registerSelf();
#endif
#ifdef BUILD_USRP_SUPPORT
        dev::USRPDriver::registerSelf();
#endif

        // Define the command line interface
        cli::Interface cli;
        cli.arg("tun",          'd', "ryfi0",       "TUN interface name");
        cli.arg("config",       'c', "",            "Load parameters from a configuration file");
        cli.arg("list",         'l', false,         "List SDR devices");
        cli.arg("drivers",       0,  false,         "List SDR drivers that RyFi was compiled with");
        cli.arg("rxdev",        'i', "",            "Receive SDR device in the format driver:serial.");
        cli.arg("txdev",        'o', "",            "Transmit SDR device in the format driver:serial.");
        cli.arg("rxfreq",       'r', 435e6,         "Receive Frequency");
        cli.arg("txfreq",       't', 2315e6,        "Transmit Frequency");
        cli.arg("baudrate",     'b', 720e3,         "Baudrate");
        cli.arg("udpdump",      'u', false,         "Dump RX samples to UDP for monitoring");
        cli.arg("udphost",      'a', "localhost",   "UDP host for RX sample dump");
        cli.arg("udpport",      'p', 1234,          "UDP port for RX sample dump");
        cli.arg("genconfig",     0,  "",            "Save parameters to a configuration file and exit");

        // Parse the command line
        cli::Command cmd = cli::parse(cli, argc, argv);

        // If asked to display the device list
        if (cmd["list"]) {
            // Show device list and exit
            return displayDeviceList();
        }

        // If asked to display the driver list
        if (cmd["drivers"]) {
            // Show driver list and exit
            return displayDriverList();
        }

        // Show info line
        flog::info("RyFi v" RYFI_VERSION " by Ryzerth ON5RYZ");

        // Get the selected baudrate
        double baudrate = cmd["baudrate"];

        // Check that a RX and TX device have been given
        std::string rxdev = cmd["rxdev"];
        std::string txdev = cmd["txdev"];
        if (rxdev.empty() || txdev.empty()) {
            flog::error("Both an RX and TX device must be provided");
            return -1;
        }

        // Create the TUN interface
        std::string iface = cmd["tun"];
        flog::info("Creating the TUN interface '{}'...", iface);
        tun = std::make_shared<TUN>(iface);

        // Intialize the TX DSP
        flog::info("Initialising the transmit DSP...");
        ryfi::Transmitter tx(baudrate, SDR_SAMPLERATE);
        dsp::loop::FastAGC<dsp::complex_t> agc(tx.out, 0.5, 1e6, 0.00001, 0.00001);

        // Open the RX device
        flog::info("Opening the RX device...");
        auto rxd = dev::openRX(rxdev);

        // Configure the RX device
        flog::info("Configuring the RX device...");
        rxd->tune(cmd["rxfreq"]);
        rxd->setSamplerate(SDR_SAMPLERATE);

        // Open the TX device
        flog::info("Opening the TX device...");
        auto txd = dev::openTX(txdev, &agc.out);

        // Configure the TX device
        flog::info("Configuring the TX device...");
        txd->tune(cmd["txfreq"]);
        txd->setSamplerate(SDR_SAMPLERATE);

        // Intialize the RX DSP
        flog::info("Initialising the receive DSP...");
        dsp::tap lpTaps = dsp::taps::lowPass(RX_BANDWIDTH / 2.0, RX_BANDWIDTH / 20.0f, SDR_SAMPLERATE);
        dsp::filter::FIR<dsp::complex_t, float> lp(&rxd->out, lpTaps);
        ryfi::Receiver rx(&lp.out, baudrate, SDR_SAMPLERATE);
        rx.onPacket.bind(packetHandler);
        dsp::sink::Null<dsp::complex_t> ns(rx.softOut, NULL, NULL);

        // Start the DSP
        flog::info("Starting the DSP...");
        tx.start();
        agc.start();
        lp.start();
        rx.start();
        ns.start();

        flog::info("Starting the RX device...");
        rxd->start();

        // Start the TX device
        flog::info("Starting the TX device...");
        try {
            txd->start();
        }
        catch (const std::exception& e) {
            rxd->stop();
            rxd->close();
            throw e;
        }

        // Start the sender thread
#ifdef sdsdWIN32
        // Disabled on Windows due to lack of TUN support
        std::thread sendThread;
#else
        std::thread sendThread(sendWorker, &tx);
#endif

        // Set CTRL+C handler
        signal(SIGINT, intHandler);

        // Do nothing
        flog::info("Ready! Press CTRL+C to stop.");
        while (run) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

        // Remove CTRL+C handler
        signal(SIGINT, SIG_DFL);

        // TODO: Stop sender thread
        if (sendThread.joinable()) { sendThread.join(); }

        // Stop the RX device
        flog::info("Stopping the RX device...");
        rxd->stop();
        rxd->close();

        // Stop the TX device
        flog::info("Stopping the TX device...");
        txd->stop();
        txd->close();

        // Stop the DSP
        flog::info("Stopping the DSP...");
        tx.stop();
        agc.stop();
        lp.stop();
        rx.stop();
        ns.stop();

        // Exit
        flog::info("All done!");
    }
    catch (const std::exception& e) {
        flog::error("{}", e.what());
        return -1;
    }
    return 0;
}