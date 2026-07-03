# AC'97 Driver Foundation - Initialization

The `ac97_init()` function orchestrates the exact sequence required to bring the hardware online.

## Step 1: PCI Discovery
The PCI bus is scanned for the Audio Controller class code.
Upon discovery, the two Base Address Registers (BARs) are read. The driver verifies that the LSB of both BARs is `1`, proving they are mapped into I/O Space (as required by the AC'97 specification).

## Step 2: Bus Master Activation
The PCI Command Register (`0x04`) is modified to enable `0x01` (I/O Space) and `0x04` (Bus Mastering). Without Bus Mastering, future DMA phases will silently fail to fetch memory.

## Step 3: Base Registration
The stripped I/O port addresses are handed to the `ac97_codec` layer via `ac97_codec_init_base()`.

## Step 4: Hardware Reset
A `Cold Reset` is requested. The driver halts execution in a `pause` loop while waiting for the `Primary Codec Ready` bit to assert in the Global Status register.

## Step 5: Self-Test
The driver runs a read/write integrity check on the volume registers and dumps the power-state telemetry to the kernel log.
