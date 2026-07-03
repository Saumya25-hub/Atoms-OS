# AC'97 Playback - Debugging & Risks

## Known Risks

### CPU Starvation
Currently, the playback engine utilizes polling. If the polling loop (e.g., in a system task) is starved of CPU time by a higher-priority task, the hardware will consume all prepared descriptors and reach `CIV == LVI`. At this point, it will under-run and halt. 
The driver handles this defensively by checking the `DCH` status bit. If it is set unexpectedly during `RUNNING`, it registers an "underrun" in the telemetry and automatically resets the RPBM bit to restart playback.

### Future IRQ Integration
Polling is inefficient because it requires constant CPU wake-ups to check the `CIV` register. In the future (Phase 7.7+), the `IOC` (Interrupt On Completion) bit will be set on specific descriptors (e.g., half-buffer and full-buffer). The driver will then sleep until the IRQ fires, saving significant CPU overhead.

## Debugging Workflow
If playback hangs or stutters:
1. Call `ac97_playback_status()` to dump telemetry.
2. If `Underruns` is rapidly increasing, the update loop is not being called frequently enough.
3. If `Bytes Sent` is stuck, the DMA `PO_CR` bit may have been cleared erroneously.
