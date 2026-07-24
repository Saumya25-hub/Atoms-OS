#ifndef ATOMS_SDK_NETWORK_SLL_H
#define ATOMS_SDK_NETWORK_SLL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ATOMS Native SDK — network.sll
typedef int32_t SLL_Socket;

SLL_Socket SLL_SocketCreate(int domain, int type, int protocol);
int32_t    SLL_SocketConnect(SLL_Socket sock, const char* host, uint16_t port);
int32_t    SLL_SocketSend(SLL_Socket sock, const void* data, uint32_t len);
int32_t    SLL_SocketRecv(SLL_Socket sock, void* buffer, uint32_t max_len);
void       SLL_SocketClose(SLL_Socket sock);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SDK_NETWORK_SLL_H
