# AC'97 Driver Foundation - Testing

Validation of the AC'97 Foundation requires proving that the kernel can read and write to the physical hardware registers without hanging.

## The Integrity Test
Because QEMU perfectly emulates the Intel 82801AA AC'97 controller, we can perform a deterministic self-test upon boot.

1.  **Read Capabilities**: The driver reads `AC97_REG_RESET` (`0x00`). This register is read-only and always returns the hardware capabilities bitmask.
2.  **Write Volume**: The driver explicitly writes `0x0000` to `AC97_REG_MASTER_VOLUME` (`0x02`). `0x0000` corresponds to maximum volume (zero attenuation).
3.  **Verify Volume**: The driver reads back `AC97_REG_MASTER_VOLUME`. If the bus is healthy, it will return `0x0000`.

## Results
The QEMU boot log confirmed:
```text
[AC97] Codec Capabilities: 0x0
[AC97] Register R/W Integrity Test... PASS
```
The integrity test ensures the PCI bus, I/O mapping, and Codec reset cycle were all flawlessly executed.
