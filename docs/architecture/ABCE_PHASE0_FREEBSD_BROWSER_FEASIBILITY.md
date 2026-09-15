# ATOMS OS — ABCE PHASE 0
## FreeBSD Compatibility Environment + Existing Browser Feasibility Audit

**Audit date:** 2026-09-15  
**Repository:** `Saumya25-hub/Atoms-OS`  
**Audited branch:** `main`  
**Audited repository head:** `9265e2e4b7a3a30159686311dada9a3705044e70`  
**Classification:** Forensic / feasibility only — no ABCE implementation performed  
**Scope rule:** BOS remains the native ATOMS kernel. FreeBSD is evaluated only as a compatibility/guest environment.

---

## 1. Executive Summary

### Verdict: **YELLOW — technically feasible, but not implementation-ready**

The audit establishes that the current BOS Kernel already contains a substantial portion of the *primitive* infrastructure needed to host a complex foreign userspace: x86_64 Ring 3 execution, isolated PML4 address spaces, process/thread management, ELF loading, `mmap`/`munmap`/`mprotect` syscall entry points, IPC, shared memory, filesystem syscalls, native IPv4/TCP/UDP/DNS, GUI surface mapping, input events, audio service entry points, and an established Chromium/APAL investigation history.

However, the current BOS ABI is **not yet a FreeBSD ABI** and several primitives are currently only partial or semantically too weak for a real FreeBSD Chromium process tree. The most important blockers are:

1. **FreeBSD userspace ABI/runtime is missing.** A FreeBSD Chromium executable is not just an ELF file; it expects FreeBSD libc/libpthread/rtld semantics and a large shared-library graph.
2. **Dynamic ELF runtime linking is incomplete.** BOS contains relocation-engine pieces, but the complete FreeBSD-style `ld-elf.so.1` activation model, dependency loading, symbol lookup, TLS initialization, constructors, and `dlopen`/`dlclose` behavior are not demonstrated as production-ready.
3. **The current `mmap`/`mprotect` implementation is insufficient for a browser runtime.** `mmap` uses a global bump allocator and ignores file-backed mappings; `mprotect` changes writable state but does not implement the full protection model, including executable semantics.
4. **The current futex implementation is not a real futex wait queue.** `FUTEX_WAIT` validates the value and yields once; it does not block the calling thread on a keyed wait queue and wake it correctly.
5. **The native network API is narrower than FreeBSD's socket ABI.** Current BOS socket code is IPv4-centric and there is no verified `kqueue`/`kevent` equivalent. The FreeBSD Chromium port explicitly carries an `epoll-shim` dependency.
6. **The graphics/userspace ABI is not a FreeBSD desktop stack.** BOS has a useful native BWE/BCM surface path, but the FreeBSD Chromium port depends on Wayland/X11, GTK, GL/Mesa/libdrm, font libraries, input libraries, and related userspace components.
7. **FreeBSD Chromium is a real, maintained target.** The FreeBSD ports tree currently carries Chromium **152.0.7977.75**, restricted to `amd64` and `aarch64`, with a large native dependency graph and FreeBSD-specific patches. This is the realistic browser target; there is no evidence of an official Google Chrome FreeBSD release channel.
8. **A complete FreeBSD kernel guest is unnecessary for the preferred architecture, but a tiny FreeBSD compatibility layer is not tiny.** The practical target is a **FreeBSD userspace ABI/runtime compatibility environment on BOS**, not a FreeBSD kernel subsystem.

Therefore ABCE should **not** begin as a VM, and should **not** import a FreeBSD kernel. The recommended direction is **Option C, evolved into a controlled hybrid Option D**: expose the FreeBSD userspace ABI through an ABCE compatibility layer while translating selected services directly to BOS-native primitives. This preserves ATOMS identity and avoids the enormous cost of virtualizing a second kernel.

No implementation was performed in this phase.

---

## 2. Audit Baseline and Evidence Rules

This audit deliberately distinguishes source evidence from earlier architecture documents. Several older Chromium-readiness documents in the repository describe earlier states such as “missing mmap” or “missing waitpid”. The current source was checked where possible before assigning status.

### Current repository evidence

- `docs/START_HERE.md` identifies ATOMS as an independent OS with BOS Kernel, BOFS, Ring 0/Ring 3 separation and native UEFI boot.
- `kernel/core/syscall/include/syscall.h` currently defines standard memory, synchronization, filesystem, thread, process, IPC, shared-memory, GUI and audio syscall numbers.
- `kernel/core/syscall/src/services.c` currently contains implementations for memory mapping, protection, futex, filesystem access and process/thread/IPC services.
- `kernel/loader/reloc/reloc_engine.c` contains relocation handlers for `R_X86_64_64`, `R_X86_64_GLOB_DAT`, `R_X86_64_JUMP_SLOT` and `R_X86_64_RELATIVE`.
- `kernel/net/socket/` contains a native socket layer, while current source evidence shows `AF_INET` as the supported domain.
- Existing Chromium forensic documents establish the earlier APAL/Chromium work, but their claims are not treated as proof when current source disagrees.

Repository head used for this audit: `9265e2e4b7a3a30159686311dada9a3705044e70` (2026-09-15).

---

# 3. ATOMS Current Readiness Matrix

Legend:

- **GREEN** — primitive exists and is useful for ABCE; still requires ABI adaptation where applicable.
- **YELLOW** — primitive exists but is incomplete, narrow, or not yet demonstrated at the required semantics.
- **RED** — required capability is not currently demonstrated.

