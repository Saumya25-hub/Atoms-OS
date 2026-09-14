# Chapter 20: Native Applications & Roadmap

## 1. Current Native Applications
- **DOOM Port**: Classical DOOM game engine ported directly to native ATOMS OS userspace (`userspace/apps/doom/`), utilizing custom software rendering and keyboard event hooks.
- **Native Media Player**: Hardware-accelerated audio/video playback engine (`userspace/apps/media_player/`) integrating FFmpeg and libmpv audio codecs for real-time MP3/WAV playback.
- **Hardware Certification Dashboards**: Graphical diagnostic dashboards (`apal_dashboard`, `runtime_dashboard`, `process_ipc_cert`) reporting real-time system metrics.

## 2. Experimental: ATRIX Browser Engine
- Toolchain integration: Chromium GN and Ninja build configurations in `tools/gn.exe` and `tools/ninja.exe`.
- Blink DOM parser and Google V8 JavaScript runtime compiled as host and guest prototypes.

## 3. Engineering Roadmap
1. Complete Ring 3 userspace process teardown and atomic PML4 page reclamation.
2. Enhance dynamic `.sll` shared library lazy binding.
3. Advance ATRIX browser engine from prototype probe to full userspace web renderer.
4. Expand physical bare-metal hardware certification to additional x86_64 chipsets and discrete GPUs.
