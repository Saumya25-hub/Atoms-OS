# ATOMS OS / SignaturesOS Architecture Audit Handbook

**Document:** [`OS_ARCHITECTURE_AUDIT.md`](OS_ARCHITECTURE_AUDIT.md)  
**Repository root:** [`d:/Signatures_OS`](.)  
**Primary evidence:** [`README.md`](README.md), [`CURRENT_TARGET.md`](CURRENT_TARGET.md), [`build.ps1`](build.ps1), [`kernel_main()`](kernel/kernel.c:621), [`boot.asm`](boot/boot.asm), [`stage2.asm`](boot/stage2.asm), [`kernel_entry.asm`](kernel/kernel_entry.asm), [`kernel/linker.ld`](kernel/linker.ld), [`userspace/linker.ld`](userspace/linker.ld)

---

## 1. Executive Summary

ATOMS OS / SignaturesOS is a custom x86_64 operating system with a BIOS boot chain, protected-mode and long-mode transition, freestanding C kernel, VBE framebuffer graphics, a large ring-0 desktop/windowing stack, filesystem support with FAT32 and production-oriented NTFS read-only functionality, a scheduler/syscall/user-mode path, IPC/shared-memory infrastructure, security/TLS and sandbox subsystems, an audio stack targeting AC'97 playback, a browser engine, OpenGL-compatible software rasterization, and multiple desktop/user applications.

The project is not a small kernel demo. It is closer to an experimental monolithic OS platform where many services that would normally live in user space currently run in ring 0: shell, desktop, compositor, browser engine, application launcher, GUI apps, display quality engines, audio service, network/browser layers, security, sandbox certification tests, and diagnostics.

The current repository status is mixed:

| Area | Status | Evidence |
|---|---:|---|
| Boot chain | Implemented | [`boot.asm`](boot/boot.asm), [`stage2.asm`](boot/stage2.asm), [`kernel_entry.asm`](kernel/kernel_entry.asm) |
| Kernel entry/init | Implemented, actively evolving | [`kernel_main()`](kernel/kernel.c:621) |
| Memory managers | Implemented foundation | [`pmm.h`](kernel/core/memory/pmm/include/pmm.h), [`vmm.h`](kernel/core/memory/vmm/include/vmm.h), [`heap.h`](kernel/core/memory/heap/include/heap.h) |
| Interrupts/timer | Implemented foundation | [`idt_init()`](arch/x86_64/interrupt/idt.h:4), [`irq_init()`](kernel/core/interrupt/include/irq.h:9), [`timer_init()`](kernel/core/timer/include/timer.h:11) |
| Scheduler | Implemented cooperative/preemptive foundation | [`scheduler.h`](kernel/core/scheduler/include/scheduler.h) |
| Syscalls | Implemented table and wrappers | [`syscall.h`](kernel/core/syscall/include/syscall.h) |
| VFS/FAT32/NTFS | Implemented VFS and advanced NTFS read-only APIs | [`vfs.h`](kernel/vfs/vfs_legacy/include/vfs.h), [`ntfs.h`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) |
| Graphics/windowing | Large implemented/experimental stack | [`bwe.h`](kernel/wm/bwe/include/bwe.h), [`surface.h`](kernel/wm/surface/surface.h), [`bocompositor.h`](kernel/wm/compositor/bocompositor.h) |
| Display train/presentation | Implemented/frozen architecture with active integration | [`agdte.h`](kernel/graphics/AGDTE/include/agdte.h), [`bspe.h`](kernel/graphics/BSPE/include/bspe.h) |
| Input | Multiple generations coexisting | [`input.h`](kernel/drivers/input/input.h), [`input_core.h`](kernel/drivers/input/core/input_core.h), [`dispatcher.h`](kernel/drivers/input/dispatcher/dispatcher.h) |
| Audio | V3 framework with HAL/mixer/AC97 integration | [`audio_api.h`](kernel/audio/api/audio_api.h), [`audio_hal.h`](kernel/audio/hal/audio_hal.h), [`audio_mixer.h`](kernel/audio/mixer/audio_mixer.h) |
| IPC/shared memory | Phase 2 production-style API | [`ipc_api.h`](kernel/ipc/include/ipc_api.h) |
| Security/TLS | Phase 3 production-style architecture and tests | [`kernel/security/README.md`](kernel/security/README.md), [`bos_security.h`](kernel/security/include/bos_security.h) |
| Sandbox | Phase 4 capability/process sandbox API | [`bos_sandbox.h`](kernel/sandbox/include/bos_sandbox.h) |
| Browser | Extensive in-kernel ABE browser engine | [`kernel/browser_engine`](kernel/browser_engine) |
| Userspace | Init/test/shell/libs/apps/DOOM/Atoms/BishopMath | [`userspace`](userspace), [`userspace/linker.ld`](userspace/linker.ld) |

---

## 2. Repository Topology

### 2.1 Root-Level Directories

| Directory | Purpose | Main dependencies | Status / debt |
|---|---|---|---|
| [`arch`](arch) | x86_64 hardware abstraction: CPU features, GDT, IDT, ISR stubs, port I/O | Kernel core, boot entry | Active and essential |
| [`boot`](boot) | Stage 1 and Stage 2 BIOS bootloader | BIOS, disk geometry, VBE, E820 | Active and essential |
| [`kernel`](kernel) | Monolithic kernel and ring-0 OS services | All core subsystems | Main implementation area |
| [`drivers`](drivers) | Legacy low-level hardware drivers | PIC/PIT/VGA/PS2/VMMouse | Coexists with [`kernel/drivers`](kernel/drivers) |
| [`bovisual`](bovisual) | Visual drawing/UI primitives | VBE framebuffer, BWE | Active graphics foundation |
| [`userspace`](userspace) | Ring-3 programs, libraries, apps, language runtimes | Syscalls, VFS, GUI syscalls | Active but kernel still hosts many apps |
| [`tools`](tools) | Image builders, asset generators, NTFS tooling, verification helpers | Host Windows/PowerShell/Python/Clang | Essential build/support tooling |
| [`assets`](assets) | Icons, DOOM WAD, graphical resources | BOAsset, apps, image tools | Active asset source |
| [`docs`](docs) | Subsystem documentation and forensic reports | N/A | Valuable architecture evidence |
| [`doc`](doc) | Phase reports | N/A | Historical planning docs |
| [`BUGS`](BUGS) | Audit reports and bug investigations | N/A | Important forensic knowledge base |
| [`sdk`](sdk) | Public SDK headers for apps/libraries | SLL, ABE | Developer-facing API surface |
| [`sds`](sds) | Diagnostics framework used by shell/userspace | Shell, libbos | Linked into shell build |
| [`MUSIC`](MUSIC) | Demo audio assets | Audio player/VFS | Contains [`DEMO1.wav`](MUSIC/DEMO1.wav) |
| [`WALLPAPER`](WALLPAPER) | Wallpaper assets | BOPAWN/wallpaper engine | Active desktop personalization assets |
| [`photo`](photo) | Image assets | Image/wallpaper tooling | Asset area |
| [`boot_sound`](boot_sound) | Boot audio assets | Audio stack | Asset area |
| [`AI_BRIDGE`](AI_BRIDGE) | AI/handoff metadata | N/A | Project support |
| [`Recovery_Step18_Archive`](Recovery_Step18_Archive) | Recovery/archive snapshot | N/A | Historical preservation |
| [`capstone_src`](capstone_src) | Capstone source archive/external code | Build/debug tooling | External source area |

