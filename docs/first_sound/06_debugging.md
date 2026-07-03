# First Sound - Debugging

If the tone generator stops producing sound, fails to output entirely, or glitches:

1. **Verify PCM Math**: Ensure `phase_accum` is passed as a pointer. If the phase resets to 0 every chunk, it will create severe harmonic clicking.
2. **Verify Stream Capacity**: `audio_stream_available` and `audio_stream_capacity` dictates how much data to push. Pushing more data than `capacity` will cause `audio_stream_write` to overwrite unmixed data.
3. **Verify LVI**: If `PO_CIV == PO_LVI`, the hardware will halt. Check if the `ac97_playback_update` loop is running frequently enough to keep `PO_LVI` pushed ahead.
