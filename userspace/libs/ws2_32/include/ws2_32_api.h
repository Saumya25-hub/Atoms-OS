#ifndef BOS_WS2_32_API_H
#define BOS_WS2_32_API_H

#include "ws2_32_types.h"

int32_t WS2_32Initialize(void);
int32_t WS2_32Shutdown(void);

int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
int WSACleanup(void);

SOCKET socket(int af, int type, int protocol);
int bind(SOCKET s, const struct sockaddr* name, int namelen);
int listen(SOCKET s, int backlog);
SOCKET accept(SOCKET s, struct sockaddr* addr, int* addrlen);
int connect(SOCKET s, const struct sockaddr* name, int namelen);

int closesocket(SOCKET s);
int shutdown(SOCKET s, int how);

int send(SOCKET s, const char* buf, int len, int flags);
int recv(SOCKET s, char* buf, int len, int flags);

int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);
int recvfrom(SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen);

int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timeval* timeout);
int poll(struct pollfd* fds, unsigned long nfds, int timeout);

int setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);
int getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);

int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** result);
void freeaddrinfo(struct addrinfo* ai);

int gethostname(char* name, int namelen);

const char* inet_ntop(int af, const void* src, char* dst, size_t size);
int inet_pton(int af, const char* src, void* dst);

int WSAGetLastError(void);
void WSASetLastError(int err);

HANDLE WSAEventSelect(SOCKET s, HANDLE hEventObject, long lNetworkEvents);
int ioctlsocket(SOCKET s, long cmd, unsigned long* argp);

int __WSAFDIsSet(SOCKET s, fd_set* set);

void ws2_32_run_certification_suite(void);

#endif // BOS_WS2_32_API_H