### 2.2 Important Root Files

| File | Purpose |
|---|---|
| [`build.ps1`](build.ps1) | Main build pipeline for bootloader, kernel, userspace ELFs, image generation, VM artifacts |
| [`run_os.ps1`](run_os.ps1) | QEMU launch script for OS image |
| [`run_test.ps1`](run_test.ps1) | QEMU test launch, including optional NTFS/USB/audio testing |
| [`CURRENT_TARGET.md`](CURRENT_TARGET.md) | Current milestone: Phase 16 Rook Engine boot splash/page navigation |
| [`README.md`](README.md) | Release status and NTFS validation matrix |
| [`MICRO_STUTTER_FORENSIC_AUDIT.md`](MICRO_STUTTER_FORENSIC_AUDIT.md) | Display/input performance forensic analysis |
| [`Mouse_Forensic_Audit.md`](Mouse_Forensic_Audit.md) | Mouse/input forensic analysis |
| [`tree.txt`](tree.txt) | Repository tree snapshot |
| [`SignaturesOS.vmx`](SignaturesOS.vmx) | VMware virtual machine configuration |

---

## 3. Build System Architecture

### 3.1 Toolchain

The primary build entry point is [`build.ps1`](build.ps1). It uses:

| Tool | Purpose |
|---|---|
| NASM | Assembles [`boot.asm`](boot/boot.asm), [`stage2.asm`](boot/stage2.asm), [`kernel_entry.asm`](kernel/kernel_entry.asm), ISR/context/syscall assembly |
| Clang | Freestanding C compilation for kernel and userspace |
| LLD | Kernel and userspace linking |
| PowerShell | Build orchestration, validation, VM artifact generation |
| VBoxManage | Optional raw-to-VDI/VMDK conversion |
| QEMU/VMware/VirtualBox | Runtime execution environments |

The kernel is linked as a flat binary using [`kernel/linker.ld`](kernel/linker.ld), loaded at physical address `0x100000`. Userspace ELFs are linked at `0x40000000` using [`userspace/linker.ld`](userspace/linker.ld).

### 3.2 Build Order

The build flow is:

```text
Host PowerShell
  -> Assemble stage 1 boot sector
  -> Compile/assemble kernel objects
  -> Link kernel flat binary
  -> Measure kernel binary size
  -> Reassemble stage 2 with exact kernel sector count
  -> Pad kernel to sector boundary
  -> Build userspace ELFs
  -> Build image_builder.exe
  -> Create OS.img from boot + stage2 + kernel
  -> Validate boot signature and image geometry
  -> Optionally convert to VDI/VMDK
```

### 3.3 Kernel Build Characteristics

Kernel compilation is freestanding and mostly avoids platform ABI dependencies:

| Setting | Meaning |
|---|---|
| `-ffreestanding` | No hosted C runtime assumptions |
| `-fno-stack-protector` | No compiler stack protector runtime dependency |
| `-fno-pic` | Flat/non-PIE style kernel binary |
| `-mno-red-zone` | Interrupt-safe x86_64 kernel stack behavior |
| `-mno-sse`, `-mno-sse2`, `-mno-mmx`, `-msoft-float` | Avoids requiring SIMD/FPU in most kernel code |

Selected graphics/browser/OpenGL components are compiled with SSE/SSE2 where needed by the software rasterizer and layout/math code.

### 3.4 Build-System Technical Debt

| Issue | Impact | Recommendation |
|---|---|---|
| Static kernel sector constant and dynamic required sector count coexist | Validation drift risk if `$KERNEL_SECTORS` differs from computed required count | Normalize all validation and image sizing to computed required sectors |
| Many compile commands in one script | Hard to reason about dependency graph | Generate object list from manifest or split into module build manifests |
| Ring-0 services compiled into one binary | Fast iteration but large kernel blast radius | Gradually move apps/services to userspace where syscall ABI permits |
| Duplicate subsystem generations | Build can include old and new architecture simultaneously | Maintain module ownership matrix and deprecation list |

---

## 4. Boot Process

### 4.1 Stage 1 Boot Sector

[`boot.asm`](boot/boot.asm) is the BIOS boot sector. Its responsibilities are:

1. Execute at BIOS boot address.
2. Preserve BIOS boot drive value from `DL`.
3. Use INT 13h extended disk reads.
4. Load Stage 2 sectors.
5. Transfer control to Stage 2.
6. Provide boot signature `0xAA55`.

### 4.2 Stage 2 Loader

[`stage2.asm`](boot/stage2.asm) is responsible for turning a BIOS-loaded disk image into a 64-bit kernel environment.

Stage 2 responsibilities:

| Step | Responsibility |
|---:|---|
| 1 | Load kernel sectors from disk into [`KERNEL_EXEC`](boot/stage2.asm) address |
| 2 | Query physical memory map through BIOS E820 |
| 3 | Detect/select VBE framebuffer mode |
| 4 | Populate boot info structure with framebuffer width, height, pitch, bpp, address |
| 5 | Build/load GDT |
| 6 | Enter protected mode |
| 7 | Set up paging/long-mode prerequisites |
| 8 | Enable long mode |
| 9 | Jump to 64-bit kernel entry with boot info pointer in `RDI` |

### 4.3 Kernel Entry

[`kernel_entry.asm`](kernel/kernel_entry.asm) is the 64-bit entry stub. It disables interrupts, preserves boot info, prepares the early execution environment, and calls [`kernel_main()`](kernel/kernel.c:621).

### 4.4 Memory Layout

| Region | Address/Source | Purpose |
|---|---:|---|
| Kernel load address | `0x100000` from [`kernel/linker.ld`](kernel/linker.ld:7) | Kernel flat binary base |
| Kernel text | [`kernel/linker.ld`](kernel/linker.ld:9) | Code section |
| Kernel rodata | [`kernel/linker.ld`](kernel/linker.ld:15) | Constants/string tables |
| Kernel data | [`kernel/linker.ld`](kernel/linker.ld:20) | Initialized globals |
| Kernel bss | [`kernel/linker.ld`](kernel/linker.ld:25) | Zero-initialized globals |
| Userspace base | `0x40000000` from [`userspace/linker.ld`](userspace/linker.ld:4) | ELF virtual base |
| User stack top | `0x00007FFFFFFFE000` from [`process_builder.h`](kernel/core/process/include/process_builder.h:8) | Ring-3 stack top |
| GUI backbuffer | `0x90000000` allocated in [`kernel_main()`](kernel/kernel.c:931) | BOVISUAL backbuffer |

---

## 5. Kernel Initialization Flow

The authoritative runtime startup order is [`kernel_main()`](kernel/kernel.c:621). In normal boot mode it performs:

```text
kernel_main(boot_info)
  -> cpu_features_init
  -> console/display init
  -> gdt_init
  -> IDT / ISR / exception / PIC / IRQ init
  -> input init: vizier, kernel_input, keyboard, optional BMDE
  -> pmm_init
  -> vmm_init
  -> vbe_init
  -> heap_init
  -> pci_init, e1000_init, usb_registry/core/hid, xhci_init
  -> BOImage init
  -> syscall_init
  -> conhost_init
  -> disk_manager_init, vfs_init, fat32_init, ntfs_init, ntfs_run_tests
  -> mount root filesystem
  -> BOAsset + BOFont init
  -> context_init, scheduler_init, timer_init
  -> AME_Init
  -> audio_init, audio_mixer_init, audio_hal_init, AudioSvc task
  -> AGDPE, VBE display driver, DIE, AGDAE, BDCE
  -> loader tests, IPC tests, security tests, sandbox tests
  -> rook init and pages
  -> allocate GUI backbuffer
  -> BOVISUAL + cursor init
  -> Identity, BOTHEME, BWE, Desktop Shell, Start/Login
  -> Horse Engine
  -> AGDTE
  -> BWE_Compose
  -> scheduler_register_boot_task
  -> sti
  -> main GUI/audio/input/display loop
```