| Area | BOS evidence | Status | ABCE consequence |
|---|---|---:|---|
| x86_64 long mode | Native UEFI/x86_64 kernel | GREEN | Correct execution target for FreeBSD amd64 userspace. |
| Ring 0 / Ring 3 | `IA32_LSTAR`, user CS/SS, syscall frame | GREEN | Strong base for foreign userspace isolation. |
| Process creation | BOS process manager / ELF/BOSX loading | GREEN/YELLOW | Process model exists; FreeBSD fork/exec/wait semantics still need ABI mapping. |
| Thread creation | `SYS_THREAD_SPAWN`, scheduler user task creation | GREEN/YELLOW | Threads exist; pthread semantics and TLS ABI still need work. |
| Context switching | Scheduler/task infrastructure | GREEN | Required execution primitive exists. |
| CPU state | Scheduler saves/restores task state and FPU/SSE state | GREEN | Suitable base for browser threads. |
| Interrupts/exceptions | IDT/APIC/exception path | GREEN/YELLOW | Ring 3 faults can be isolated; POSIX/FreeBSD signal semantics are incomplete. |
| Syscall gateway | Hardware `syscall`/`sysret` path | GREEN | Good native ABI boundary. |
| Virtual memory | 4-level paging/PML4 isolation | GREEN | Core requirement exists. |
| `mmap` | `SYS_MMAP` + `sys_service_mmap` | YELLOW | Anonymous mappings work; current allocator is not a full POSIX/FreeBSD VM API. |
| `munmap` | `SYS_MUNMAP` | YELLOW | Basic unmapping exists; mapping metadata/lifetime semantics need hardening. |
| `mprotect` | `SYS_MPROTECT` | YELLOW | Writable-bit changes exist; full R/W/X semantics are not implemented. |
| Page faults | Ring 3 exception handling | GREEN/YELLOW | Isolation exists; signal delivery must be added for FreeBSD semantics. |
| User pointer validation | syscall validation helpers | GREEN | Important security primitive exists. |
| Shared memory | BOS SHM manager + `SYS_SHM_CALL` | GREEN/YELLOW | Primitive exists; FreeBSD/POSIX mapping semantics need ABI layer. |
| IPC | BOS IPC channels + `SYS_IPC_CALL` | GREEN/YELLOW | Good ABCE substrate; FreeBSD IPC ABI still absent. |
| Futex-equivalent | `SYS_FUTEX` | YELLOW | Current implementation is a yield-based approximation, not a complete futex wait queue. |
| TLS | x86_64 FS/GS support is present in architecture work | YELLOW | Native mechanism exists; FreeBSD TLS/rtld initialization not demonstrated. |
| ELF executable loading | BOS ELF loader | GREEN | Can load ELF, but FreeBSD `PT_INTERP`/rtld model is a separate requirement. |
| ELF dynamic linking | Relocation engine exists | YELLOW | Relocations exist, but complete runtime loader/dependency/symbol/TLS behavior is not proven. |
| File descriptors | BOS FD/VFS syscalls exist | YELLOW | Basic FD-like services exist; full FreeBSD FD flags, `fcntl`, `ioctl`, inheritance, pollability are incomplete. |
| BOFS/VFS | Native VFS + BOFS | GREEN | Excellent substrate for transparent storage bridge. |
| Permissions/ownership | `stat` fields and file modes exist | YELLOW | Need FreeBSD uid/gid/mode semantics in ABCE. |
| TCP | Native TCP implementation | GREEN/YELLOW | Core transport exists. |
| UDP | Native UDP implementation | GREEN/YELLOW | Core transport exists. |
| DNS | Native DNS resolver | GREEN/YELLOW | Resolver exists; browser needs libc resolver semantics and asynchronous integration. |
| IPv4 | Native | GREEN | Good starting point. |
| IPv6 | No verified current native socket ABI | RED/YELLOW | Significant compatibility gap for a modern browser. |
| Non-blocking sockets | No verified `O_NONBLOCK`/`fcntl` ABI | RED/YELLOW | Needed for realistic browser networking. |
| `kqueue`/`kevent` | No verified BOS equivalent | RED | Major FreeBSD userspace event-loop dependency. |
| Graphics framebuffer | UEFI GOP/native framebuffer | GREEN | Strong low-level display base. |
| BWE/BCM windows | Native window/surface mapping | GREEN | Strong transparent-window substrate. |
| Shared graphics surfaces | GUI surface mapping + shared memory primitives | GREEN/YELLOW | Feasible bridge; synchronization semantics need design. |
| GPU acceleration | No verified complete 3D API | RED/YELLOW | Full FreeBSD Chromium GPU path should not be the Phase 1 target. |
| Keyboard/mouse | Native PS/2/USB HID and GUI event queue | GREEN | Good input substrate. |
| Audio | Audio syscall and driver foundations | YELLOW | Browser audio compatibility still needs a userspace API layer. |
| Fonts | Native basic bitmap fonts | RED for browser parity | FreeBSD Chromium expects FreeType/HarfBuzz/fontconfig-class services. |
| Video codecs | Partial/absent native media stack | RED/YELLOW | Modern media features cannot be assumed. |
| Resource limits | No verified full FreeBSD `rlimit` ABI | RED/YELLOW | Browser sandbox/resource controls need an ABCE policy model. |
| Signals | `SYS_KILL`/fault isolation exist, but POSIX delivery is incomplete | YELLOW | Required for FreeBSD process semantics. |
| Security sandbox | BOS process isolation + native permissions; no FreeBSD Capsicum ABI | YELLOW | BOS must remain final authority; FreeBSD sandbox calls need translation. |
| Virtualization | No verified VMX/SVM/EPT hypervisor subsystem | RED | Full FreeBSD VM is not currently a practical native path. |

### Important memory finding

