# PATCH PLAN: E1000 NET_DEVICE REGISTRATION, BOOT TELEMETRY & NETDIAG

## 1. What to Modify

### A. `kernel/drivers/net/e1000/e1000.c`
- Implement `g_e1000_netdev` struct and operations (`e1000_netdev_xmit`, `e1000_netdev_poll`).
- Register `g_e1000_netdev` via `net_device_register()` during `e1000_init()`.
- Add static NAT fallback if DHCP times out:
  - IP: `192.168.2.100`
  - Subnet: `255.255.255.0`
  - Gateway: `192.168.2.1`
  - DNS: `8.8.8.8` (and `1.1.1.1`)
  - State: `NETIF_STATE_CONFIGURED`
- Add pointer identity telemetry: `[NET][TRACE] driver_netif=... dhcp_netif=... dns_netif=...`

### B. `kernel/kernel.c`
- Add granular PCI network controller enumeration and boot telemetry:
  - `[NET][PCI] enumerating network controllers`
  - `[NET][PCI] vendor=0x... device=0x... class=0x...`
  - `[NET][DRIVER] selected=e1000`
  - `[NET][DRIVER] init_start`
  - `[NET][DRIVER] init_success`
  - `[NET][LINK] state=UP`
  - `[NET][CONFIG] state=CONFIGURED`

### C. `userspace/shell/commands_sys.c`
- Implement `cmd_netdiag` and register `netdiag` in `commands_sys_init()`.
- Output: NIC detected, Driver, PCI BDF, MAC address, Link state, IP address, Subnet, Gateway, DNS, and Netif state.

## 2. Why
- Fixes the root cause of blocked packet transmission in Intel E1000 virtual NICs.
- Guarantees `netif` is configured and available for ARP, DNS, TCP, and HTTPS traffic.
- Exposes deterministic `netdiag` command to verify networking independently before running `minbrow`.

## 3. Expected Result
- Zero `DNS_FAILURE reason=NETIF_NOT_CONFIGURED` errors.
- Working DHCP / DNS / TCP / TLS pipeline in VMware Workstation.
