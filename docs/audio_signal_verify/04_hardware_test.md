# 04 - Hardware Subsystem Foundation

## Goal
Verify that the OS successfully probes, identifies, and activates the physical AC'97 hardware components via PCI.

## Forensic Proof

**From qemu_verify.log:**
```
[AC97] Scanning PCI for Audio Controller...
[AC97] Found Audio Controller. Vendor/Device: 0x0x24158086
[AC97] NAM BAR: 0x0xC000, NABM BAR: 0x0xC400, IRQ: 11
[AC97] Performing Cold Reset...
[AC97] Codec Ready (PCR flag set).
[AC97] Codec Capabilities: 0x0x0
[AC97] Register R/W Integrity Test... PASS
[AC97] Power Status: 0x0xF
[AC97] SUCCESS: Hardware Foundation Established.
```

## Analysis
The PCI subsystem identified the Intel 82801AA AC'97 Audio Controller (`0x24158086`).
The Native Audio Mixer (NAM) and Native Audio Bus Master (NABM) base IO addresses were successfully retrieved.
The hardware cold reset worked, the Codec declared itself ready, and the R/W integrity test over I/O ports passed.
This proves total driver dominance over the AC'97 controller state.