### 5.1 Main Loop

The normal boot main loop in [`kernel_main()`](kernel/kernel.c:1051) coordinates:

| Action | Function / subsystem |
|---|---|
| Audio telemetry | [`audio_player_is_playing()`](kernel/kernel.c:1054) and AC97 forensic hooks |
| Runtime telemetry | [`print_1sec_telemetry()`](kernel/kernel.c:382) |
| USB polling | [`xhci_poll()`](kernel/kernel.c:1088) |
| Input adaptation | [`input_adapter_pump()`](kernel/kernel.c:1094) |
| BWE events | [`BWE_PumpEvents()`](kernel/kernel.c:1096) |
| Fast cursor path | [`BSPE_CursorPresenter_PumpFastPath()`](kernel/kernel.c:1099) |
| Frame pacing | 16ms frame deadline logic in [`kernel_main()`](kernel/kernel.c:1107) |
| Scheduler yield/idle | [`scheduler_yield()`](kernel/kernel.c:1150), `hlt` idle |
| GUI heartbeat | [`BOHeart_Pulse()`](kernel/kernel.c:1163) |
| Display train pulse | [`AGDTE_Pulse()`](kernel/kernel.c:1166) |
| Debug dump | [`bodebug_dump()`](kernel/kernel.c:1174) |

### 5.2 Shutdown Flow

A complete shutdown path is not centralized. Available shutdown hooks exist in subsystems such as [`BOCompositor_Shutdown()`](kernel/wm/compositor/bocompositor.h:20), [`audio_shutdown()`](kernel/audio/api/audio_api.h:9), [`audio_hal_shutdown()`](kernel/audio/hal/audio_hal.h:54), [`BOFont_Shutdown()`](kernel/ui/bofont/bofont.h:19), [`BOAsset_Shutdown()`](kernel/ui/boasset/boasset.h:8), [`AGDTE_Shutdown()`](kernel/graphics/AGDTE/include/agdte.h:272), [`BOGE_Shutdown()`](kernel/graphics/BOGE/include/boge.h:67), [`bos_ipc_shutdown()`](kernel/ipc/include/ipc_api.h:18), [`bos_loader_shutdown()`](kernel/loader/include/loader_api.h:14), and [`horse_shutdown()`](kernel/engine/horse_engine.h:41). The OS needs a formal shutdown orchestrator to invoke these in reverse dependency order.

---

## 6. Core Kernel Architecture

### 6.1 Architecture Layer

[`arch`](arch) contains low-level CPU/platform code:

| Component | Purpose |
|---|---|
| [`arch/x86_64/cpu`](arch/x86_64/cpu) | CPU feature detection |
| [`arch/x86_64/gdt`](arch/x86_64/gdt) | GDT/TSS setup and flush |
| [`arch/x86_64/interrupt`](arch/x86_64/interrupt) | IDT and ISR stubs |
| [`arch/x86_64/io`](arch/x86_64/io) | Port I/O wrappers |

Public entry points include [`idt_init()`](arch/x86_64/interrupt/idt.h:4), [`idt_set_gate()`](arch/x86_64/interrupt/idt.h:5), and GDT functions compiled into the kernel build.

### 6.2 Interrupts

Interrupt architecture layers:

| Layer | Responsibility |
|---|---|
| IDT | Vector table setup through [`idt_init()`](arch/x86_64/interrupt/idt.h:4) |
| ISR | CPU exception/interrupt entry stubs and register frame capture |
| Exception manager | High-level CPU exception handling |
| PIC | Legacy 8259 remap/masking/EOI |
| IRQ manager | IRQ handler registration and dispatch via [`irq_dispatch()`](kernel/core/interrupt/include/irq.h:18) |
| Timer | PIT/timer driver and scheduler tick via [`timer_init()`](kernel/core/timer/include/timer.h:11) |

### 6.3 Memory Managers

#### Physical Memory Manager

[`pmm.h`](kernel/core/memory/pmm/include/pmm.h) exposes page allocation/free and memory accounting. It initializes from the Stage 2 E820 boot info through [`pmm_init()`](kernel/core/memory/pmm/include/pmm.h:10).

#### Virtual Memory Manager

[`vmm.h`](kernel/core/memory/vmm/include/vmm.h) owns page table mapping/unmapping, combined PMM+VMM page allocation, address space creation, switching, and translation.

#### Kernel Heap

[`heap.h`](kernel/core/memory/heap/include/heap.h) explicitly states the kernel heap does not own memory and must not call PMM directly. It manages allocations over memory supplied by VMM. It includes canaries, request sizes, caller RIP tracking, validation, heap walking, tracing, and the BMLE large-allocation telemetry layer.

### 6.4 Scheduler and Tasks

[`scheduler.h`](kernel/core/scheduler/include/scheduler.h) defines scheduler lifecycle, task creation, task termination, sleeping, yielding, boot-task registration, and diagnostics. [`runqueue.h`](kernel/core/scheduler/include/runqueue.h) defines an intrusive-list run queue with magic checking.

Current ownership model:

| Owner | Owns |
|---|---|
| Scheduler | Runnable task list, current task pointer |
| Task struct | CPU context, stack pointer, task metadata |
| Timer IRQ | Scheduler tick callback path |
| Kernel main loop | GUI pacing and cooperative scheduler yielding |

### 6.5 Syscalls and User Mode

[`syscall.h`](kernel/core/syscall/include/syscall.h) defines syscall IDs from yielding and I/O through filesystem, GUI, surface presentation, and input event polling. The syscall assembly path uses dedicated stubs in [`kernel/core/syscall/src`](kernel/core/syscall/src). Ring-3 entry uses [`enter_usermode()`](kernel/core/process/include/enter_usermode.h:7). User stack construction is controlled by [`process_build_user_stack()`](kernel/core/process/include/process_builder.h:11).

### 6.6 Loader, IPC, Security, Sandbox

| Subsystem | Public API | Purpose |
|---|---|---|
| Dynamic loader | [`loader_api.h`](kernel/loader/include/loader_api.h) | `dlopen`/`dlsym`/`dlclose` style dynamic library runtime |
| IPC | [`ipc_api.h`](kernel/ipc/include/ipc_api.h) | Channels, shared memory, pipes, ports, router/broadcast APIs |
| Security/TLS | [`bos_security.h`](kernel/security/include/bos_security.h) | CSPRNG, hash, AES, RSA, ECC, X.509, trust store, TLS/session engine |
| Sandbox | [`bos_sandbox.h`](kernel/sandbox/include/bos_sandbox.h) | Capability, token, syscall, process, memory, filesystem, IPC, network policies |

---

## 7. Filesystem and Storage Architecture

### 7.1 Storage Stack

```text
ATA / disk hardware
  -> ATA driver
  -> Block device registry
  -> MBR partition manager
  -> Disk manager
  -> VFS mount manager
  -> FAT32 / NTFS drivers
  -> Syscalls / shell / apps
```

