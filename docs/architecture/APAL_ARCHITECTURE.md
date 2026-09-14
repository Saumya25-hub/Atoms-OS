# ATOMS OS — APAL Architecture Specification
## ATOMS Platform Adaptation Layer (APAL)

**Document Version**: 1.0.0  
**Target Platform**: ATOMS OS (x86_64, BOS Microkernel, BOFS VFS)  
**Consumer**: Upstream Chromium Source Tree (`third_party/chromium/src/`)  
**Execution Context**: Ring 3 Userspace (CPL=3, Unprivileged)  

---

## 1. Architectural Mission & Boundary

The **ATOMS Platform Adaptation Layer (APAL)** provides a clean, native Ring 3 adaptation boundary enabling the real upstream Chromium browser engine to interface directly with ATOMS OS without rewriting browser subsystems, without creating duplicate OS components, and without masquerading as Linux.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        UPSTREAM CHROMIUM                               │
│        (base/, mojo/, net/, ui/, media/, v8/, content/, blink/)        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│             ATOMS PLATFORM ADAPTATION LAYER (APAL)                     │
│  ┌──────────────┬──────────────┬──────────────┬─────────────────────┐  │
│  │ Memory       │ Threads/Sync │ Process      │ IPC / Mojo Pipes    │  │
│  ├──────────────┼──────────────┼──────────────┼─────────────────────┤  │
│  │ Shared Mem   │ Filesystem   │ Sockets      │ Time & Clocks       │  │
│  ├──────────────┼──────────────┼──────────────┼─────────────────────┤  │
│  │ Random/RDRAND│ Graphics/BWE │ Input / HID  │ Audio PCM           │  │
│  ├──────────────┼──────────────┼──────────────┼─────────────────────┤  │
│  │ Module Loader│ User Faults  │ System Info  │ Security / Entropy  │  │
│  └──────────────┴──────────────┴──────────────┴─────────────────────┘  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│             EXISTING ATOMS USERSPACE RUNTIME & STATIC LIBS             │
│   libatoms_c.a (musl libc)  │  libatoms_cpp.a (LLVM libc++abi)         │
│   atoms_heap.c (Arenas)     │  atoms_pthread.c (Stacks & Joins)        │
│   atoms_tls.c (FS/GS MSRs)  │  atoms_syscall_adapter.c (Direct Wire)   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│              ATOMS FAST HARDWARE SYSCALL GATEWAY (LSTAR)               │
│   Syscalls 1..37: mmap, munmap, mprotect, futex, clock_gettime,        │
│   open, read, write, seek, stat, thread_spawn, gui_create_window, etc.  │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│                        BOS KERNEL & HARDWARE                           │
│   PMM / VMM / Scheduler / RTL8168 / Intel E1000 / AC'97 / XHCI / BOFS  │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Reuse-First Architectural Policy

In compliance with the **Reuse-First Directive**, APAL strictly adapts existing ATOMS facilities rather than re-implementing them:

