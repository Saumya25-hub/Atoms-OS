# 🎯 CURRENT TARGET

## Current Module
BOSurface (BWE — BISHOP Windowing Engine) & Native Console Host (ConHost)

## Current Phase
Phase 15 — Native Console Host (CONHOST) & Preemptive Terminal Integration

## Today's Goal
Build a production-grade Console Host architecture and solve the preemptive scheduling race condition preventing native `.BOSX` processes from interacting with GUI terminals.

## Status
- ✅ Phase 15 ConHost Subsystem & Preemptive Scheduler Integration tested & passed on QEMU!
- ✅ Resolved critical execution deadlock where `SHELL.BOSX` outran terminal window creation and blocked on uninitialized keyboard FIFOs.
- ✅ Boot task registered in scheduler, enabling true preemptive multitasking between GUI System Task and userspace `.BOSX` applications.

## Do NOT Touch
- BOSX Loader Subsystem (`bosx_loader.c`)
- Native Console Host (`conhost.c`, `conhost.h`)
- Preemptive Scheduler (`scheduler.c`, `scheduler.h`)
- BMDE (Debug Engine)
- File System (FAT32/VFS)

## Done Today
- ✅ Implemented Native Console Host (`conhost.c`, `conhost.h`) decoupling terminal rendering from process stdout/stdin streams.
- ✅ Diagnosed and solved race condition in `BOSX_Load` using atomic `cli`/`sti` boundaries during process spawn and session attachment.
- ✅ Implemented `scheduler_register_boot_task()` allowing `kernel_main` GUI loop to register as a preemptible kernel task.
- ✅ Enabled `scheduler_running = true` and allocated dedicated Ring 0 stack for boot task to maintain valid `TSS.RSP0` across context switches.
- ✅ Verified `SHELL.BOSX` interactive startup, Atoms Library self-test output, and real-time keyboard input flow in QEMU.

## Context Switch Notes
```
IF interrupted by a bug:
1. Write what you were doing HERE
2. Fix the bug
3. Come back and read this file
4. Resume exactly where you left off
```
