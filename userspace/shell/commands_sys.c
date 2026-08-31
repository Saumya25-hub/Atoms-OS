#include "command.h"
#include "../libbos/include/bos.h"
#include <stdbool.h>
#include <stdint.h>

static void cmd_help(int argc, char** argv) {
    (void)argc;
    (void)argv;
    command_print_help();
}

static void cmd_ver(int argc, char** argv) {
    (void)argc;
    (void)argv;
    shell_print("Signatures OS v1.0 - Shell V3\n");
}

static void cmd_about(int argc, char** argv) {
    (void)argc;
    (void)argv;
    shell_print("\n====================================================\n");
    shell_print("               SIGNATURES OS - V1                   \n");
    shell_print("====================================================\n\n");
    shell_print("           Architect & Creator: SAUMYA              \n\n");
    shell_print("       \"Not just an OS. A digital legacy.\"        \n");
    shell_print("     A Masterpiece of System Design and Passion.    \n\n");
    shell_print("====================================================\n\n");
}

static void cmd_cls(int argc, char** argv) {
    (void)argc;
    (void)argv;
    for (int i = 0; i < 25; i++) {
        shell_print("\n");
    }
}

static void cmd_echo(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        shell_print(argv[i]);
        if (i < argc - 1) shell_print(" ");
    }
    shell_print("\n");
}

static void cmd_time(int argc, char** argv) {
    (void)argc;
    (void)argv;
    shell_print("Time command not yet implemented.\n");
}

static void cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    shell_print("Date command not yet implemented.\n");
}

static void cmd_test(int argc, char** argv) {
    (void)argc;
    (void)argv;
    shell_print("Starting Validation & Stress Test Framework...\n");
    // Spawn /tests.elf (Absolute path for VFS)
    uint64_t pid = bos_spawn("/tests.elf");
    if (pid == 0) {
        shell_print("ERROR: Could not spawn /tests.elf. (Maybe it was excluded from this build?)\n");
    } else {
        // Wait for it (we just yield loop for a bit, or assume OS handles interactive spawn correctly)
        // Since the shell doesn't block properly yet, it will just run concurrently.
        // That's fine for testing.
    }
}

static void cmd_minbrow(int argc, char** argv) {
    shell_print("Launching ATRIX Minimal Real-Web Browser Probe (MINBROW.ELF)...\n");
#ifdef BWE_KERNEL
    extern uint32_t minbrow_probe_launch_mode(uint32_t* out_win_id, const char* mode);
    uint32_t win_id = 0;
    const char* mode = (argc > 1) ? argv[1] : "https://www.google.com/";
    minbrow_probe_launch_mode(&win_id, mode);
    shell_print("Minimal Browser Probe active on desktop (Window ID: ");
    shell_print_dec(win_id);
    shell_print(")\n");
#else
    uint64_t pid = bos_spawn("/MINBROW.ELF");
    if (pid == 0 || pid == (uint64_t)-1) {
        pid = bos_spawn("MINBROW.ELF");
    }
    if (pid == 0 || pid == (uint64_t)-1) {
        shell_print("ERROR: Could not spawn MINBROW.ELF. File not found or invalid format.\n");
    } else {
        shell_print("Spawned MINBROW.ELF with PID: ");
        shell_print_dec(pid);
        shell_print("\n");
    }
#endif
}

static void format_shell_ip(uint32_t ip, char* buf) {
    uint8_t* b = (uint8_t*)&ip;
    char tmp[32];
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t val = b[i];
        if (val >= 100) {
            tmp[idx++] = '0' + (val / 100); val %= 100;
            tmp[idx++] = '0' + (val / 10); val %= 10;
            tmp[idx++] = '0' + val;
        } else if (val >= 10) {
            tmp[idx++] = '0' + (val / 10); val %= 10;
            tmp[idx++] = '0' + val;
        } else {
            tmp[idx++] = '0' + val;
        }
        if (i < 3) tmp[idx++] = '.';
    }
    tmp[idx] = '\0';
    for (int i = 0; tmp[i] != '\0'; i++) buf[i] = tmp[i];
    buf[idx] = '\0';
}