Storage directories:

| Directory | Purpose |
|---|---|
| [`kernel/drivers/storage_legacy`](kernel/drivers/storage_legacy) | ATA legacy storage driver |
| [`kernel/vfs/vfs_legacy/storage`](kernel/vfs/vfs_legacy/storage) | Block device, disk manager, MBR |
| [`kernel/vfs/vfs_legacy`](kernel/vfs/vfs_legacy) | VFS registry, mount manager, open/read/write/readdir APIs |
| [`kernel/vfs/vfs_legacy/fs/fat32`](kernel/vfs/vfs_legacy/fs/fat32) | FAT32 driver |
| [`kernel/vfs/vfs_legacy/fs/ntfs`](kernel/vfs/vfs_legacy/fs/ntfs) | NTFS read-only production driver |

### 7.2 VFS API

[`FilesystemDriver`](kernel/vfs/vfs_legacy/include/vfs.h:17) requires mount, open, read, write, close, readdir, mkdir, create, rename, and delete callbacks. Public VFS calls include [`vfs_mount_fs()`](kernel/vfs/vfs_legacy/include/vfs.h:43), [`vfs_open()`](kernel/vfs/vfs_legacy/include/vfs.h:51), [`vfs_read()`](kernel/vfs/vfs_legacy/include/vfs.h:52), [`vfs_write()`](kernel/vfs/vfs_legacy/include/vfs.h:53), [`vfs_readdir()`](kernel/vfs/vfs_legacy/include/vfs.h:57), and mutation APIs.

### 7.3 NTFS Engine

[`ntfs.h`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h) defines a multi-phase NTFS architecture:

| Phase | Engine | Public functions |
|---|---|---|
| Phase 1 | BPB/volume mount validation | [`ntfs_init()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:426), [`ntfs_mount()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:427) |
| Phase 2 | MFT core engine | [`ntfs_mft_read_record()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:436), [`ntfs_mft_apply_fixup()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:438) |
| Phase 3 | Attribute engine | [`ntfs_attr_find()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:443), [`ntfs_decode_data_runs()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:445) |
| Phase 4 | File read engine | [`ntfs_file_open_by_record()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:451), [`ntfs_file_read()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:453) |
| Phase 5 | Directory/index engine | [`ntfs_dir_lookup_entry()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:460), [`ntfs_resolve_path()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:462) |
| Phase 7 | Cache/performance diagnostics | [`ntfs_mft_cache_init()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:466), [`ntfs_path_cache_init()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:468) |

The README claims successful validation against real Windows XP NTFS media including MFT, USA fixups, resident/non-resident files, long filenames, deep traversal, large directories, index allocation, fragmented runlists, and a 200 MB fragmented file.

---

## 8. Graphics, Windowing, Display, and UI

### 8.1 Graphics Stack Overview

```text
Application / Desktop / Browser / Graph3D
  -> Horse Engine / Shell app launch
  -> BWE / BOSurface window tree
  -> BOCompositor / BOGE / BOVISUAL / OpenGL-BGL
  -> BSPE presentation / AGDTE display train
  -> AGDPE / VBE backend / physical framebuffer
```

### 8.2 BWE and BOSurface

BWE is the main window/control API, while BOSurface is the surface tree and compatibility layer. Important functions include [`BWE_Initialize()`](kernel/wm/bwe/include/bwe.h:160), [`BOS_CreateSurface()`](kernel/wm/bwe/include/bwe.h:161), [`BOS_CreateWindow()`](kernel/wm/bwe/include/bwe.h:163), [`BOS_Show()`](kernel/wm/bwe/include/bwe.h:164), and [`BOS_SurfacePresent()`](kernel/wm/bwe/include/bwe.h:256).

BOSurface tracks parent/child relationships, Z-order, focus, drag, hover, wallpaper, taskbar, icons, controls, and rendering hooks. It integrates with BOCompositor and BOVISUAL.

### 8.3 BOCompositor

[`bocompositor.h`](kernel/wm/compositor/bocompositor.h) defines a decoupled compositor that only understands rectangles, IDs, visibility, focus flags, z-order, damage, clipping, and render callbacks. It initializes static subsystem pools and is a clean boundary between window logic and composition logic.

Composition flow:

```text
BWE/BOSurface invalidates region
  -> BOCompositor damage tracker
  -> sorted visible surface stack
  -> occlusion evaluation
  -> clip push
  -> render callback into BWE/BOSurface
  -> clip pop
  -> clear damage
```

### 8.4 BOVISUAL, BOImage, BOFont, BOAsset

| Engine | Purpose | API |
|---|---|---|
| BOVISUAL | Framebuffer drawing, text, primitives | [`bovisual`](bovisual) |
| BOImage | Image decoding/textures/atlas/batching/quality | [`boimage.h`](kernel/ui/boimage/boimage.h) |
| BOFont | Typography engine, atlases, glyph cache, text layout | [`bofont.h`](kernel/ui/bofont/bofont.h) |
| BOAsset | Asset lifecycle/cache/preload | [`boasset.h`](kernel/ui/boasset/boasset.h) |

### 8.5 AGDPE, DIE, AGDAE, BDCE

| Engine | Directory | Purpose |
|---|---|---|
| AGDPE | [`kernel/display/agdpe`](kernel/display/agdpe) | Hardware display platform HAL for VBE and future GPU backends |
| DIE | [`kernel/display`](kernel/display) | Display Intelligence Engine: capabilities, detection, layout, runtime, policy |
| AGDAE | [`kernel/display/agdae`](kernel/display/agdae) | Display adaptation/scaling/geometry engine |
| BDCE | [`kernel/display/bdce`](kernel/display/bdce) | Boot/display configuration authority and validation |

[`AGDPE_DisplayDevice`](kernel/display/agdpe/agdpe.h:34) abstracts physical display state, capabilities, framebuffer, swap, mode-setting, and vsync.

### 8.6 BSPE

BSPE is the BOS Surface Presentation Engine. [`kernel/graphics/BSPE/README.md`](kernel/graphics/BSPE/README.md) defines it as a pure presentation engine that never inspects UI layout or draws primitives. Responsibilities:

| Module | Responsibility |
|---|---|
| [`Present`](kernel/graphics/BSPE/Present) | Present queue and presentation loop |
| [`Swapchain`](kernel/graphics/BSPE/Swapchain) | Double/triple buffering |
| [`Damage`](kernel/graphics/BSPE/Damage) | Dual-page damage history |
| [`FramePacer`](kernel/graphics/BSPE/FramePacer) | VSync/timer pacing |
| [`Cursor`](kernel/graphics/BSPE/Cursor) | Hardware cursor plane and software fallback |
| [`DisplayHAL`](kernel/graphics/BSPE/DisplayHAL) | Driver abstraction |
| [`Drivers`](kernel/graphics/BSPE/Drivers) | VBE/VESA/VirtIO/Bochs backends |
| [`Debug`](kernel/graphics/BSPE/Debug) | Telemetry HUD |

### 8.7 AGDTE

AGDTE is the master display train controller. [`agdte.h`](kernel/graphics/AGDTE/include/agdte.h) explicitly says it is not another renderer/compositor/library; every rendered frame must pass through AGDTE before monitor presentation. It enforces zero heap allocations, fixed pools, deterministic integer timing, no blocking loops, no unnecessary copies, and single authoritative buffer ownership.

Major AGDTE modules:

