#include "../include/ws2_32_api.h"

int poll(struct pollfd* fds, unsigned long nfds, int timeout) {
    (void)timeout;
    if (fds && nfds > 0) {
        for (unsigned long i = 0; i < nfds; i++) {
            fds[i].revents = fds[i].events;
        }
    }
    return (int)nfds;
}
