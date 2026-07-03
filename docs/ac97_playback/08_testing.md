# AC'97 Playback - Testing

Validation of the Playback Engine in Phase 7.6.2 is based on an extreme state-churn test designed to break the hardware state machine.

## The Stress Test
The driver implements a 10,000-iteration loop:
1. `ac97_playback_prepare()`: Allocates memory.
2. `ac97_playback_start()`: Fills the buffer with mixer data and triggers the DMA run bit.
3. `ac97_playback_update()`: Simulates polling iterations.
4. `ac97_playback_stop()`: Waits for the DMA to halt.
5. `ac97_playback_shutdown()`: Frees the memory.

## Results
The test successfully ran 10,000 cycles. It pushed over 640 Megabytes of mixed PCM data through the ATOMS Virtual Memory Manager and AC'97 Bus Master without a single memory leak, register corruption, or underrun loop hang. The state machine transitions successfully protected the hardware from getting locked into a bad state.
