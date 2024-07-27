#pragma once
#include <string>

#define TUN_MAX_IP_PACKET_SIZE  65536

class TUN {
public:
    /**
     * Create a TUN interface.
     * @param name Name of the interface.
    */
    TUN(const std::string& name);

    // Destructor
    ~TUN();

    /**
     * Close the interface.
    */
    void close();

    /**
     * Get the state of the interface.
     * @return True if the interface is UP, false otherwise.
    */
    bool getState() const;

    /**
     * Set the state of the interface.
     * @param up Whether the interface should be UP or DOWN.
    */
    bool setState(bool up);

   /**
    * Receive an IP packet.
    * @param data Buffer to receive the data into.
    * @param maxLen Maximum number of bytes to receive.
    * @param timeout Timeout to receive the packet. -1 for no timeout.
    * @return Number of bytes received or negative on error.
   */
    int recv(uint8_t* data, int maxLen, int timeout = -1);

    /**
     * Send an IP packet.
     * @param data Buffer containing the packet to send.
     * @param len Number of bytes to send.
     * @return Number of byte sent or negative on error.
    */
    int send(const uint8_t* data, int len);

private:
    int fd = 0;
    bool up = false;
};