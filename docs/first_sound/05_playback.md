# First Sound - Playback Lifecycle

Because we have not yet established background kernel threads for audio, the playback lifecycle for this test is executed as a dedicated blocking loop during boot.

## The Loop
The loop calculates the required frames for a 10-minute tone (28.8 million frames). It checks the available capacity in the Audio Stream, fills the available capacity with the sine wave, and immediately calls `ac97_playback_update()` to advance the DMA buffer. 

Once the target frame count is reached, it safely invokes `ac97_playback_stop()` and `ac97_playback_shutdown()`, halting the hardware deterministically.