The current `sys_service_mmap()` implementation is useful proof of concept but is not a production browser VM implementation. It uses a global bump pointer beginning at `0x60000000`, imposes a per-call length limit, allocates physical pages eagerly, inserts a guard page between allocations, and currently ignores file-backed mapping semantics. `sys_service_mprotect()` changes the writable bit but does not fully implement executable protection semantics. These details are decisive for V8/JIT and FreeBSD libc/rtld compatibility.

### Important synchronization finding

The current `sys_service_futex()` handles `FUTEX_WAIT` by checking the user value and then calling `scheduler_yield()`. `FUTEX_WAKE` returns a count-like result without maintaining a keyed wait queue. This is **not equivalent to a production futex implementation** and should not be counted as complete browser synchronization support.

---

# 4. FreeBSD Forensic Audit

## 4.1 Current FreeBSD target

As of this audit, the current production FreeBSD release is **15.1-RELEASE**, released June 16, 2026. FreeBSD lists 15.1 as the current production release, with `amd64`, `aarch64`, `armv7`, `powerpc64`, `powerpc64le`, and `riscv64` supported by the release family. The ABCE browser target should use **amd64** because the ATOMS platform under audit is x86_64 and the current FreeBSD Chromium port explicitly targets `amd64` and `aarch64`.

FreeBSD 15.1 uses the traditional BSD process/thread/kernel architecture and provides a complete kernel, libc, pthread/runtime, network stack, filesystem interfaces, dynamic linker and security framework. The FreeBSD Architecture Handbook describes kernel-visible threads as `struct thread` and processes as `struct proc`, with process credentials and resource controls attached to the process model.

FreeBSD also provides Capsicum capability-mode sandboxing. In capability mode, processes are restricted to operations on file descriptors and limited global state; capability rights can be reduced on file descriptors and cannot be expanded.

## 4.2 Required / Optional / Unnecessary / ATOMS-native replacement

| FreeBSD facility | Classification | ABCE treatment |
|---|---|---|
| amd64 process model | REQUIRED | Present through BOS process model + ABI adapter. |
| FreeBSD syscall ABI | REQUIRED | Translate into ABCE services; do not import FreeBSD kernel. |
| libc / libpthread | REQUIRED | Must provide compatible user-visible ABI. |
| `ld-elf.so.1` / rtld | REQUIRED | Need FreeBSD-compatible dynamic-loader behavior or an equivalent ABCE activation path. |
| ELF `PT_INTERP` handling | REQUIRED | BOS executable activation must recognize FreeBSD interpreter/runtime expectations. |
| VM: `mmap`, `mprotect`, `munmap`, shared mappings | REQUIRED | Translate to BOS VMM. |
| Thread/TLS ABI | REQUIRED | Map FreeBSD pthread/TLS behavior to BOS threads and FS/GS facilities. |
| Signals | REQUIRED | Translate FreeBSD signal ABI to BOS exception/process mechanisms. |
| FD table / `fcntl` / `ioctl` | REQUIRED | Expand BOS FD abstraction. |
| Sockets | REQUIRED | Translate FreeBSD socket ABI to BOS native network stack. |
| `kqueue`/`kevent` | REQUIRED for full FreeBSD Chromium | Implement as ABCE event abstraction or provide a compatible equivalent. |
| IPv6 | REQUIRED for full modern browser | BOS network layer must expose it. |
| Filesystem paths | REQUIRED | Map FreeBSD namespace to controlled ABCE namespace over BOFS. |
| `/proc`-like process inspection | REQUIRED where Chromium expects it | Provide virtual compatibility data, not a FreeBSD procfs. |
| `/dev` device model | OPTIONAL/feature-dependent | Replace with ABCE virtual devices where browser features need them. |
| FreeBSD kernel scheduler | UNNECESSARY | BOS scheduler remains authoritative. |
| FreeBSD physical memory manager | UNNECESSARY | BOS PMM/VMM remain authoritative. |
| FreeBSD NIC drivers | UNNECESSARY | BOS NIC drivers remain authoritative. |
| FreeBSD UFS/ZFS | UNNECESSARY | BOFS/VFS remains authoritative. |
| FreeBSD desktop environment | UNNECESSARY | BWE/BCM and ATOMS Desktop remain authoritative. |
| FreeBSD boot process | UNNECESSARY | BOS continues to boot from UEFI. |
| Capsicum kernel implementation | UNNECESSARY as code | Recreate the security *policy semantics* through BOS-native capability/process controls where required. |

---

# 5. Existing Browser Availability — 2026

## 5.1 Google Chrome

**Finding: no official Google Chrome FreeBSD release channel was identified.** Google Chrome's official product distribution is not a FreeBSD target. Therefore the term “FreeBSD Chrome” should not be used for the ABCE target.

## 5.2 Chromium

**Finding: Chromium is actively maintained in the FreeBSD Ports Collection.**

The current FreeBSD ports tree records:

- Port: `www/chromium`
- Version: **152.0.7977.75**
- Port revision: `2`
- Architectures: **`aarch64`, `amd64`**
- Build system: GN + Ninja, Clang/LLVM, Rust and several supporting tools
- Graphics: Wayland and X11 paths, GL/GBM/Mesa/libdrm dependencies
- Text: FreeType, HarfBuzz, fontconfig
- Audio: PulseAudio/Sndio/ALSA options
- Networking/event compatibility: `libepoll-shim` is a direct dependency
- Security: FreeBSD-specific Chromium sandbox patches exist
- License expression recorded by the port: `BSD3CLAUSE LGPL21 MPL11`

The FreeBSD ports tree is therefore the most concrete provenance source for an existing FreeBSD-compatible browser build.

## 5.3 Third-party browser builds