| Module | Responsibility |
|---|---|
| Surface manager | Managed display surfaces/layers |
| Buffer manager | Buffer IDs, roles, ownership states |
| Display state | Active displays, cadence, backend |
| Timing engine | Submit/deadline/presentation timestamps |
| Scheduler | Present-now/wait/skip/merge decisions |
| Presenter | Execute present requests |
| Diagnostics | Latency, decisions, pulse time, present time |
| Quality Engine | Stabilizer, motion analyzer, dirty optimizer, cadence optimizer |

### 8.8 OpenGL/BGL/Graph 3D

The OpenGL stack is a software rasterizer documented in [`OPENGL_MASTER_CONTEXT.md`](docs/opengl/OPENGL_MASTER_CONTEXT.md). Pipeline:

```text
App / ATOMS Graph 3D
  -> Horse Engine
  -> BWE window
  -> BGL drawable + BGL context
  -> OpenGL API layer
  -> GL state machine
  -> transform/clip/viewport
  -> rasterization/fragment/depth/stencil/texture/blend
  -> drawable color buffer
  -> BWE canvas/window surface
  -> compositor/BSPE/AGDTE
```

Implemented phases include fixed-function geometry, perspective/frustum clipping, depth/alpha, lighting, textures, display lists, vertex arrays, mipmapping, readback, stencil, polygon offset, face culling, FBO/RBO render-to-texture, and ATOMS Graph 3D benchmark.

---

## 9. Input Architecture

### 9.1 Input Generations

There are several input layers coexisting:

| Layer | File/dir | Purpose |
|---|---|---|
| Legacy kernel input queue | [`input.h`](kernel/drivers/input/input.h) | Simple BVEvent queue for mouse/key events |
| Input abstraction | [`input_abstraction.h`](kernel/drivers/input/input_abstraction.h) | Converts absolute/relative sources into unified absolute state |
| Input Core V2 | [`input_core.h`](kernel/drivers/input/core/input_core.h) | Standardized immutable events and priority consumers |
| Dispatcher | [`dispatcher.h`](kernel/drivers/input/dispatcher/dispatcher.h) | Producer-to-consumer backbone with queue/filter/router/priority/diagnostics |
| Pointer Engine | [`pointer_engine.h`](kernel/drivers/input/pointer/pointer_engine.h) | Single authoritative pointer state after Input Core |
| Mouse Engine | [`mouse_engine.h`](kernel/drivers/input/mouse_engine/mouse_engine.h) | Raw packet ingestion and pointer preparation |
| Cursor Engine | [`cursor_engine.h`](kernel/drivers/input/cursor/cursor_engine.h) | Visual cursor presentation coordinator |
| Keyboard | [`keyboard.h`](kernel/drivers/keyboard/include/keyboard.h) | IRQ1 scancode/key event manager |

### 9.2 Input Data Flow

```text
PS/2 / USB HID / VMware mouse / keyboard IRQ or poll
  -> raw device driver
  -> input_abstraction or input_core event creation
  -> pointer engine / mouse engine / keyboard manager
  -> dispatcher priority consumers
  -> cursor engine and BWE hit testing
  -> focused window/control
  -> userspace app via syscalls or BWE process queue
```

### 9.3 Thread and Memory Ownership

| Item | Owner |
|---|---|
| IRQ packets | Device driver/IRQ handler until queued |
| Kernel event queue | Input subsystem static queue |
| Pointer position | Pointer Engine / input abstraction authoritative state |
| Cursor sprite state | Cursor Engine / BSPE cursor presenter |
| Window focus | BWE/BOSurface focus engine |
| User app event queue | BWE process queue/syscall layer |

### 9.4 Input Debt

| Issue | Risk | Recommendation |
|---|---|---|
| Multiple active input generations | Confusing routing and duplicated state | Declare one canonical pipeline and bridge legacy APIs through it |
| Polling and IRQ paths coexist | Latency/race risk | Document producer timing and introduce lock-free invariants |
| Many global telemetry counters | Non-atomic data races possible | Use atomic counters or interrupt-safe snapshots |
| Cursor fast path bypasses normal frame cadence | Smoothness gain but consistency risk | Maintain explicit damage/ownership rules with BSPE/AGDTE |

---

## 10. Audio Architecture

### 10.1 Audio Stack

```text
Application / audio player / WAV asset
  -> audio_stream API
  -> AudioRingBuffer
  -> audio_mixer_process
  -> audio_hal
  -> AC97 driver / DMA buffer
  -> hardware playback
```

### 10.2 Components

| Directory | Purpose |
|---|---|
| [`kernel/audio/api`](kernel/audio/api) | Public audio subsystem and stream API |
| [`kernel/audio/core`](kernel/audio/core) | Stream registry and realtime worker |
| [`kernel/audio/streams`](kernel/audio/streams) | Audio stream objects and ring buffers |
| [`kernel/audio/mixer`](kernel/audio/mixer) | Software mixer and mix math |
| [`kernel/audio/hal`](kernel/audio/hal) | Hardware abstraction and driver registry |
| [`kernel/audio/drivers/ac97`](kernel/audio/drivers/ac97) | AC97 codec, DMA, playback, registers |
| [`kernel/audio/session`](kernel/audio/session) | Audio player and producer worker |
| [`kernel/audio/formats`](kernel/audio/formats) | PCM format handling |
| [`kernel/audio/volume`](kernel/audio/volume) | Volume control |
| [`kernel/audio/diagnostics`](kernel/audio/diagnostics) | Debug/test/diagnostic modes |
| [`kernel/audio/forensic`](kernel/audio/forensic) | Forensic session dumping |

### 10.3 Runtime Integration

[`kernel_main()`](kernel/kernel.c:833) initializes audio during normal boot with [`audio_init()`](kernel/audio/api/audio_api.h:8), [`audio_mixer_init()`](kernel/audio/mixer/audio_mixer.h:8), and [`audio_hal_init()`](kernel/audio/hal/audio_hal.h:39), then spawns an `AudioSvc` kernel task via [`scheduler_create_kernel_task()`](kernel/core/scheduler/include/scheduler.h:11). The service calls `audio_player_update()` and sleeps for 20 ms.

### 10.4 Audio Debt

| Issue | Impact | Recommendation |
|---|---|---|
| README says foundation only, but build includes HAL/AC97/DMA modules | Documentation drift | Update audio README with V3 actual runtime status |
| Audio service is ring-0 task | Driver bugs can crash kernel | Split media player/decoder into userspace later |
| Telemetry globals | Data race/logging overhead risk | Consolidate into lock-free snapshot structure |
| AC97 focus | Limited hardware support | Add feature flags and fallback/no-device state machine |

---

## 11. Application, Shell, and Desktop Engines

### 11.1 Rook Engine

[`rook.h`](kernel/shell/rook/include/rook.h) defines Rook Engine V1.0 as page navigation and screen management. It owns permanent page IDs, navigation links, lifecycle callbacks, dirty rectangles, backbuffer access, rendering, update, and event dispatch.

Lifecycle callbacks:

| Callback | Purpose |
|---|---|
| `on_create` | Allocate/prepare static page state |
| `on_init` | Initialize internal page resources |
| `on_load` | Load assets |
| `on_enter` | Page becomes active |
| `on_update` | Time-based updates |
| `on_render` | Draw into framebuffer |
| `on_pause` | Temporarily inactive |
| `on_resume` | Return from pause |
| `on_exit` | Leave active state |
| `on_unload` | Release runtime resources |
| `on_destroy` | Permanent teardown |

