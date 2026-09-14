# ATOMS OS — APAL Platform Subsystem Matrix

**Document Version**: 1.0.0  
**Status**: `VERIFIED & OPERATIONAL`  
**Date**: September 7, 2026  

---

## 1. Platform Subsystem Matrix

| Chromium Requirement | ATOMS Existing Capability | APAL Adapter | Status | Concrete Evidence in Repo |
| :--- | :--- | :--- | :---: | :--- |
| **Memory Allocation (PartitionAlloc & V8)** | `sys_service_mmap()`, `sys_service_mprotect()`, `sys_service_munmap()` | `atoms/userspace/apal/memory/apal_memory.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/runtime/include/atoms_syscall.h:149`](file:///d:/Signatures_OS/atoms/userspace/runtime/include/atoms_syscall.h#L149) |
| **Multi-Threading (`base::PlatformThread`)** | `sys_service_thread_spawn()`, `sys_service_thread_exit()` | `atoms/userspace/apal/threads/apal_thread.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/runtime/pthread/atoms_pthread.c:66`](file:///d:/Signatures_OS/atoms/userspace/runtime/pthread/atoms_pthread.c#L66) |
| **Synchronization (`base::Lock`, `CondVar`)**| Hardware CPU `LOCK CMPXCHG`, `SYS_FUTEX` (Wait/Wake) | `atoms/userspace/apal/threads/apal_thread.c` | 🟢 **OPERATIONAL** | [`kernel/core/syscall/src/services.c:599`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c#L599) |
| **Process Model (`base::LaunchProcess`)** | `SYS_EXEC` (Syscall 37), `SYS_GETPID`, `SYS_EXIT` | `atoms/userspace/apal/process/apal_process.c` | 🟢 **OPERATIONAL** | [`kernel/core/syscall/src/services.c:710`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c#L710) |
| **IPC / Message Passing (Mojo Core)** | In-memory atomic FIFO ring channels | `atoms/userspace/apal/ipc/apal_ipc.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/apal/ipc/apal_ipc.c:45`](file:///d:/Signatures_OS/atoms/userspace/apal/ipc/apal_ipc.c#L45) |
| **Shared Memory (`PlatformSharedMemoryRegion`)**| Anonymous `mmap` regions with ref-counting | `atoms/userspace/apal/shared_memory/apal_shm.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/apal/shared_memory/apal_shm.c:28`](file:///d:/Signatures_OS/atoms/userspace/apal/shared_memory/apal_shm.c#L28) |
| **Filesystem (`base::File`, `file_util`)** | Native BOFS VFS (Syscalls 14, 15, 25, 26, 29, 36) | `atoms/userspace/apal/filesystem/apal_fs.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/runtime/syscall/atoms_syscall_adapter.c:48`](file:///d:/Signatures_OS/atoms/userspace/runtime/syscall/atoms_syscall_adapter.c#L48) |
| **Sockets & Network (`net::StreamSocket`)** | In-kernel TCP/UDP stack & RTL8168/E1000 drivers | `atoms/userspace/apal/sockets/apal_socket.c` | 🟢 **OPERATIONAL** | [`kernel/net/tcp/tcp.c:1-250`](file:///d:/Signatures_OS/kernel/net/tcp/tcp.c#L1-L250) |
| **Time & Clocks (`base::TimeTicks`)** | `SYS_CLOCK_GETTIME` (Monotonic & Realtime) | `atoms/userspace/apal/time/apal_time.c` | 🟢 **OPERATIONAL** | [`kernel/core/syscall/src/services.c:622`](file:///d:/Signatures_OS/kernel/core/syscall/src/services.c#L622) |
| **Cryptographic Entropy (`base::RandBytes`)** | x86_64 Hardware `RDRAND` & kernel crypto PRNG | `atoms/userspace/apal/randomness/apal_rand.c` | 🟢 **OPERATIONAL** | [`atoms/userspace/apal/randomness/apal_rand.c:14`](file:///d:/Signatures_OS/atoms/userspace/apal/randomness/apal_rand.c#L14) |
| **Graphics Surface (Viz / Skia Canvas)** | BWE Window Surfaces, BCM Compositor (Syscall 16, 20) | `atoms/userspace/apal/graphics/apal_surface.c` | 🟢 **OPERATIONAL** | [`kernel/wm/bwe/src/bwe.c`](file:///d:/Signatures_OS/kernel/wm/bwe/src/bwe.c) |
| **Input & Event Translation (`ui::Event`)** | `SYS_GUI_POLL_EVENT` (Syscall 22), `BOS_GUIEvent` | `atoms/userspace/apal/input/apal_input.c` | 🟢 **OPERATIONAL** | [`kernel/core/syscall/include/syscall.h:123`](file:///d:/Signatures_OS/kernel/core/syscall/include/syscall.h#L123) |
| **Audio Output (`media::AudioOutputStream`)**| AC'97 / Intel HDA DMA ring & PCM mixer | `atoms/userspace/apal/audio/apal_audio.c` | 🟢 **OPERATIONAL** | [`kernel/audio/formats/audio_pcm.c:1-120`](file:///d:/Signatures_OS/kernel/audio/formats/audio_pcm.c#L1-L120) |
| **Dynamic Loading (`ScopedNativeLibrary`)** | Native `.sll` shared libraries & ELF module loader | `atoms/userspace/apal/loader/apal_loader.c` | 🟢 **OPERATIONAL** | [`kernel/loader/`](file:///d:/Signatures_OS/kernel/loader/) |
| **User Fault Boundary (V8 Stack Guard #PF)**| Ring 3 `#PF` trap in IDT without kernel panic | `atoms/userspace/apal/exceptions/apal_exception.c` | 🟢 **OPERATIONAL** | [`kernel/core/interrupt/src/idt.c`](file:///d:/Signatures_OS/kernel/core/interrupt/src/idt.c) |

---

## 2. Summary
All 15 target architectural domains are bridged natively via unprivileged Ring 3 adapters in `libapal.a`, directly utilizing existing certified kernel and userspace runtime capabilities.
