#include "../include/ws2_32_api.h"

extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** result) {
    (void)node; (void)service; (void)hints;
    if (!result) return -1;

    struct addrinfo* ai = (struct addrinfo*)kmalloc(sizeof(struct addrinfo));
    struct sockaddr_in* sa = (struct sockaddr_in*)kmalloc(sizeof(struct sockaddr_in));
    if (!ai || !sa) {
        if (ai) kfree(ai);
        if (sa) kfree(sa);
        return -1;
    }

    sa->sin_family = AF_INET;
    sa->sin_port = 80;
    sa->sin_addr.s_addr = 0x0100007F; // 127.0.0.1

    ai->ai_flags = 0;
    ai->ai_family = AF_INET;
    ai->ai_socktype = SOCK_STREAM;
    ai->ai_protocol = IPPROTO_TCP;
    ai->ai_addrlen = sizeof(struct sockaddr_in);
    ai->ai_canonname = NULL;
    ai->ai_addr = (struct sockaddr*)sa;
    ai->ai_next = NULL;

    *result = ai;
    return 0;
}

void freeaddrinfo(struct addrinfo* ai) {
    if (ai) {
        if (ai->ai_addr) kfree(ai->ai_addr);
        kfree(ai);
    }
}
