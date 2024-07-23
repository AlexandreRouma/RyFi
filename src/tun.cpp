#include "tun.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>

namespace tun {
    int open(const std::string& name) {
        int tuntap_fd, rc;
        size_t iface_name_len;
        struct ifreq setiff_request;

        tuntap_fd = ::open("/dev/net/tun", O_RDWR | O_CLOEXEC);
        if (tuntap_fd == -1) {
            return NULL;
        }

        memset(&setiff_request, 0, sizeof setiff_request);
        setiff_request.ifr_flags = IFF_TUN | IFF_NO_PI;
        memcpy(setiff_request.ifr_name, name.c_str(), name.size() + 1);
        rc = ioctl(tuntap_fd, TUNSETIFF, &setiff_request);
        if (rc == -1) {
            int ioctl_errno = errno;
            close(tuntap_fd);
            errno = ioctl_errno;
            return NULL;
        }

        return tuntap_fd;
    }
}