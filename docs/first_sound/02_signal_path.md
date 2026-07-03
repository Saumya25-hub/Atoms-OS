# First Sound - Signal Path

The signal path for the First Sound validation runs continuously without filesystem dependencies.

## The Path
1. **Mathematical Generator**: `audio_pcm_generate_sine()` produces a 440Hz 16-bit stereo sine wave.
2. **Audio Stream**: The sine wave is packed into an `AudioPcmPacket` and written to a dedicated `AudioStream` via the Audio API.
3. **Software Mixer**: `audio_mixer_process()` consumes the stream's ring buffer and mixes it (applying volume) into the physical DMA buffer chunks.
4. **DMA Engine**: The AC'97 hardware Bus Master reads the physical buffer over the PCI bus and transmits it to the Codec.

This entire pipeline operates exclusively in RAM, ensuring zero latency and zero dependency on the VFS.
