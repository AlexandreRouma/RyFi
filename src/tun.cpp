#include "tun.h"
#include <stdexcept>

#ifdef _WIN32

#else
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#endif

TUN::TUN(const std::string& name) {
#ifdef _WIN32

#else
    // Open the TUN driver
    fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC);
    if (fd <= 0) {
        throw std::runtime_error("Failed to open the TUN driver");
    }

    // Prepare a configuration struct
    ifreq req;
    memset(&req, 0, sizeof(req));

    // Configure the flags and copy the interface name
    req.ifr_flags = IFF_TUN | IFF_NO_PI;
    memcpy(req.ifr_name, name.c_str(), name.size() + 1);

    // Apply the configuration
    int err = ioctl(fd, TUNSETIFF, &req);
    if (err == -1) {
        // Close the TUN driver
        ::close(fd);

        // Throw an error
        throw std::runtime_error("Failed to configure the TUN interface");
    }
#endif
}

TUN::~TUN() {
    // Close the interface
    close();
}

void TUN::close() {
#ifdef _WIN32

#else
    // If the interface is already close, do nothing
    if (fd <= 0) { return; }

    // Close the interface
    ::close(fd);

    // Mark as closed
    fd = 0;
#endif
}

bool TUN::getState() const {
    return up;
}

bool TUN::setState(bool up) {
#ifdef _WIN32
    return -1;
#else
    
#endif
}

int TUN::recv(uint8_t* data, int maxLen, int timeout) {
#ifdef _WIN32
    return -1;
#else
    // Read a packet
    return read(fd, data, maxLen);
#endif
}

int TUN::send(const uint8_t* data, int len) {
#ifdef _WIN32
    return -1;
#else
    // Send the packet
    return write(fd, data, len);
#endif
}