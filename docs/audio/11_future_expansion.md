# ATOMS OS Foundation v2 Audio Subsystem
## 11: Future Expansion

The core architecture defined in Phase 7.1 is designed to scale. It explicitly decouples format decoding from playback, and hardware specifics from mixing logic, ensuring a long shelf-life for the OS audio stack.

### Future Format Support
Because the kernel only deals with raw PCM data, adding new formats requires zero kernel modifications.
*   **OGG / FLAC**: Can be added entirely as userspace static libraries (`lib/media/ogg_decoder.c`) that apps link against.
*   **MIDI**: A future MIDI synthesizer can be built as a userspace daemon that interprets MIDI commands, renders them to PCM using a SoundFont, and streams the PCM to the Horse Engine.

### Video Playback Integration
Future video players will require perfect Audio-Video (A/V) sync.
*   The `AudioStream` API will be expanded to support timestamp querying. Video engines can ask the Audio Subsystem "Exactly how many PCM frames have been played out of the speakers?" and adjust the video frame presentation to match the audio clock.

### Advanced Hardware Standards
The driver interface (`audio_driver_ops`) is intentionally generic.
*   **Intel HDA (High Definition Audio)**: The most common modern standard. The HDA driver will simply implement the ops and use DMA ring buffers exactly like a legacy AC97 or SB16 card.
*   **USB Audio**: Handled by a future USB driver stack. When a USB DAC is plugged in, the USB subsystem will instantiate an `audio_driver_ops` struct and register it with the `Audio Device Manager` dynamically.
*   **Bluetooth Audio (A2DP)**: Will appear to the Audio Core as just another driver. The Bluetooth stack will take the mixed PCM output, compress it to SBC/AAC, and transmit it wirelessly.
