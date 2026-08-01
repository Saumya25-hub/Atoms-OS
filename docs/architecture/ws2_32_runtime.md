# 🏛️ WS2_32.sll V1.0 Architecture Specification

> **Subsystem:** WS2_32.sll V1.0 Network Runtime & Winsock Framework  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Ring 3 Networking Layer (Above KERNEL32 & BOSLL)  

---

## 1. Executive Summary & Architectural Philosophy

**WS2_32.sll V1.0** is the official Ring 3 Network Runtime and Winsock Framework inside **ATOMS OS**. Inspired by production networking architectures (Windows `WS2_32.dll`, Winsock 2, BSD Sockets, Linux Socket API, ReactOS, Wine, lwIP, FreeBSD Networking, POSIX Networking), WS2_32.sll provides the single authority for every network-enabled application inside ATOMS OS.

### Core Architectural Mandates:
- **Public Networking Runtime Authority**: Every Ring 3 application (Atrix Browser, Package Manager, AI Clients, FTP, SSH, Cloud, Games) delegates socket creation, address resolution, connection setup, polling, data transfer, and socket options to WS2_32.sll.
- **Zero TCP/IP Stack Ownership**: WS2_32.sll owns zero TCP/UDP packet processing, IP routing, or NIC driver code. Those belong exclusively to the Kernel Network Manager, TCP/IP Stack, and NIC Drivers.
- **Layered Subsystem Flow**:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │       Ring 3 Applications (Browser, Package Mgr, Apps)      │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Standard Winsock 2 / POSIX API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       WS2_32.sll                            │
 │  ├── 1. Runtime Manager       ├── 11. Poll Runtime           │
 │  ├── 2. Socket Engine         ├── 12. Socket Options Engine  │
 │  ├── 3. TCP Runtime           ├── 13. Interface Manager      │
 │  ├── 4. UDP Runtime           ├── 14. Security Runtime       │
 │  ├── 5. Address Manager       ├── 15. Buffer Manager        │
 │  ├── 6. DNS Resolver          ├── 16. Performance Runtime    │
 │  ├── 7. Data Transfer Engine  ├── 17. Diagnostics Engine     │
 │  ├── 8. Async Runtime         ├── 18. Compatibility Runtime  │
 │  ├── 9. Event Engine          ├── 19. Resource Manager       │
 │  └── 10. Select Runtime       └── 20. 300-Test Suite         │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Subsystem Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                     KERNEL32.sll / BOSLL.sll                │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Socket Syscall Gateway
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               Kernel Network Manager & TCP/IP Stack         │
 └──────────────────────────────┬──────────────────────────────┘
                                │ NIC Hardware Drivers
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                    Ethernet / Wi-Fi Hardware                │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/ws2_32/`)

```text
userspace/libs/ws2_32/
├── include/
│   ├── ws2_32_types.h
│   ├── ws2_32_api.h
│   └── ws2_32_public.h
├── core/
│   ├── ws2_runtime.c
│   ├── ws2_transfer.c
│   └── ws2_resource.c
├── startup/
│   └── ws2_compat.c
├── socket/
│   └── ws2_socket.c
├── tcp/
│   └── ws2_tcp.c
├── udp/
│   └── ws2_udp.c
├── address/
│   ├── ws2_address.c
│   └── ws2_interface.c
├── dns/
│   └── ws2_dns.c
├── async/
│   └── ws2_async.c
├── events/
│   └── ws2_events.c
├── select/
│   └── ws2_select.c
├── poll/
│   └── ws2_poll.c
├── options/
│   └── ws2_options.c
├── security/
│   └── ws2_security.c
├── buffers/
│   └── ws2_buffers.c
├── performance/
│   └── ws2_performance.c
├── diagnostics/
│   └── ws2_diagnostics.c
├── tests/
│   └── ws2_32_certification_tests.c
└── docs/
    └── ws2_32_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Manager**: `WSAStartup`, `WSACleanup`, subsystem init & version negotiation.
2. **Socket Engine**: `socket`, `bind`, `listen`, `accept`, `connect`, `shutdown`, `closesocket`.
3. **TCP Runtime**: Reliable stream sockets, connection state tracking.
4. **UDP Runtime**: Datagram sockets, broadcast, multicast handling.
5. **Address Manager**: `sockaddr`, `sockaddr_in`, `sockaddr_in6`, `inet_ntop`, `inet_pton`.
6. **DNS Resolver**: `getaddrinfo`, `freeaddrinfo`, `gethostbyname`, DNS caching.
7. **Data Transfer Engine**: `send`, `recv`, `sendto`, `recvfrom`.
8. **Async Runtime**: Non-blocking sockets, async connect/send/recv (`ioctlsocket`).
9. **Event Engine**: `WSAEventSelect`, network event notifications.
10. **Select Runtime**: `select`, `FD_SET`, `FD_CLR`, `FD_ZERO`, `FD_ISSET`.
11. **Poll Runtime**: `poll`, event masks, ready socket waiting.
12. **Socket Options Engine**: `setsockopt`, `getsockopt`, timeouts, reuseaddr, keepalive, nodelay.
13. **Interface Manager**: `gethostname`, network interface enumeration, loopback.
14. **Security Runtime**: Socket access control, network permissions.
15. **Buffer Manager**: Ring buffers, zero-copy transfer buffers.
16. **Performance Runtime**: Socket, DNS, and connection caches.
17. **Diagnostics Engine**: Packet statistics, socket dumps, memory profiling.
18. **Compatibility Runtime**: `WSAGetLastError`, `WSASetLastError`, legacy behavior.
19. **Resource Manager**: Socket handle allocations and reference counting.
20. **Certification Battery**: 300-test production certification suite (`ws2_32_certification_tests.c`).
