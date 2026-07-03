# AC'97 Playback - Pipeline

The audio data flows through a strict, linear pipeline ensuring zero data loss and safe synchronization.

## Flow
1. **Audio Streams**: Independent streams (from the future OS features like UI, Doom, etc.) supply raw PCM to the Audio Core.
2. **Software Mixer**: `audio_mixer_process()` combines all active streams into a single mixed output buffer.
3. **Playback Engine**: The `ac97_playback` engine intercepts the mixed PCM and injects it directly into the physical memory allocated for the DMA controller.
4. **DMA Engine**: The hardware reads the physical Buffer Descriptor List (BDL) and pulls the raw bits over the PCI bus.
5. **AC'97 Codec**: The codec receives the digital PCM data and converts it into analog signals for the speakers.

## Rules
- The Playback Engine **never** touches individual Audio Streams.
- The Playback Engine **always** reads from the Software Mixer.
