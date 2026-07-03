# 07 - Bus Master Rotation Metrics

## Goal
Verify that the AC'97 hardware is physically processing chunks in the background and updating its registers asynchronously.

## Forensic Proof

**From qemu_verify.log:**
```
[ROTATION VERIFY]
Old CIV: 0
New CIV: 2
```

## Analysis
When the OS entered `ac97_playback_update()`, the `CIV` (Current Index Value) had advanced from 0 to 2.
This means the AC'97 hardware successfully drained Descriptor 0 and Descriptor 1 in the background without OS intervention.
This proves that Bus Master DMA is functioning exactly as defined in the Intel AC'97 specification.