static void cmd_netdiag(int argc, char** argv) {
    (void)argc; (void)argv;
#ifdef BWE_KERNEL
    extern void* netif_get_default(void);
    extern void* net_device_get_default(void);
    typedef struct {
        uint8_t  mac_addr[6];
        uint32_t ip_addr;
        uint32_t netmask;
        uint32_t gateway_ip;
        uint32_t dns_server;
        uint32_t dhcp_server;
        uint32_t lease_time;
        uint32_t t1_time;
        uint32_t t2_time;
        int      state;
        bool     link_up;
    } ShellNetIf;

    typedef struct {
        char     name[16];
        uint8_t  mac_addr[6];
        void*    pci_dev;
        uint16_t vendor_id;
        uint16_t device_id;
        uint32_t mmio_base;
        uint16_t io_base;
        bool     is_mmio;
        bool     link_up;
    } ShellNetDev;

    ShellNetIf* netif = (ShellNetIf*)netif_get_default();
    ShellNetDev* dev = (ShellNetDev*)net_device_get_default();

    shell_print("\n=== ATOMS OS NETWORK DIAGNOSTICS (NETDIAG) ===\n\n");
    shell_print("NIC Detected   : ");
    shell_print(dev ? "YES\n" : "NO\n");
    if (dev) {
        shell_print("Driver         : ");
        shell_print(dev->vendor_id == 0x8086 ? "Intel E1000 (0x8086)" : (dev->vendor_id == 0x10EC ? "Realtek R8168 (0x10EC)" : "GENERIC_NIC"));
        shell_print("\n");
        shell_print("Device Name    : "); shell_print(dev->name); shell_print("\n");
        shell_print("Link State     : "); shell_print(dev->link_up ? "UP\n" : "DOWN\n");
    }
    if (netif) {
        char ip_str[32], mask_str[32], gw_str[32], dns_str[32];
        format_shell_ip(netif->ip_addr, ip_str);
        format_shell_ip(netif->netmask, mask_str);
        format_shell_ip(netif->gateway_ip, gw_str);
        format_shell_ip(netif->dns_server, dns_str);
        shell_print("IP Address     : "); shell_print(ip_str); shell_print("\n");
        shell_print("Subnet Mask    : "); shell_print(mask_str); shell_print("\n");
        shell_print("Default Gateway: "); shell_print(gw_str); shell_print("\n");
        shell_print("DNS Server     : "); shell_print(dns_str); shell_print("\n");
        shell_print("NETIF State    : ");
        shell_print(netif->state == 2 ? "CONFIGURED\n" : "UNCONFIGURED\n");
    }
    shell_print("==============================================\n\n");
#else
    shell_print("Network diagnostics available via system service.\n");
#endif
}

void commands_sys_init(void) {
    command_register("help", cmd_help, "Show this help message", "System");
    command_register("ver", cmd_ver, "Show OS version", "System");
    command_register("about", cmd_about, "About Signatures OS", "System");
    command_register("cls", cmd_cls, "Clear the screen", "System");
    command_register("echo", cmd_echo, "Print text to screen", "System");
    command_register("time", cmd_time, "Show current time", "System");
    command_register("date", cmd_date, "Show current date", "System");
    command_register("test", cmd_test, "Run Validation & Stress Test Framework", "System");
    command_register("minbrow", cmd_minbrow, "Launch Minimal Real-Web Browser Probe", "Diagnostic");
    command_register("probe", cmd_minbrow, "Launch Real-Web Isolation Probe", "Diagnostic");
    command_register("netdiag", cmd_netdiag, "Show Network Interface Diagnostics", "Diagnostic");
}

