# FORENSIC REPORT: ATOMS OS NETWORK INITIALIZATION & NETIF CONFIGURED BUG

## 1. Executive Summary & Root Cause
During runtime trace, MINBROW repeatedly reported:
`[MINBROW][NET] DNS_FAILURE reason=NETIF_NOT_CONFIGURED`
Followed by a VM CPU shutdown.

Forensic investigation revealed two concrete defects:
1. **Missing `net_device` Registration for Intel E1000**:
   - `ethernet_send()` in `kernel/net/ethernet/ethernet.c` routes all raw Ethernet frames to `debuglan_send_raw()`, which relies on `net_device_get_default()->ops.xmit()`.
   - `r8168.c` registered `g_r8168_netdev` with `net_device_register()`, but `e1000.c` NEVER registered a `net_device_t`.
   - As a result, when running in VMware Workstation or QEMU with an Intel E1000 virtual NIC, `net_device_get_default()` returned `NULL`, and `ethernet_send()` failed on every packet transmission (`TX DMA` was never queued).
2. **Conditional DHCP Fallback**:
   - In `e1000_init()`, `netif_set_config()` was only called inside `if (dora_ok)`.
   - When DHCP failed (or timed out due to blocked TX), `netif->state` remained `NETIF_STATE_UNCONFIGURED` permanently with IP `0.0.0.0`.
3. **Missing Pre-Browser Network Diagnostic (`netdiag`)**:
   - No dedicated userspace/shell diagnostic tool existed to inspect NIC detection, MAC address, link state, and IP configuration prior to launching the browser.

## 2. Suspected Fix (No Code)
- In `kernel/drivers/net/e1000/e1000.c`:
  - Define `g_e1000_netdev` with `xmit` and `poll_rx` operations and register it via `net_device_register()`.
  - Add fallback static NAT configuration (`192.168.2.100` / gateway `192.168.2.1` / DNS `8.8.8.8`) if DHCP does not receive an offer.
- In `kernel/kernel.c`:
  - Add granular boot telemetry: `[NET][PCI]`, `[NET][DRIVER]`, `[NET][LINK]`, `[NET][DHCP]`, `[NET][CONFIG]`.
- In `userspace/shell/commands_sys.c`:
  - Implement `netdiag` command to expose full network status.