### 11.2 Desktop Shell

[`desktop_shell.h`](kernel/shell/desktop_shell/desktop_shell.h) manages desktop startup, login experience, notifications, wallpaper, wallpaper transitions, app registry, app launch, and wallpaper rendering.

### 11.3 Horse Engine

[`horse_engine.h`](kernel/engine/horse_engine.h) is the application registry/launcher engine. It defines app IDs for ATOMS, Explorer, Terminal, Calculator, Settings, Sandbox, Stress Test, Music, Image Viewer, DOOM, Input Lab, ATRIX, and Graph 3D. It exposes [`horse_init()`](kernel/engine/horse_engine.h:24), [`horse_register()`](kernel/engine/horse_engine.h:35), [`horse_launch()`](kernel/engine/horse_engine.h:36), focus, shutdown, and restart APIs.

### 11.4 Taskbar, Start Menu, System Hub

| Engine | File | Purpose |
|---|---|---|
| Task Panel | [`task_panel.h`](kernel/ui/task_panel.h) | Tracks top-level windows and focus switching |
| Start Menu | [`start_menu.h`](kernel/ui/start_menu.h) | Application launcher UI |
| System Hub | [`system_hub.h`](kernel/ui/system_hub.h) | Battery/wifi/volume/notification style panels |
| Desktop Grid | [`bomatrix.h`](kernel/shell/desktop_shell/bomatrix.h) | Desktop icon/grid snapping/layout |

---

## 12. Browser, Network, Security, and Sandbox

### 12.1 ABE Browser Engine

[`kernel/browser_engine`](kernel/browser_engine) contains an extensive browser engine:

| Area | Purpose |
|---|---|
| [`api`](kernel/browser_engine/api) | Public ABE API |
| [`core`](kernel/browser_engine/core) | Configuration/features/core lifecycle |
| [`html`](kernel/browser_engine/html) | Tokenizer, parser, document, element, text nodes |
| [`dom`](kernel/browser_engine/dom) | DOM abstraction |
| [`css`](kernel/browser_engine/css) | Tokenizer, parser, selectors, cascade, computed styles |
| [`layout`](kernel/browser_engine/layout) | Box model, block, inline, flex, positioning, render tree, reflow |
| [`javascript`](kernel/browser_engine/javascript) | Lexer, parser, compiler, VM, GC, objects, promises, modules, DOM bindings |
| [`network`](kernel/browser_engine/network) | DNS, HTTP, TLS, redirects, downloads, compression |
| [`web_platform`](kernel/browser_engine/web_platform) | Fetch, XHR, storage, history, navigator, performance, scheduler, observers, blobs, forms |
| [`render`](kernel/browser_engine/render) | Browser rendering integration |
| [`tab`](kernel/browser_engine/tab) | Tab model |
| [`window`](kernel/browser_engine/window) | Browser window integration |

### 12.2 Network Stack

Two network-related trees exist: [`kernel/net`](kernel/net) and [`kernel/network`](kernel/network). [`kernel/net`](kernel/net) contains protocol implementations for Ethernet, ARP, IPv4, ICMP, UDP, DHCP, DNS, TCP, HTTP, TLS, socket manager, firewall/security, and net service. [`kernel/network/network_manager.h`](kernel/network/network_manager.h) defines an enterprise-style interface registry for loopback/ethernet/wifi.

### 12.3 Security/TLS

Security initialization order from [`bos_security.h`](kernel/security/include/bos_security.h):

1. Debug and diagnostics engine.
2. CSPRNG random engine.
3. SHA hash engine.
4. AES engine.
5. RSA engine.
6. ECC engine.
7. X.509 parser.
8. Certificate validator and trust store.
9. TLS and session engine.

### 12.4 Sandbox

Sandbox API in [`bos_sandbox.h`](kernel/sandbox/include/bos_sandbox.h) covers process contexts, capabilities, permission checks, tokens, syscall validation, resource limits, process isolation, and certification tests.

---

## 13. Userspace, SDK, Libraries, and Apps

### 13.1 Userspace Layout

| Directory/File | Purpose |
|---|---|
| [`userspace/init.c`](userspace/init.c) | Init process |
| [`userspace/test.c`](userspace/test.c) | Test process |
| [`userspace/shell`](userspace/shell) | Userspace shell and command parser |
| [`userspace/libbos`](userspace/libbos) | Syscall/disk/display helper library |
| [`userspace/libbos_gui`](userspace/libbos_gui) | GUI syscall wrappers/widgets |
| [`userspace/atoms`](userspace/atoms) | Atoms language compiler/VM/runtime |
| [`userspace/bishopmath`](userspace/bishopmath) | BishopMath numeric/vector/matrix/scientific library |
| [`userspace/apps/doom`](userspace/apps/doom) | DOOM port and mini libc |
| [`userspace/apps/gui_demo`](userspace/apps/gui_demo) | GUI demo app |
| [`userspace/apps/input_lab`](userspace/apps/input_lab) | Input testing app |
| [`userspace/tests`](userspace/tests) | Bishop/BOSL/filesystem tests |

### 13.2 Kernel Apps

[`kernel/apps`](kernel/apps) contains ring-0 desktop apps: ATOMS Graph 3D, ATRIX browser, calculator, files, music, settings, and terminal.

### 13.3 SDK and SDS

[`sdk`](sdk) contains developer-facing headers for ABE and SLL APIs. [`sds`](sds) provides diagnostics, console, core, database, and logger modules linked into the userspace shell build.

---

## 14. Engine Catalog

