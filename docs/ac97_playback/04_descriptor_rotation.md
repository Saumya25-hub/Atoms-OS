# AC'97 Playback - Descriptor Rotation

Managing the 32 descriptors requires exact alignment between the software tracking index (`g_last_civ`) and the hardware index (`CIV`).

## Rotation Algorithm
1. The software reads the hardware's `PO_CIV`.
2. If `g_last_civ != CIV`, it means the hardware has progressed.
3. The software iterates from `g_last_civ` up to `CIV - 1` (wrapping at 31).
4. For every descriptor in that range, the physical memory block associated with that descriptor is overwritten with fresh PCM data from the mixer.
5. The software then updates `PO_LVI` to point just behind the current `CIV`.

## Ownership
- **Software Owns**: Descriptors from `CIV` up to `LVI`.
- **Hardware Owns**: The descriptor at `CIV`.

By strictly adhering to this rotation, the software never overwrites memory that the hardware is actively streaming to the codec, preventing audible tearing and static.
