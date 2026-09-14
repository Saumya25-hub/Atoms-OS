# ATOMS OS — Freestanding libmpv Porting & System Assumptions Audit
**Document ID:** `docs/media/LIBMPV_PORTING.md`  
**Subsystem:** ATOMS Userspace Runtime & Freestanding Compatibility Matrix  
**Target:** Native libmpv / FFmpeg Freestanding Execution on BOS Userspace  
**Date:** September 12, 2026  

---

## 1. Porting Philosophy

ATOMS is a standalone, independent operating system with its own microkernel (`BOS`), window manager (`BWE`), surface engine (`BOSurface v2.5`), and VFS. It is **NOT** Linux, and it is **NOT** Windows.

Therefore:
- **No Wine or Emulation:** Do not emulate Win32 or Linux ABI.
- **No POSIX Bloat:** Do not port heavy glibc, systemd, or X11/Wayland dependencies.
- **Isolate via Embedding APIs:** Use `mpv`'s native client, render, and stream callback APIs so `libmpv` delegates I/O, rendering, timing, and memory directly to ATOMS primitives.

---

## 2. Host Operating System Assumptions Audit Matrix

| System Subsystem | Upstream Assumption | Action | ATOMS Adaptation & Architectural Rationale |
|:---|:---|:---:|:---|
| **Filesystem / File I/O** | POSIX `open()`, `read()`, `lseek()`, `close()`, `stat()` | **ADAPT** | Replaced entirely via `mpv_stream_cb_add_ro()`. Every file read/seek routes to ATOMS VFS syscalls (`SYS_OPEN`, `SYS_READ`, `SYS_SEEK`, `SYS_CLOSE`). No host POSIX filesystem assumed. |
| **Memory Allocation** | libc `malloc()`, `calloc()`, `realloc()`, `free()`, `posix_memalign()` | **KEEP / ADAPT** | Uses ATOMS userspace heap (`atoms/userspace/runtime/` with musl-derived allocator backed by `SYS_MMAP` / `SYS_BRK`). Tracking hooks capture per-playback-session allocation. |
| **Threading** | POSIX `pthread_create()`, `pthread_join()`, `pthread_detach()` | **ADAPT** | For single-process architecture, internal `libmpv` worker threads map to ATOMS user threads / worker loops (`SYS_CLONE` / `SYS_YIELD`). Main player operates non-blocking event pump. |
| **Mutex / Mutex Locks** | `pthread_mutex_t`, `pthread_rwlock_t` | **ADAPT** | Mapped to ATOMS lightweight spin-wait / atomic futex primitives (`atomic_flag`, atomic compare-exchange) in `atoms/userspace/runtime/`. |
| **Condition Variables** | `pthread_cond_wait()`, `pthread_cond_signal()` | **ADAPT** | Implemented using ATOMS event flags and monotonic tick waits (`SYS_SLEEP` / `SYS_YIELD`). |
| **Clocks & Timing** | `clock_gettime(CLOCK_MONOTONIC)` | **ADAPT** | Mapped to ATOMS `SYS_UPTIME` syscall providing microsecond monotonic hardware timer ticks. Master PTS synchronization derives directly from this clock. |
| **Terminal / Console I/O** | `isatty()`, `tcgetattr()`, ANSI escapes | **DISABLE** | Terminal CLI features are fully disabled (`-Dgpl=false`, `--disable-cplayer`). Log messages route to `bos_media_telemetry` and `SYS_WRITE`. |
| **Environment Variables** | `getenv()`, `setenv()` | **DISABLE / STUB** | `getenv()` returns `NULL` for standard unix paths (`HOME`, `XDG_*`, `PATH`). Configuration is passed directly via `mpv_set_property_string()`. |
| **Locale / Localization** | `setlocale()`, `nl_langinfo()`, `iconv` | **DISABLE** | Fixed to standard C / UTF-8 locale. Multilingual GUI is managed at the ATOMS application layer. |
| **Signals** | `sigaction()`, `SIGINT`, `SIGTERM`, `SIGPIPE` | **DISABLE** | Signal handling is disabled. Process termination is handled gracefully by BWE window close events (`BOS_GUI_EVENT_WINDOW_CLOSE`). |
| **Network & Sockets** | BSD sockets, `socket()`, `connect()`, `poll()` | **DISABLE (Initial)** | Network streaming is disabled for initial local media playback. Future streaming will integrate with ATOMS native TCP/IP stack (`kernel/net/`). |
| **Dynamic Loading** | `dlopen()`, `dlsym()`, `dlclose()` | **DISABLE** | Static compilation into `libbos_media.a` for initial boot; no runtime `.so` loading. Future migration to ATOMS `.sll` shared libraries. |
| **Video Presentation** | X11, Wayland, DRM/KMS, Win32 GDI/DirectX | **REPLACE** | Completely replaced with `mpv_render_context` using `MPV_RENDER_API_TYPE_SW`. Frames blit directly into `bos::Surface` ARGB32 memory buffers. |
| **Audio Output** | ALSA, PulseAudio, PipeWire, WASAPI, CoreAudio | **REPLACE** | Completely replaced with ATOMS BOS Audio bridge feeding `SYS_AUDIO_CALL` (Intel HDA / AC97 DMA ring). |
| **GPU Acceleration** | VAAPI, VDPAU, NVDEC (Linux driver), DXVA2 | **ADAPT** | Universal Video Acceleration HAL probes PCI bus (`0x10DE` RTX 4060). Negotiates HW acceleration if kernel driver is ready; otherwise automatically falls back to software decode. |

---

## 3. ABI Isolation & Symbol Management

To prevent upstream symbol pollution in ATOMS applications:
1. Only symbols prefixed with `bos_media_*` are exported in the public header `bos_media.h`.
2. All `mpv_*`, `av_*`, and `sws_*` symbols are isolated inside `libbos_media` static archive / `.sll`.
3. Application code (`userspace/apps/media_player/main.cpp`, file managers, shell) depends exclusively on `bos_media.h` and standard ATOMS SDK libraries (`libbos_ui_cpp.a`).