| Engine | Owner/Path | Purpose | Init/API |
|---|---|---|---|
| Window Engine / BWE | [`kernel/wm/bwe`](kernel/wm/bwe) | Windows, controls, events, geometry, paint | [`BWE_Initialize()`](kernel/wm/bwe/include/bwe.h:160) |
| BOSurface | [`kernel/wm/surface`](kernel/wm/surface) | Surface tree, focus, drag, desktop/window composition | [`BOSurface_Init()`](kernel/wm/surface/surface.h:183) |
| BOCompositor | [`kernel/wm/compositor`](kernel/wm/compositor) | Z-order, damage, clipping, composition callbacks | [`BOCompositor_Initialize()`](kernel/wm/compositor/bocompositor.h:17) |
| Display Engine / AGDPE | [`kernel/display/agdpe`](kernel/display/agdpe) | Physical display HAL | [`AGDPE_Initialize()`](kernel/display/agdpe/agdpe.h:69) |
| Display Intelligence Engine | [`kernel/display`](kernel/display) | Capabilities/detection/layout/runtime/policy | [`DIE_Initialize()`](kernel/kernel.c:872) |
| Display Adaptation Engine | [`kernel/display/agdae`](kernel/display/agdae) | Resolution adaptation and scaling | [`AGDAE_Initialize()`](kernel/kernel.c:878) |
| BDCE | [`kernel/display/bdce`](kernel/display/bdce) | Display config authority/validation | [`BDCE_SeedFromCurrentSystem()`](kernel/kernel.c:881) |
| BSPE | [`kernel/graphics/BSPE`](kernel/graphics/BSPE) | Pure presentation engine | [`BSPE_Initialize()`](kernel/graphics/BSPE/include/bspe.h:81) |
| AGDTE | [`kernel/graphics/AGDTE`](kernel/graphics/AGDTE) | Master display train controller | [`AGDTE_Initialize()`](kernel/graphics/AGDTE/include/agdte.h:271) |
| Display Quality Engine | [`kernel/graphics/AGDTE/quality`](kernel/graphics/AGDTE/quality) | Stabilization, motion, dirty optimization, cadence | [`AGDTE_QualityEngine_Initialize()`](kernel/graphics/AGDTE/quality/quality_engine.h:31) |
| Graphics Engine / BOGE | [`kernel/graphics/BOGE`](kernel/graphics/BOGE) | Surface command queue and staging frames | [`BOGE_Initialize()`](kernel/graphics/BOGE/include/boge.h:66) |
| OpenGL Engine | [`kernel/graphics/gl`](kernel/graphics/gl) | Software OpenGL-compatible renderer | [`kernel/graphics/gl`](kernel/graphics/gl) |
| BGL | [`kernel/graphics/bgl`](kernel/graphics/bgl) | GL drawable/context binding to windows | [`bgl.h`](kernel/graphics/bgl/bgl.h) |
| Motion Engine / AME | [`kernel/ame`](kernel/ame) | Animation tracks/easing/accessibility/reduced motion | [`AME_Init()`](kernel/ame/include/ame.h:57) |
| Animation Engine / BOFLOW | [`kernel/gui/animation`](kernel/gui/animation) | Legacy/foundation animations | [`boflow_init()`](kernel/gui/animation/animation_engine.h:40) |
| Input Engine V2 | [`kernel/drivers/input/core`](kernel/drivers/input/core) | Universal input events and consumers | [`input_core_init()`](kernel/drivers/input/core/input_core.h:103) |
| Dispatcher Engine | [`kernel/drivers/input/dispatcher`](kernel/drivers/input/dispatcher) | Input queue/filter/router/priority backbone | [`dispatcher_init()`](kernel/drivers/input/dispatcher/dispatcher.h:20) |
| Pointer Engine | [`kernel/drivers/input/pointer`](kernel/drivers/input/pointer) | Authoritative pointer state | [`pointer_engine_init()`](kernel/drivers/input/pointer/pointer_engine.h:22) |
| Mouse Engine | [`kernel/drivers/input/mouse_engine`](kernel/drivers/input/mouse_engine) | Raw mouse packet processing | [`mouse_engine_init()`](kernel/drivers/input/mouse_engine/mouse_engine.h:7) |
| Cursor Engine | [`kernel/drivers/input/cursor`](kernel/drivers/input/cursor) | Cursor state/theme/animation/backend/render | [`cursor_engine_init()`](kernel/drivers/input/cursor/cursor_engine.h:29) |
| Keyboard Engine | [`kernel/drivers/keyboard`](kernel/drivers/keyboard) | Scancode/key event manager | [`keyboard_init()`](kernel/drivers/keyboard/include/keyboard.h:52) |
| Taskbar/Task Panel | [`kernel/ui/task_panel.*`](kernel/ui/task_panel.h) | Active task visualization/focus switching | [`TaskPanel_Initialize()`](kernel/ui/task_panel.h:4) |
| Start Menu | [`kernel/ui/start_menu.*`](kernel/ui/start_menu.h) | Launcher UI | [`StartMenu_Initialize()`](kernel/ui/start_menu.h:4) |
| Desktop Shell | [`kernel/shell/desktop_shell`](kernel/shell/desktop_shell) | Desktop, login, wallpaper, app registry | [`Desktop_Shell_Initialize()`](kernel/shell/desktop_shell/desktop_shell.h:7) |
| Shell Engine | [`kernel/shell`](kernel/shell) / [`userspace/shell`](userspace/shell) | Console/shell/app workflow | Multiple |
| Horse Engine | [`kernel/engine`](kernel/engine) | Application registry and launcher | [`horse_init()`](kernel/engine/horse_engine.h:24) |
| Rook Engine | [`kernel/shell/rook`](kernel/shell/rook) | Boot pages and screen navigation | [`rook_init()`](kernel/shell/rook/include/rook.h:82) |
| Bishop Math Engine | [`userspace/bishopmath`](userspace/bishopmath) | Math library/runtime | Userspace library |
| Asset Engine / BOAsset | [`kernel/ui/boasset`](kernel/ui/boasset) | Asset cache/lifecycle/preload | [`BOAsset_Initialize()`](kernel/ui/boasset/boasset.h:7) |
| Image Engine / BOImage | [`kernel/ui/boimage`](kernel/ui/boimage) | Image textures/atlas/sampling/blending | [`BOImage_Init()`](kernel/ui/boimage/boimage.h:54) |
| Wallpaper/Image Engine / BOPAWN | [`kernel/media/bopawn`](kernel/media/bopawn) | Image loading, cache, wallpaper engine | [`bopawn_init()`](kernel/media/bopawn/bopawn.h:12) |
| Filesystem Engine / VFS | [`kernel/vfs`](kernel/vfs) | VFS, mount, FAT32, NTFS | [`vfs_init()`](kernel/vfs/vfs_legacy/include/vfs.h:37) |
| NTFS engines | [`kernel/vfs/vfs_legacy/fs/ntfs`](kernel/vfs/vfs_legacy/fs/ntfs) | MFT, attributes, file read, directory/index, cache | [`ntfs_init()`](kernel/vfs/vfs_legacy/fs/ntfs/include/ntfs.h:426) |
| Audio Engine | [`kernel/audio`](kernel/audio) | Streams, HAL, mixer, AC97, diagnostics | [`audio_init()`](kernel/audio/api/audio_api.h:8) |
| IPC Engine | [`kernel/ipc`](kernel/ipc) | Channels, pipes, ports, shared memory | [`bos_ipc_init()`](kernel/ipc/include/ipc_api.h:17) |
| Loader Engine | [`kernel/loader`](kernel/loader) | Runtime linker, ELF, relocations, symbols | [`bos_loader_init()`](kernel/loader/include/loader_api.h:13) |
| Security Engine | [`kernel/security`](kernel/security) | Crypto, TLS, X.509, trust, sessions | [`bos_security_init()`](kernel/security/include/bos_security.h:31) |
| Sandbox Engine | [`kernel/sandbox`](kernel/sandbox) | Capabilities, process/memory/syscall/resource isolation | [`bos_sandbox_init()`](kernel/sandbox/include/bos_sandbox.h:19) |
| Identity Engine | [`kernel/identity`](kernel/identity) | Authentication/login credentials | [`Identity_Init()`](kernel/identity/include/identity.h:19) |
| Browser Engine / ABE | [`kernel/browser_engine`](kernel/browser_engine) | HTML/CSS/JS/layout/network/web platform | [`kernel/browser_engine`](kernel/browser_engine) |

---

## 15. Dependency Graphs

### 15.1 Boot to Desktop

```text
BIOS
 -> boot.asm
 -> stage2.asm
 -> kernel_entry.asm
 -> kernel_main
 -> CPU/GDT/IDT/IRQ
 -> PMM/VMM/Heap
 -> PCI/USB/Storage/VFS
 -> Syscall/Scheduler/Timer
 -> Assets/Font/Image
 -> Audio Service
 -> Display Engines
 -> Rook Boot Pages
 -> BOVISUAL/BWE/Desktop/Horse
 -> AGDTE + Main Loop
```

### 15.2 Graphics Dependency Graph

```text
Apps / Shell / Browser / Graph3D
 -> BWE/BOSurface
 -> BOCompositor and/or BOGE
 -> BOVISUAL/BOImage/BOFont/OpenGL
 -> BSPE
 -> AGDTE
 -> AGDPE / VBE
 -> Framebuffer
```