Third-party builds may exist, but they are **not** the preferred ABCE target because their update cadence, patches, API keys, signing/provenance and redistribution terms may differ from the maintained FreeBSD Ports Chromium package.

### Browser target decision

```text
Google Chrome                 → REJECTED as FreeBSD target
FreeBSD Chromium port         → ACCEPTED as primary target
Community/third-party browser → SECONDARY / fallback only
```

---

# 6. Chromium Dependency Forensics

The FreeBSD port itself gives a much more realistic dependency picture than an abstract “POSIX-compatible browser” description.

The current port declares build dependencies including Bash, Python/Jinja/PLY, Rust, Node, GN/Ninja-related tooling, XCB protocol data, libva, Mesa and Qt development components. Runtime/library dependencies include accessibility libraries, FLAC/Opus/Speex, DBus, `libepoll-shim`, libevent, ICU, PCI, NSS, Cairo, libdrm, image libraries, AV1/H.264 support, FreeType, HarfBuzz, fontconfig, Wayland, XKB common, X shared-memory fencing and X11 components.

The port also enables `use_custom_libcxx`, `use_custom_libunwind`, PartitionAlloc, Aura/Views and LLD, while supporting multiple optional audio, codec, printing, Kerberos, PipeWire and Widevine features.

This means the target is **not** “Chromium plus a socket call”. It is a complete C/C++ userspace ecosystem.

## Dependency matrix

| Browser requirement | FreeBSD provides | BOS provides | ABCE work | Classification |
|---|---|---|---|---|
| ELF process | Yes | Yes | ABI activation | REQUIRED |
| libc | Yes | Partial/native C runtime pieces | FreeBSD libc ABI | REQUIRED |
| libc++ | Yes/Chromium-built | Toolchain support | Runtime packaging | REQUIRED |
| libpthread | Yes | Threads exist | pthread ABI layer | REQUIRED |
| TLS | Yes | FS/GS foundations | FreeBSD TLS ABI | REQUIRED |
| `mmap` | Yes | Yes, limited semantics | Full compatibility | REQUIRED |
| `mprotect` | Yes | Yes, limited semantics | Full R/W/X semantics | REQUIRED |
| shared memory | Yes | Yes | FreeBSD mapping semantics | REQUIRED |
| process spawn | Yes | Yes | FreeBSD fork/exec-like behavior | REQUIRED |
| wait/reap | Yes | Kernel process wait exists + syscall | Complete ABI behavior | REQUIRED |
| signals | Yes | Partial | Full signal model | REQUIRED |
| sockets | Yes | IPv4 TCP/UDP core | FreeBSD socket ABI | REQUIRED |
| IPv6 | Yes | Not verified | Native/bridge | REQUIRED |
| `kqueue` | Yes | Not verified | Compatibility layer | REQUIRED |
| epoll | Linux API | Port uses `libepoll-shim` | Provide expected event abstraction | REQUIRED |
| filesystem | Yes | BOFS/VFS | Namespace/FD semantics | REQUIRED |
| dynamic linker | `ld-elf.so.1` | Relocation engine only | Full activation/runtime linking | REQUIRED |
| fontconfig | Yes | No | Userspace package or bridge | REQUIRED |
| FreeType | Yes | No | Userspace package or bridge | REQUIRED |
| HarfBuzz | Yes | No | Userspace package or bridge | REQUIRED |
| Wayland/X11 | Yes | BWE/BCM | Native window bridge or adapted backend | REQUIRED for full port |
| Mesa/libdrm | Yes | No complete 3D stack verified | Defer or virtualize | OPTIONAL for first prototype |
| Audio | Yes | Partial | Sndio/Pulse/ALSA compatibility or native backend | REQUIRED for audio parity; optional first prototype |
| Video codecs | Yes | Partial/insufficient | Package user-space codec stack | OPTIONAL first prototype |
| GPU sandbox | FreeBSD-specific Chromium sandbox | BOS security boundary | Translate policy | REQUIRED |
| Capsicum | Yes | No | BOS-native security equivalent | REQUIRED for faithful sandbox semantics, not code reuse |

---

# 7. FreeBSD Chromium Sandbox Findings

The current FreeBSD Chromium port contains a dedicated FreeBSD sandbox patch set. This is significant: FreeBSD Chromium is not simply an unmodified Linux build.

The current patch tree contains FreeBSD-specific sandbox sources and modifies sandbox feature selection for `IS_BSD`. The FreeBSD sandbox implementation includes resource-limit handling and FreeBSD-specific process initialization. The port also contains FreeBSD-specific process, HID, network, graphics and build patches.

However, the FreeBSD Chromium sandbox code must **not** be mistaken for a requirement to import a FreeBSD kernel. It describes the security assumptions made by Chromium when it runs on FreeBSD. ABCE can translate those assumptions into BOS-native security primitives.

For ATOMS, the security hierarchy should be:

```text
Chromium sandbox policy
        ↓
ABCE policy translator
        ↓
BOS process / capability boundary
        ↓
BOS Kernel security authority
```

BOS must remain the final authority.

---

# 8. Architecture Options

## Option A — Full FreeBSD Virtual Machine

```text
BOS
 ↓
Virtualization
 ↓
FreeBSD Kernel
 ↓
FreeBSD userspace
 ↓
Chromium
```

### Assessment

**Not recommended.** The repository contains no verified VMX/SVM/EPT/VPID hypervisor subsystem. Building one would add a second kernel boundary, virtual device model, guest memory translation, virtual interrupts, virtual storage/network/graphics, VM lifecycle and host/guest IPC. It is architecturally clean but far outside the minimum goal.

