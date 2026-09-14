# libmpv for ATOMS OS
**Subsystem:** `third_party/media/mpv/`  
**Upstream Project:** mpv media player  
**Official Repository:** `https://github.com/mpv-player/mpv`  
**Target Version:** v0.38.0  
**Target Commit:** `2b3f1a8c9e46a784d59fcf533b66e3957cecb8ff`  
**Original License:** GPLv2+ (default) / LGPLv2.1+ (opt-in via `-Dgpl=false`)  
**Configured License:** **LGPLv2.1+**  

---

## 1. Files Included
- `include/mpv/client.h`: Official C Client API for embedding
- `include/mpv/render.h`: Render API (SW and HW blit contexts)
- `include/mpv/stream_cb.h`: Custom Stream Callback API for OS VFS integration
- `LICENSE`: GNU Lesser General Public License version 2.1
- `COPYRIGHT`: Official upstream copyright notice
- `README.ATOMS.md`: This supply chain and build manifest

---

## 2. ATOMS Modifications & Integration Strategy
1. **Embedding Isolation:** ATOMS does not invoke any CLI binary or process. Only the embedding client API is used.
2. **Stream Bridge:** All file I/O routes through `mpv_stream_cb_add_ro()` which calls ATOMS VFS syscalls (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`), eliminating POSIX file dependencies.
3. **Software Render Context:** Video frames are rendered via `MPV_RENDER_API_TYPE_SW` directly into `bos::Surface` ARGB32 memory.
4. **Audio Routing:** Audio output routes through the ATOMS Audio HAL via `SYS_AUDIO_CALL`.

---

## 3. Excluded GPL Files & Prohibited Components
Passing `-Dgpl=false` to mpv configuration strictly excludes:
- `filters/f_auto_pullup.c`
- `filters/f_hwupload.c` (GPL parts)
- `video/decode/vd_lavc.c` (GPL fallback hooks)
- `sub/lavc_conv.c` (GPL subtitle format hacks)
- All CLI binaries, man pages, and shell tools.

---

## 4. Redistribution & Freedom Requirements
- Source code modifications must remain available under LGPLv2.1+.
- Reverse engineering and relinking with replacement `libmpv` objects must be permitted.
- Zero commercial or royalty-bearing SDKs may be introduced.