### 15.3 Input Dependency Graph

```text
Hardware driver
 -> raw packet/scancode/report
 -> input abstraction / input core
 -> pointer/mouse/keyboard engines
 -> dispatcher/router/filters
 -> cursor engine + BWE hit testing
 -> focused window/control
 -> userspace syscall event read
```

### 15.4 Memory Ownership Flow

```text
E820 memory map
 -> PMM frame bitmap
 -> VMM maps physical frames into virtual spaces
 -> heap manages allocation metadata over VMM-supplied regions
 -> subsystems allocate static pools or kmalloc blocks
 -> apps/user processes use dedicated address spaces and user stacks
```

---

## 16. Known Issues and Technical Debt

| Area | Issue | Severity | Recommendation |
|---|---|---:|---|
| Architecture | Multiple generations of GUI/input/display coexist | High | Freeze canonical pipeline and mark legacy compatibility shims |
| Kernel size | Browser/apps/security/sandbox/GUI all ring-0 | High | Move services to userspace gradually |
| Initialization | [`kernel_main()`](kernel/kernel.c:621) is a very large orchestrator | High | Split into subsystem boot phases and dependency manifests |
| Shutdown | No single reverse-order shutdown orchestrator | Medium | Implement power manager/shutdown manager |
| Build | [`build.ps1`](build.ps1) is monolithic | Medium | Generate build graph from module manifests |
| Memory | Static pools and fixed limits everywhere | Medium | Document capacities and add graceful exhaustion paths |
| Input | Polling, IRQ, fast cursor paths can race | High | Add atomic counters, ownership docs, queue invariants |
| Display | Multiple presentation layers: BOCompositor, BOGE, BSPE, AGDTE | Medium | Define authoritative path for each frame type |
| Audio | Documentation drift between foundation README and V3 implementation | Medium | Update audio docs and state machine diagrams |
| Security | Production-style claims need continuous test evidence | Medium | Store certification logs and expected vectors |
| Filesystem | NTFS read-only advanced, writes not implemented | Medium | Clearly separate read-only certified paths from future write roadmap |
| Userspace | GUI/apps still mostly kernel-resident | High | Prioritize GUI syscalls and userspace app migration |

---

## 17. Completion Matrix Toward Version 1.0

| Milestone | Required for v1.0 | Current status |
|---|---|---|
| Stable boot to desktop | Yes | Mostly present |
| Deterministic init order | Yes | Present but centralized in large [`kernel_main()`](kernel/kernel.c:621) |
| Memory safety validation | Yes | Heap canaries/tracing present; broader ownership audit needed |
| Interrupt/timer/scheduler stability | Yes | Present; needs stress and SMP policy clarification |
| Filesystem read path | Yes | FAT32 + NTFS read-only present |
| Filesystem write path | Optional/roadmap | FAT32 mutation APIs exist; NTFS write not present |
| GUI/window manager | Yes | Present and large, needs canonicalization |
| Input latency and correctness | Yes | Present with extensive telemetry; needs final route freeze |
| Audio playback | Optional but targeted | AC97/HAL/mixer/player present |
| Userspace apps | Yes | Present; kernel apps still dominate |
| IPC/shared memory | Yes | Phase 2 API present |
| Security/TLS | Optional for desktop v1.0, required for browser/network | Present architecture/tests |
| Sandbox | Optional/advanced | Present architecture/tests |
| Browser | Optional/advanced | Large implementation tree present |
| Shutdown/restart | Yes | Horse power API exists; global shutdown orchestration missing |
| Documentation | Yes | This file is the starting source of truth |

---

## 18. Roadmap to Version 1.0

### Phase A: Freeze Canonical Boot and Subsystem Order

1. Convert [`kernel_main()`](kernel/kernel.c:621) into named boot phases.
2. Add a subsystem registry with dependency declarations.
3. Record init success/failure state per subsystem.
4. Add reverse-order shutdown.

### Phase B: Canonicalize Display Pipeline

1. Define whether normal GUI frames use BWE→BOCompositor→BSPE→AGDTE or BWE→AGDTE directly.
2. Reserve BOGE for future V2 rendering or integrate it explicitly.
3. Document cursor fast path invariants.
4. Make AGDTE the sole present authority.

### Phase C: Canonicalize Input Pipeline

1. Choose Input Core V2 + Dispatcher as authoritative.
2. Route legacy [`kernel_input_push_mouse()`](kernel/drivers/input/input.h:14) and [`kernel_input_push_key()`](kernel/drivers/input/input.h:17) through V2.
3. Define interrupt-safe queue semantics.
4. Add event replay tests.

### Phase D: Move Applications Out of Ring 0

1. Expand GUI syscalls.
2. Migrate calculator/settings/files/music/terminal from [`kernel/apps`](kernel/apps) to [`userspace/apps`](userspace/apps).
3. Keep only window manager/compositor/display in kernel.
4. Use IPC/shared memory for surface buffers.

### Phase E: Harden Filesystem and Loader

1. Preserve NTFS read-only certification paths.
2. Add read-only mount policy enforcement.
3. Improve VFS error codes and path normalization.
4. Use loader/IPC/sandbox together for apps.

### Phase F: Documentation and Tests

1. Keep this handbook updated after every architectural change.
2. Add module READMEs for every subsystem lacking one.
3. Store test logs for NTFS, input latency, OpenGL, audio, security, sandbox, and boot.

---

## 19. How Future AI Engineers Should Work

1. Start with [`CURRENT_TARGET.md`](CURRENT_TARGET.md), [`README.md`](README.md), and this file.
2. Read [`build.ps1`](build.ps1) before changing any subsystem; build inclusion is the real architecture truth.
3. Inspect [`kernel_main()`](kernel/kernel.c:621) before changing initialization or subsystem dependencies.
4. Never add a new engine without listing owner, init order, dependencies, memory model, thread model, public API, and shutdown path.
5. Prefer replacing legacy paths with adapters over introducing parallel pipelines.
6. Do not move kernel apps to userspace until the needed syscalls, memory mappings, and IPC paths are verified.
7. Preserve freestanding constraints: avoid libc assumptions, heap allocation in interrupt paths, floating point in core kernel, and blocking inside display/input hot paths.
8. When editing graphics/input, always document frame/cursor/event ownership.
9. When editing filesystem, clearly distinguish FAT32, NTFS read-only, and future write paths.
10. When editing build scripts, validate boot signature, image sector layout, kernel sector count, and VM artifacts.
11. Update this file whenever subsystem topology or init order changes.

---

## 20. Final Audit Assessment

ATOMS OS / SignaturesOS has reached an advanced experimental OS state with many subsystems normally expected only in much larger operating systems: display train management, windowing, input routing, audio service, NTFS read-only engine, OpenGL software rasterizer, browser engine, IPC, security/TLS, sandboxing, userspace ELFs, and application frameworks.

The main architectural risk is not missing ambition; it is uncontrolled parallelism. Several generations of the same idea exist simultaneously: legacy GUI and BWE, BOCompositor and BOGE, BSPE and AGDTE, legacy input and Input Core V2, top-level drivers and kernel drivers, duplicate audio trees, kernel apps and userspace apps. The path to v1.0 is to freeze canonical ownership, keep compatibility adapters, move applications out of ring 0, and formalize initialization/shutdown graphs.

This handbook should be treated as the initial single source of truth for future engineering and AI-agent handoff.