| Metric | Assessment |
|---|---|
| RAM overhead | High; complete guest OS and services |
| CPU overhead | Moderate/high depending on device virtualization |
| I/O latency | Higher due to virtual devices/queues |
| Graphics latency | Highest risk without GPU passthrough/virtio acceleration |
| Complexity | Very high |
| Isolation | Strong |
| ATOMS fit | Poor |

## Option B — FreeBSD Kernel Guest/Subsystem

```text
BOS
 ↓
ABCE
 ↓
FreeBSD Kernel
 ↓
Browser userspace
```

### Assessment

**Rejected for ABCE Phase 0 target.** A FreeBSD kernel expects to own kernel-mode execution, hardware abstraction, interrupts, VM, scheduler, process tables, filesystems, devices and system-call entry. Running it as a subsystem is effectively a hypervisor/virtual-machine problem or a major kernel port. That violates the minimum-work objective.

## Option C — FreeBSD Compatibility Runtime

```text
BOS
 ↓
ABCE
 ↓
FreeBSD-compatible userspace/runtime
 ↓
Chromium
```

### Assessment

**Technically viable and closest to the target.** ABCE implements the userspace ABI and translates it to BOS-native primitives. No FreeBSD kernel is present.

The catch is scope: “runtime” must include enough libc, pthread, rtld, syscall, FD, signal, socket, event, filesystem, TLS and graphics behavior to satisfy the selected Chromium build.

## Option D — Hybrid

```text
BOS
 ↓
ABCE
 ├── FreeBSD ABI compatibility
 ├── BOS-native process/VM/IPC
 ├── BOS-native network bridge
 ├── BOS-native BOFS bridge
 └── BOS-native BWE/BCM graphics bridge
 ↓
FreeBSD-compatible Chromium userspace
```

### Assessment

**RECOMMENDED.** This is Option C implemented as a deliberately thin compatibility boundary, with BOS-native services used wherever Chromium's FreeBSD ABI can be faithfully mapped without importing FreeBSD kernel semantics.

---

# 9. Recommended ABCE Architecture

The recommended conceptual architecture is:

```text
UEFI
  ↓
BOS Kernel
  ↓
ATOMS Userspace
  ↓
ABCE Supervisor / Runtime
  ├── FreeBSD syscall ABI
  ├── FreeBSD libc/pthread ABI
  ├── FreeBSD ELF/rtld activation
  ├── FreeBSD FD + event ABI
  ├── FreeBSD socket ABI
  ├── FreeBSD filesystem namespace
  ├── FreeBSD signal/process ABI
  ├── FreeBSD sandbox policy translation
  └── browser-specific compatibility policy
       ↓
FreeBSD-compatible Chromium
       ↓
ABCE graphics/network/filesystem bridges
       ↓
BOS-native BWE / BOFS / network / input
       ↓
ATOMS Desktop
```

This keeps the browser visible as an ATOMS application without creating a second operating-system boot environment.

---

# 10. Filesystem Design

Target:

```text
Chromium FreeBSD path
        ↓
ABCE namespace
        ↓
BOS VFS
        ↓
BOFS
        ↓
ATOMS Downloads/
```

The ABCE namespace should be virtual. Chromium must not receive unrestricted access to the entire BOFS namespace.

Suggested policy classes:

| Virtual path class | BOS target | Policy |
|---|---|---|
| Home/profile | Controlled ABCE browser profile directory | Read/write |
| Cache | Browser cache directory | Read/write, disposable |
| Downloads | `ATOMS Downloads/` | Read/write |
| Temp | ABCE temp directory | Read/write, cleanup |
| System libraries | ABCE runtime bundle | Read-only |
| Devices | Virtual ABCE devices | Explicitly granted |
| Raw BOFS | None | Denied |
| Kernel memory | None | Denied |
| Arbitrary device nodes | None | Denied |

This is preferable to mounting BOFS into a pretend FreeBSD root filesystem.

---

# 11. Graphics Design

Target path:

```text
Chromium
 ↓
FreeBSD-compatible graphics backend
 ↓
ABCE graphics adapter
 ↓
BWE/BCM shared surface
 ↓
ATOMS compositor
```

### Phase 0 conclusion

A virtual GPU is **not required for the first compatibility milestone** if Chromium can be run with a software-rendered or otherwise constrained graphics path. BOS already exposes a native framebuffer and BWE surface mapping. This gives a realistic path to a browser window without emulating a full FreeBSD X/Wayland desktop.

However, the current FreeBSD Chromium port depends on Wayland/X11/GL/Mesa/libdrm components. Therefore “transparent browser window” is not automatically equivalent to “the entire FreeBSD graphics stack is unnecessary.” The ABCE target should choose one of these two paths:

1. **Compatibility-first:** supply a minimal FreeBSD browser graphics userspace and translate final surfaces into BWE.
2. **Native-backend:** adapt Chromium's platform abstraction directly to BWE, while retaining FreeBSD-compatible userspace for the non-graphics portions.

The second path is likely lower-latency but increases Chromium porting surface.

### Low-latency requirement

Prefer shared surfaces over copy-based frame transport:

```text
Renderer surface
      ↓
shared physical/page-backed surface
      ↓
BWE window surface
      ↓
BCM composition
```

The surface contract must include ownership, fencing/versioning, dirty rectangles and lifetime rules.

---

# 12. Network Design

Target:

```text
Chromium
 ↓
FreeBSD socket ABI
 ↓
ABCE socket translator
 ↓
BOS socket/network API
 ↓
BOS TCP/UDP/DNS
 ↓
NIC
```

The current BOS network layer is a useful foundation, but the verified socket implementation currently restricts `atoms_socket()` to `AF_INET`. There is no current evidence of an IPv6 socket ABI or `kqueue`/`kevent` equivalent.