| Subsystem | Upstream Abstraction | Reused ATOMS Facility | APAL Adaptation Approach |
| :--- | :--- | :--- | :--- |
| **Virtual Memory** | `base::PageAllocator`, V8 | `sys_service_mmap`, `mprotect` | `apal_memory`: Wraps `MAP_ANONYMOUS` with 4KB alignment and W^X support. |
| **Threading** | `base::PlatformThread` | `sys_service_thread_spawn` | `apal_thread`: 16-byte stack frame alignment and `pthread_create` bridge. |
| **Synchronization** | `base::Lock`, `ConditionVariable` | Hardware atomics & `SYS_FUTEX` | `apal_thread`: Spin-acquire with futex sleep wait queue and wake cascade. |
| **Process Model** | `base::LaunchProcess` | `SYS_EXEC` (Syscall 37) | `apal_process`: Direct invocation with argv/envp arrays; no `fork()` overhead. |
| **Filesystem** | `base::File`, `file_util.h` | BOFS / VFS (Syscalls 14, 15, 29, 36) | `apal_fs`: POSIX descriptor map to native BOFS inodes and block buffers. |
| **IPC / Message Pipe** | Mojo Core platform layer | Circular memory channels | `apal_ipc`: Handle-based packet endpoints with lock-free atomic rings. |
| **Shared Memory** | `PlatformSharedMemoryRegion` | Anonymous `mmap` regions | `apal_shm`: Ref-counted memory descriptor mappings with readonly isolation. |
| **Networking** | `net::StreamSocket` | Kernel TCP/UDP stack | `apal_socket`: Standard socket API mapping to kernel network endpoints. |
| **Time & Clocks** | `base::TimeTicks`, `base::Time` | `SYS_CLOCK_GETTIME` | `apal_time`: Nanosecond monotonic ticks and microsecond wall-clock queries. |
| **Entropy** | `base::RandBytes` | x86_64 `RDRAND` instruction | `apal_rand`: Hardware CPU entropy with XorShift128+ cryptographic fallback. |
| **Graphics** | Chromium Viz, Skia surface | BWE Compositor (Syscall 16, 20, 21) | `apal_surface`: 32-bpp BGRA direct framebuffer surface mapping and invalidation. |
| **Input / HID** | `ui::Event` | `SYS_GUI_POLL_EVENT` (Syscall 22) | `apal_input`: Maps `BOS_GUIEvent` keycodes, modifiers, and mouse delta to DOM events. |
| **Audio** | `media::AudioOutputStream` | AC'97 / HDA PCM engine | `apal_audio`: Queues 48kHz 16-bit stereo PCM chunk buffers directly to hardware. |
| **Module Loading** | `base::ScopedNativeLibrary` | ATOMS `.sll` / ELF loader | `apal_loader`: Dynamic module symbol resolution and relocation linking. |
| **Fault Boundary** | V8 stack guard page `#PF` | Ring 3 IDT exception trap | `apal_exception`: Intercepts user guard page touches cleanly without kernel panic. |

---

## 3. Directory & Module Organization

All adaptation layer source code is permanently hosted within `atoms/userspace/apal/`:

```
atoms/userspace/apal/
├── include/
│   ├── apal.h                # Master include umbrella
│   └── apal_types.h          # Standard types, error codes, handle descriptors
├── memory/
│   ├── apal_memory.h         # Page allocator & W^X interface
│   └── apal_memory.c         # sys_mmap/munmap/mprotect bindings
├── threads/
│   ├── apal_thread.h         # Thread & sync primitives
│   └── apal_thread.c         # sys_thread_spawn & futex bindings
├── process/
│   ├── apal_process.h        # Process lifecycle & arguments
│   └── apal_process.c        # sys_exec bindings
├── ipc/
│   ├── apal_ipc.h            # Mojo-compatible message pipes
│   └── apal_ipc.c            # Channel queue implementation
├── shared_memory/
│   ├── apal_shm.h            # Shared memory region descriptors
│   └── apal_shm.c            # Anonymous shared mapping manager
├── filesystem/
│   ├── apal_fs.h             # File I/O & directory management
│   └── apal_fs.c             # BOFS/VFS file syscall bindings
├── sockets/
│   ├── apal_socket.h         # Socket transport endpoints
│   └── apal_socket.c         # Non-blocking network streaming
├── time/
│   ├── apal_time.h           # Monotonic & realtime clock queries
│   └── apal_time.c           # clock_gettime & nanosleep bindings
├── randomness/
│   ├── apal_rand.h           # Cryptographic entropy source
│   └── apal_rand.c           # Hardware RDRAND & PRNG generator
├── graphics/
│   ├── apal_surface.h        # BWE window presentation
│   └── apal_surface.c        # 32-bpp BGRA frame mapping & blit
├── input/
│   ├── apal_input.h          # Input events & DOM keys
│   └── apal_input.c          # BOS_GUIEvent translation
├── audio/
│   ├── apal_audio.h          # PCM stream output
│   └── apal_audio.c          # Audio buffer queueing
├── loader/
│   ├── apal_loader.h         # Module & .sll loading
│   └── apal_loader.c         # Dynamic symbol resolver
├── exceptions/
│   ├── apal_exception.h      # User-mode fault containment
│   └── apal_exception.c      # Guard page boundary validator
├── apal.c                    # Master lifecycle (init/shutdown)
├── build_apal.py             # Library build script -> libapal.a
└── libapal.a                 # Static archive for Chromium linking
```

---

## 4. Upstream Chromium GN Integration

Upstream Chromium sources in `third_party/chromium/src/` target ATOMS OS cleanly via:
- `build/config/atoms/config.gni`: Declares `is_atoms = true`.
- `build/config/atoms/BUILD.gn`: Configures Clang for `x86_64-unknown-none-elf` freestanding compilation, linking with `libapal.a`, `libatoms_cpp.a`, and `libatoms_c.a`.
