# ATOMS OS — BCM Phase 7: Frame Completion & Synchronization Architecture

## 1. Executive Summary

BCM Phase 7 implements a robust **Frame Completion, Synchronization, and Reliability** protocol. It provides monotonic frame identity, definitive completion detection, resource retirement tracking, bounded presentation timeout recovery, and protection against double/stale presentation attempts.

---

## 2. Monotonic Frame Identity & Lifecycle

Every composed frame is assigned a unique, monotonically increasing 64-bit identifier `frame_id`:

```text
[FRAME CREATED]  (frame_id = ++g_bcm_state.current_frame_id)
       │
       ▼
[FRAME COMPOSED] (RAM framebuffer ready)
       │
       ▼
[FRAME SUBMITTED] (Submitted to AGDTE / BSPE)
       │
       ▼
[FRAME IN-FLIGHT] (in_flight_frame_id = frame_id, presentation_start_tick recorded)
       │
       ▼
[FRAME COMPLETED] (VRAM PCIe MMIO transfer verified complete)
       │
       ▼
[FRAME RETIRED]  (last_completed_frame_id = frame_id, in_flight_frame_id = 0)
       │
       ▼
[RESOURCES SAFE FOR REUSE]
```

---

## 3. Synchronization & Deterministic Contracts

### A. Double-Presentation Guard
If `BCM_BeginPresentation(id)` is called while a frame is already in flight (`is_presenting == true` or `in_flight_frame_id != 0`), the request is rejected with `BCM_ERR_BUSY`, incrementing `telemetry.dropped_presentations`.

### B. Stale / Unknown Completion Rejection
When `BCM_CompletePresentation(id, status)` is called:
- If `id != g_bcm_state.in_flight_frame_id`, the call is safely rejected as a stale or duplicate completion without state corruption.
- If `id == g_bcm_state.in_flight_frame_id`, the frame is formally retired:
  - `g_bcm_state.last_completed_frame_id = id`
  - `g_bcm_state.in_flight_frame_id = 0`
  - `g_bcm_state.is_presenting = false`
  - `g_bcm_state.telemetry.frames_presented++`

### C. Bounded Presentation Timeout Protection
To prevent hardware lockups (e.g. PCIe bus hang or stuck MMIO) from hanging the compositor thread:
- `presentation_start_tick` is recorded upon entry to presentation.
- If `(now - presentation_start_tick) > timeout_ms` (default 50 ms):
  - BCM increments `telemetry.presentation_timeouts`.
  - BCM performs non-panicking recovery: forces frame retirement, resets in-flight lock, requests full repaint, and restores `BCM_STATE_IDLE`.
  - The system remains fully responsive.