The FreeBSD Chromium port directly depends on `libepoll-shim`, which demonstrates that Chromium's event abstraction on FreeBSD is not limited to simple blocking TCP calls.

### Required networking compatibility surface

- IPv4
- IPv6
- TCP
- UDP
- DNS resolver ABI
- non-blocking sockets
- `fcntl`/socket flags
- `getsockopt`/`setsockopt` equivalents
- polling/event notification
- socketpair/unix-domain sockets where Chromium uses them
- error/errno semantics
- close/lifetime behavior

A direct translation from FreeBSD calls to BOS native calls is preferable to importing a second network stack.

---

# 13. Memory / Resource Model

Target budget:

> ABCE maximum target budget ≈ 4 GB, **not** a permanent reservation.

Recommended model:

```text
BOS Resource Manager
        ↓
ABCE accounting domain
        ↓
Chromium processes / shared memory / caches
```

### Requirements

- demand allocation
- per-process accounting
- total ABCE cap
- reclaim after browser exit
- no permanent 4 GB reservation
- BOS priority over ABCE
- explicit shared-memory accounting
- crash cleanup
- cache cleanup
- guard against one renderer consuming the entire ABCE budget

### Reality check

A 4 GB ceiling is a **resource-control target**, not a promise that a Chromium build will be smooth on a machine with only 3 GB total RAM. Browser memory usage varies with tabs, sites, extensions, caches, media, renderer count and graphics mode.

The first implementation should measure:

- browser process baseline
- renderer baseline
- V8 heap growth
- shared memory
- graphics surfaces
- font caches
- network buffers
- page-cache/file mappings
- peak startup memory
- peak 1-tab and 5-tab memory

---

# 14. Latency Model

## VM path

```text
browser
 → guest syscall
 → guest kernel
 → VM exit / virtual device
 → BOS
 → virtual device
 → guest
```

This adds avoidable boundaries.

## ABCE path

```text
browser
 → FreeBSD ABI call
 → ABCE translator
 → BOS syscall/native service
 → result
```

This is substantially closer to native execution.

### Main latency risks

1. syscall translation
2. process/thread context switching
3. filesystem namespace translation
4. network event translation
5. graphics surface synchronization
6. cross-process IPC
7. shared-memory mapping
8. browser startup and dynamic linking
9. font loading
10. cache I/O

### Future measurable benchmarks

| Benchmark | Measurement |
|---|---|
| ABCE syscall | median + p99 ns/us |
| FreeBSD `read/write` → BOFS | MB/s + p99 latency |
| TCP connect | ms |
| TCP throughput | MB/s |
| DNS lookup | ms |
| `kqueue`/event wait | wakeup latency |
| IPC message | ns/us per 1 KB message |
| shared-memory handoff | us/frame |
| surface present | frame-to-display latency |
| input event | input-to-render latency |
| browser cold start | ms |
| browser warm start | ms |
| renderer spawn | ms |
| first page paint | ms |

### Native-feel target

ABCE should be judged against native ATOMS applications, not against a VM. The most important metric is not theoretical syscall overhead; it is **input-to-paint and network/file-event latency under real browser load**.

---

# 15. Security Model

BOS remains the final security authority.

```text
Browser content
      ↓
Chromium sandbox
      ↓
ABCE compatibility policy
      ↓
BOS process isolation
      ↓
BOS kernel
```

### Required controls

- separate browser process identities
- renderer isolation
- controlled executable mapping
- W^X policy
- controlled filesystem namespace
- controlled network access
- controlled IPC endpoints
- shared-memory ownership
- graphics-surface ownership
- process kill/reap
- fault isolation
- resource quotas
- no direct physical-memory access
- no direct MMIO/device access
- no raw BOFS metadata access

### FreeBSD Capsicum

FreeBSD's Capsicum model is valuable as a **policy reference**, not as kernel code to import. Its capability-mode idea maps naturally to ABCE's controlled FD/object model. ABCE should translate the browser's expected authority restrictions into BOS-native capability handles and process policy.

---

# 16. Licensing / Provenance Audit

No blanket “no policy issue” conclusion is made.

### ATOMS-authored

BOS Kernel, BOFS, BWE/BCM, Rook, ABCE (future), native ATOMS services and ATOMS-specific adapter code remain ATOMS-authored and governed by the repository's own license/provenance rules.

### FreeBSD

FreeBSD uses permissive BSD-style licensing. The FreeBSD licensing policy identifies BSD-2-Clause and BSD-3-Clause among acceptable BSD-like licenses. Individual source files must still retain their upstream notices.

### Chromium

Chromium's source tree contains multiple license classes. Chromium's own third-party guidance explicitly identifies BSD-3-Clause and Apache-2.0 as notice-level licenses and also documents reciprocal/restricted cases. Chromium requires license metadata and license files for third-party dependencies.

### Current FreeBSD Chromium port

The FreeBSD port currently declares:

```text
BSD3CLAUSE LGPL21 MPL11
```

with a multi-license combination. This means a future ABCE browser distribution must retain the correct Chromium/third-party notices and must audit the **actual shipped dependency closure**, not merely the top-level port license field.

### Provenance rule

```text
ATOMS code       → ATOMS provenance
FreeBSD code     → FreeBSD provenance
Chromium code    → Chromium provenance
Other libraries  → each upstream project's provenance
```

No source should be copied into ATOMS merely because its license is permissive. License compatibility and redistribution obligations must be tracked per component.

---

# 17. Highest-Risk Subsystems

Ranked by feasibility impact:

1. **FreeBSD userspace ABI + libc/rtld/TLS** — highest risk.
2. **Chromium process/sandbox semantics** — high risk.
3. **Event/synchronization ABI (`kqueue`, pthread/futex semantics)** — high risk.
4. **Graphics/font userspace stack** — high risk for real browser UX.
5. **Filesystem/FD semantics** — medium/high risk.
6. **IPv6/nonblocking network semantics** — medium/high risk.
7. **Audio/media** — medium risk; can be deferred.
8. **Native window integration** — medium risk because BWE is already strong.

---

# 18. Recommended Architecture Verdict

## **YELLOW**

The architecture is feasible, but the BOS platform is not yet sufficiently complete to execute a stock FreeBSD Chromium binary transparently.

This is **not a RED verdict** because the missing pieces are compatibility/runtime surfaces rather than a fundamental incompatibility between x86_64 BOS and FreeBSD userspace.

This is **not a GREEN verdict** because the current VM, syscall, event, dynamic-loader, network and userspace graphics semantics are not yet sufficient for a production FreeBSD Chromium build.

### Recommended architecture

**Option D — Hybrid FreeBSD Compatibility Runtime on BOS**

with the following rule:

> FreeBSD is an ABI/runtime compatibility target; BOS remains the kernel, scheduler, memory manager, VFS, network authority and desktop compositor.

---

# 19. Required BOS Changes — Phase 1 Prerequisites

These are **recommendations only; no changes are made in Phase 0**.

### Memory

- Replace global bump-only `mmap` allocation with per-address-space mapping metadata.
- Support anonymous and file-backed mappings.
- Implement complete `PROT_READ/WRITE/EXEC` semantics.
- Define W^X policy compatible with JIT workloads.
- Implement mapping lookup and safe partial unmapping.
- Ensure mapping reclamation on process exit.

### Synchronization

- Implement real keyed futex wait queues.
- Support wake counts and timeout semantics.
- Integrate with scheduler blocking/wakeup rather than `yield`.

### Process / signals

- Complete FreeBSD-compatible process creation/exec/wait semantics.
- Complete signal delivery, masks, handlers and fault-to-signal translation.
- Define parent/child and process-group semantics required by Chromium.

### Dynamic loading

- Implement a proper dynamic ELF activation path.
- Resolve `DT_NEEDED`, `DT_SONAME`, symbol tables, relocations, PLT/GOT and constructors.
- Support TLS modules and `dlopen`/`dlclose` behavior as required.
- Define `PT_INTERP` handling for FreeBSD `ld-elf.so.1` compatibility.

### FD/event ABI

- Expand file descriptors into a stable POSIX/FreeBSD-compatible table.
- Implement `fcntl`-class flags.
- Provide an event notification abstraction compatible with `kqueue`/`kevent` or the subset Chromium actually consumes.
- Support pipes/socketpairs and descriptor inheritance where required.

### Networking

- Add IPv6.
- Add non-blocking socket semantics.
- Add socket options and error translation.
- Add event notification integration.

### Graphics/userspace

- Decide whether Phase 1 uses a software renderer or a minimal native BWE Chromium backend.
- Provide FreeType/HarfBuzz/fontconfig-class services if the stock FreeBSD Chromium UI path is retained.
- Define shared-surface synchronization.

---

# 20. Required ABCE Components

Phase 1 should conceptually contain:

```text
abce/
 ├── abi/
 │   ├── syscall/
 │   ├── errno/
 │   ├── signals/
 │   ├── process/
 │   └── fd/
 ├── runtime/
 │   ├── libc/
 │   ├── pthread/
 │   ├── tls/
 │   └── rtld/
 ├── vm/
 │   ├── mmap/
 │   └── shared_memory/
 ├── io/
 │   ├── files/
 │   ├── sockets/
 │   └── events/
 ├── graphics/
 │   └── bwe_bridge/
 ├── security/
 │   └── sandbox_policy/
 └── launcher/
```

This is an architectural decomposition only. No such implementation was created during Phase 0.

---

# 21. Required FreeBSD Components

The preferred target should use **only the FreeBSD userspace components needed by the selected Chromium build**, not the FreeBSD kernel.

Potential required runtime set:

- FreeBSD-compatible libc ABI
- pthread ABI
- rtld-compatible loader
- libc++/unwind support as required by the Chromium build
- NSS
- ICU
- FreeType
- HarfBuzz
- fontconfig
- image libraries
- browser codec libraries
- event-loop compatibility
- browser storage/cache libraries
- selected audio libraries if enabled
- selected graphics userspace libraries if retained

Unneeded for the first target:

- FreeBSD bootloader
- FreeBSD kernel
- FreeBSD scheduler
- UFS/ZFS as the browser storage backend
- FreeBSD desktop environment
- FreeBSD NIC drivers
- FreeBSD GPU kernel drivers

---

# 22. Browser Target

## Primary target

**FreeBSD Ports Chromium 152.0.7977.75** (`amd64`) as observed in the current FreeBSD ports tree.

Why:

- maintained FreeBSD port
- current version
- explicit `amd64` support
- FreeBSD-specific patches already exist
- existing Chromium platform abstraction is aware of FreeBSD/BSD
- reproducible provenance through FreeBSD Ports

## Not target

**Google Chrome for FreeBSD** — no official FreeBSD release channel identified.

## Not preferred

Unverified third-party Chromium/Chrome-like builds.

---

# 23. Phase 1 Implementation Plan — Proposal Only

This is the proposed order of work after Phase 0 approval. **Phase 1 was not started.**

### Phase 1A — ABI proof

Goal: execute a tiny FreeBSD-linked amd64 test binary, not Chromium.

Validate:

