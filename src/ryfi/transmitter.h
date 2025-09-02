#pragma once
#include "dsp/multirate/rrc_interpolator.h"
#include "dsp/filter/fir.h"
#include "packet.h"
#include "frame.h"
#include "rs_codec.h"
#include "conv_codec.h"
#include "framing.h"
#include <queue>
#include <mutex>

namespace ryfi {
    class Transmitter {
    public:
        // Default constructor
        Transmitter();

        /**
         * Create a transmitter.
         * @param baudrate Baudrate to use over the air.
         * @param samplerate Samplerate of the baseband.
        */
        Transmitter(double baudrate, double samplerate);

        // Destructor
        ~Transmitter();

        /**
         * Create a transmitter.
         * @param baudrate Baudrate to use over the air.
         * @param samplerate Samplerate of the baseband.
        */
        void init(double baudrate, double samplerate);

        /**
         * Start the transmitter's DSP.
        */
        void start();

        /**
         * Stop the transmitter's DSP.
        */
        void stop();

        /**
         * Send a packet.
         * @param pkg Packet to send.
         * @return True if the packet was send, false if it was dropped.
        */
        bool send(const Packet& pkt);

        // Baseband output
        dsp::stream<dsp::complex_t>* out;

        static inline const int MAX_QUEUE_SIZE  = 32;

    private:
        bool txFrame(const Frame& frame);
        Packet popPacket();
        void worker();

        // Packet queue
        std::mutex packetsMtx;
        std::queue<Packet> packets;

        // DSP
        dsp::stream<uint8_t> in;
        RSEncoder rs;
        ConvEncoder conv;
        Framer framer;
        dsp::multirate::RRCInterpolator<dsp::complex_t> resamp;
        bool running = false;
        std::thread workerThread;
    };
}