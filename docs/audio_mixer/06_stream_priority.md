# Software Mixer Engine - Stream Priority

As requested by the architecture outline, Phase 7.4 introduces the `priority` field inside the `AudioStream` registry.

## Future Expansion
Currently, the Mixer processes all registered streams without priority reordering, as integer summation is strictly commutative (`A + B = B + A`).

However, as the ATOMS OS Audio System evolves, `priority` (0-255) will become critical for two major future features:
1.  **Stream Culling**: If the system detects performance starvation or hits the hard limit of maximum active streams (`MAX_MIX_STREAMS`), it will automatically pause or destroy streams with the lowest priority (e.g. background UI hums) to preserve audio fidelity for High-Priority streams (e.g. VoIP or Game effects).
2.  **Sidechain Compression (Audio Ducking)**: High priority notification alerts can automatically attenuate the volume of lower-priority background music.

Default streams are initialized with Priority `128` (Normal).
