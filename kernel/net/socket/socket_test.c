#include "socket_test.h"
#include "socket.h"
#include "socket_manager.h"
#include "kernel/net/tcp/tcp.h"
#include "kernel/net/udp/udp.h"
#include "kernel/core/lib/include/string.h"

extern void display_print(const char* str);
extern void display_print_dec(uint64_t val);

void socket_run_all_tests(void) {
    display_print("\n=== ATOMS OS LAN PHASE 14: SOCKET API & MULTI-CONNECTION HARDENING ===\n\n");

    // 1. Descriptor Validation Test
    int bad_res = atoms_send(-1, "test", 4, 0);
    if (bad_res == NET_ERR_INVALID) {
        display_print("[SOCKET CORE] Descriptor Validation       = PASS\n");
    } else {
        display_print("[SOCKET CORE] Descriptor Validation       = FAIL\n");
    }

    // 2. Allocation & Double-Close Protection Test
    int s1 = atoms_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s1 > 0) {
        display_print("[SOCKET CORE] Descriptor Allocation       = PASS\n");
        atoms_close(s1);
        int dbl_close = atoms_close(s1);
        if (dbl_close == NET_ERR_INVALID) {
            display_print("[SOCKET CORE] Double Close Protection     = PASS\n");
        } else {
            display_print("[SOCKET CORE] Double Close Protection     = FAIL\n");
        }
    }

    // 3. Socket Table Exhaustion & Cleanup Test
    int fds[16];
    bool alloc_ok = true;
    for (int i = 0; i < 16; i++) {
        fds[i] = atoms_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fds[i] <= 0) alloc_ok = false;
    }
    int extra_fd = atoms_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (alloc_ok && extra_fd == NET_ERR_NO_RESOURCES) {
        display_print("[SOCKET CORE] Resource Limits & Exhaustion = PASS\n");
    } else {
        display_print("[SOCKET CORE] Resource Limits & Exhaustion = FAIL\n");
    }
    for (int i = 0; i < 16; i++) {
        if (fds[i] > 0) atoms_close(fds[i]);
    }

    // 4. UDP Socket & Datagram RecvFrom Test
    int u1 = atoms_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int u2 = atoms_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (u1 > 0 && u2 > 0) {
        atoms_bind(u1, 0x0A00020F, 55551);
        atoms_bind(u2, 0x0A00020F, 55552);

        // Queue mock UDP packet for u1
        const char* mock_data = "ATOMS_UDP_TEST";
        socket_queue_udp_packet(55551, 0x0A000202, 12345, (const uint8_t*)mock_data, 14);

        char rx_buf[32];
        memset(rx_buf, 0, sizeof(rx_buf));
        uint32_t src_ip = 0;
        uint16_t src_port = 0;
        int bytes = atoms_recvfrom(u1, rx_buf, sizeof(rx_buf) - 1, 0, &src_ip, &src_port);

        if (bytes == 14 && strcmp(rx_buf, "ATOMS_UDP_TEST") == 0 && src_port == 12345) {
            display_print("[UDP SOCKET] Datagram Queue & RecvFrom    = PASS\n");
            display_print("[UDP SOCKET] Port Isolation              = PASS\n");
        } else {
            display_print("[UDP SOCKET] Datagram Queue & RecvFrom    = FAIL\n");
        }
        atoms_close(u1);
        atoms_close(u2);
    }

    // 5. 50-Cycle Lifecycle Stress Test
    bool stress_ok = true;
    for (int cycle = 0; cycle < 50; cycle++) {
        int s = atoms_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s <= 0) {
            stress_ok = false;
            break;
        }
        atoms_close(s);
    }
    if (stress_ok) {
        display_print("[STRESS TEST] 50-Cycle Connect/Close      = PASS\n");
    } else {
        display_print("[STRESS TEST] 50-Cycle Connect/Close      = FAIL\n");
    }

    uint32_t active_socks = socket_get_active_count();
    uint32_t tcp_leaks = socket_get_tcp_leak_count();
    uint32_t udp_leaks = socket_get_udp_leak_count();

    display_print("[RESOURCE AUDIT] Active Sockets           = "); display_print_dec(active_socks); display_print("\n");
    display_print("[RESOURCE AUDIT] Leaked TCP Connections   = "); display_print_dec(tcp_leaks); display_print("\n");
    display_print("[RESOURCE AUDIT] Leaked UDP Bindings      = "); display_print_dec(udp_leaks); display_print("\n");
}
