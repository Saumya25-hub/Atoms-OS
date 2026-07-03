# AC'97 Driver Foundation - PCI Detection

ATOMS OS uses a lightweight PCI brute-force scan to identify the AC'97 controller during boot.

## The Scanner
Because ATOMS OS Foundation v2 does not yet have a generalized PCI tree traversal driver, the AC'97 driver implements a targeted scan using the standard PCI Configuration Space I/O Ports:
- **0xCF8** (Configuration Address Port)
- **0xCFC** (Configuration Data Port)

## Identification
The scanner checks every combination of Bus (0-255) and Slot (0-31). It looks for:
- A valid Vendor/Device ID.
- Base Class `0x04` (Multimedia).
- Sub Class `0x01` (Audio Controller).

When discovered (e.g., QEMU's simulated Intel 82801AA with ID `0x24158086`), the driver extracts:
1. **NAM BAR** (Native Audio Mixer - BAR0).
2. **NABM BAR** (Native Audio Bus Master - BAR1).
3. **IRQ Line**.

## I/O Activation
Crucially, after detection, the driver patches the PCI Command Register (offset `0x04`) to explicitly enable Bus Mastering (`0x04`) and I/O Space access (`0x01`). If this is skipped, all subsequent codec communication fails silently.