- ELF activation
- `PT_INTERP`
- rtld
- libc startup
- TLS
- one pthread
- `mmap`/`mprotect`
- file open/read/write
- exit

### Phase 1B — process/runtime proof

Validate:

- child process creation
- wait/reap
- signals
- IPC
- shared memory
- real futex behavior
- event notification

### Phase 1C — network proof

Validate:

- IPv4
- IPv6
- TCP
- UDP
- DNS
- nonblocking I/O
- event polling

### Phase 1D — graphics proof

Validate:

- create BWE surface
- shared surface
- input events
- font rendering
- present latency

### Phase 1E — browser bootstrap

Only after the runtime passes the above tests:

- launch FreeBSD Chromium
- browser process starts
- renderer starts
- one blank page renders
- keyboard/mouse input works
- download maps to `ATOMS Downloads/`

### Phase 1F — real web workload

Then measure:

- HTTPS
- JavaScript/V8
- multiple renderer processes
- image decoding
- CSS/layout
- WebAssembly
- storage/cache
- audio/video as separately enabled features

---

# 24. Lowest-Risk First Prototype

The lowest-risk prototype is **not Chromium** and **not a FreeBSD kernel**.

It is:

```text
BOS
 ↓
ABCE
 ↓
FreeBSD amd64 ABI test executable
 ↓
libc + rtld + pthread + mmap + file I/O
 ↓
exit
```

Success criteria:

1. FreeBSD ELF loads under BOS.
2. FreeBSD dynamic loader starts.
3. libc initializes.
4. TLS works.
5. one pthread runs.
6. anonymous `mmap` works.
7. `mprotect` correctly changes permissions.
8. file read/write reaches BOFS through the ABCE namespace.
9. process exits and all memory is reclaimed.

Only after this is green should Chromium enter the compatibility test matrix.

---

# 25. Final Forensic Findings

### What ATOMS already has

ATOMS already has the most important *kernel-level shape* needed for this project: isolated Ring 3 execution, paging, scheduling, process/thread management, syscall entry, BOFS/VFS, IPC, shared memory, native networking, native graphics surfaces and input.

### What ATOMS does not yet have

ATOMS does not yet expose a complete FreeBSD userspace contract. In particular, the current code does not establish a production-ready FreeBSD dynamic-loader environment, full pthread/TLS semantics, complete signal semantics, complete FreeBSD FD/event semantics, IPv6/nonblocking socket ABI, or the large graphics/font userspace stack used by the current FreeBSD Chromium port.

### What FreeBSD provides that matters

FreeBSD supplies a mature amd64 userspace contract, pthread/TLS/process model, dynamic linker, kqueue event system, sockets, filesystem ABI, Capsicum security model and a maintained Chromium port. These are **interfaces and reference behaviors**, not components that must become part of BOS.

### What the browser actually proves

The current FreeBSD Chromium port is substantial and current. Version 152.0.7977.75 demonstrates that the target browser is real and actively maintained, but its dependency list also proves that the compatibility problem is much larger than a syscall shim.

### Architectural conclusion

The technically correct ATOMS direction is:

```text
UEFI
  ↓
BOS Kernel
  ↓
ATOMS Userspace
  ↓
ABCE
  ↓
FreeBSD-compatible userspace ABI/runtime
  ↓
FreeBSD Chromium
  ↓
ABCE native bridges
  ├── BOFS
  ├── BOS Network
  ├── BWE/BCM
  ├── BOS Input
  └── BOS Security
  ↓
ATOMS Desktop
```

FreeBSD should remain a **compatibility target**, not the kernel of ATOMS.

---

# 26. Source / Provenance References

## ATOMS repository

- `docs/START_HERE.md`
- `kernel/core/syscall/include/syscall.h`
- `kernel/core/syscall/src/services.c`
- `kernel/core/syscall/src/dispatcher.c`
- `kernel/loader/reloc/reloc_engine.h`
- `kernel/loader/reloc/reloc_engine.c`
- `kernel/net/socket/socket.h`
- `kernel/net/socket/socket.c`
- `docs/history/CHROMIUM_ATOMS_PROCESS_IPC_GAP_ANALYSIS.md`
- `docs/architecture/atrix_chromium_integration_readiness.md`

## FreeBSD

- FreeBSD 15.1 release information: https://www.freebsd.org/releases/15.1R/
- FreeBSD Architecture Handbook: https://docs.freebsd.org/en/books/arch-handbook/book/
- FreeBSD security/Capsicum documentation: https://docs.freebsd.org/en/books/handbook/security/
- FreeBSD licensing policy: https://docs.freebsd.org/en/articles/license-guide/
- FreeBSD release/security support: https://www.freebsd.org/security/

## FreeBSD Chromium

- Current Chromium port Makefile: https://cgit.freebsd.org/ports/tree/www/chromium/Makefile
- Current Chromium FreeBSD patch set: https://cgit.freebsd.org/ports/tree/www/chromium/files/
- FreeBSD Chromium sandbox patch: https://cgit.freebsd.org/ports/tree/www/chromium/files/patch-sandbox_policy_freebsd_sandbox__freebsd.cc

## Chromium

- Chromium platform build configuration: https://chromium.googlesource.com/chromium/src/+/HEAD/build/build_config.h
- Chromium third-party licensing guidance: https://chromium.googlesource.com/chromium/src/+/main/docs/adding_to_third_party.md

---

# 27. Phase 0 Stop Condition

**STOP HERE.**

No ABCE implementation, FreeBSD source import, Chromium source import, BOS modification, VM creation or Phase 1 execution is authorized by this Phase 0 report.

The next action, if Phase 0 is accepted, is to review the blockers above and explicitly approve the **ABCE Phase 1A ABI proof** before any implementation begins.
