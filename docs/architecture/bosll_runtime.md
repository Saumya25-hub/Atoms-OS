# 🏛️ BOSLL.sll V1.0 Architectural Specification

> **Subsystem:** BOSLL.sll V1.0 BOS Low-Level Native Runtime  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Lowest Ring 3 Native System Layer (Directly Above Kernel Syscall Boundary)  

---

## 1. Executive Summary & Architectural Philosophy

**BOSLL.sll V1.0** is the lowest Ring 3 Native Runtime Layer inside **ATOMS OS**. Sitting directly above the kernel syscall boundary, BOSLL provides the native process, thread, memory, handle, loader, heap, synchronization, exception, IPC, and syscall dispatch services for all Ring 3 applications and system libraries.

### Core Architectural Mandates:
- **Lowest Ring 3 Runtime Authority**: Every high-level library (`KERNEL32.sll`, `USER32.sll`, `GDI32.sll`, `COMCTL32.sll`, `COMDLG32.sll`, `SHELL32.sll`, `OpenGL32.sll`) delegates native runtime execution to BOSLL.sll.
- **KERNEL32 Win32 Wrapper**: KERNEL32.sll acts as a Win32 compatibility layer wrapping BOSLL's native APIs (`BosCreateProcess`, `BosAllocateHeap`, `BosAllocateVirtualMemory`).
- **Layered Subsystem Flow**:

```text
 ┌─────────────────────────────────────────────────────────────┐
 │    Ring 3 Applications (Explorer, Apps, Desktop, Shell)    │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Library APIs
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │   SHELL32 | USER32 | GDI32 | COMCTL32 | COMDLG32 | OpenGL32   │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Win32 Subsystem Layer
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                         KERNEL32.sll                        │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Native Runtime Layer
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                         BOSLL.sll                           │
 │  ├── 1. Runtime Bootstrap     ├── 11. Exception Runtime      │
 │  ├── 2. Native Loader         ├── 12. Native IPC Runtime     │
 │  ├── 3. Object Manager        ├── 13. Virtual Memory Engine  │
 │  ├── 4. Handle Manager        ├── 14. Security Transition    │
 │  ├── 5. Process Runtime       ├── 15. Syscall Dispatcher     │
 │  ├── 6. Thread Runtime        ├── 16. Timer Runtime          │
 │  ├── 7. Memory Runtime        ├── 17. Performance Runtime    │
 │  ├── 8. Heap Runtime          ├── 18. CPU Context Runtime    │
 │  ├── 9. Synchronization       ├── 19. Environment Runtime    │
 │  └── 10. Diagnostics Engine   └── 20. 300-Test Suite         │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Kernel Syscalls
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                       x86_64 Kernel                         │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/bosll/`)

```text
userspace/libs/bosll/
├── include/
│   ├── bosll_types.h
│   ├── bosll_api.h
│   └── bosll_public.h
├── core/
│   └── bos_bootstrap.c
├── loader/
│   └── bos_loader.c
├── object/
│   └── bos_object.c
├── process/
│   └── bos_process.c
├── thread/
│   └── bos_thread.c
├── memory/
│   ├── bos_memory.c
│   └── bos_vmem.c
├── heap/
│   └── bos_heap.c
├── sync/
│   └── bos_sync.c
├── handles/
│   └── bos_handles.c
├── exceptions/
│   └── bos_exceptions.c
├── syscall/
│   └── bos_syscall.c
├── timer/
│   └── bos_timer.c
├── performance/
│   └── bos_performance.c
├── context/
│   └── bos_context.c
├── environment/
│   └── bos_environment.c
├── security/
│   └── bos_security.c
├── ipc/
│   └── bos_ipc.c
├── diagnostics/
│   └── bos_diagnostics.c
├── tests/
│   └── bosll_certification_tests.c
└── docs/
    └── bosll_runtime.md
```

---

## 3. Core Engine Responsibilities Matrix

1. **Runtime Bootstrap**: Subsystem initialization and shutdown (`BosInitialize`, `BosShutdown`).
2. **Native Loader**: Module loading and symbol resolution (`BosLoadLibrary`, `BosGetProcedure`).
3. **Native Object Manager**: Generic object lifetime and registration (`BosQueryObject`).
4. **Native Handle Manager**: Handle allocation, duplication, and destruction (`BosCreateHandle`, `BosCloseHandle`, `BosDuplicateHandle`).
5. **Native Process Runtime**: Process creation, termination, and query (`BosCreateProcess`, `BosTerminateProcess`, `BosGetCurrentProcess`).
6. **Native Thread Runtime**: Thread creation, exit, and context management (`BosCreateThread`, `BosExitThread`, `BosGetCurrentThread`).
7. **Native Memory Runtime**: Physical and virtual memory paging (`BosAllocateVirtualMemory`, `BosFreeVirtualMemory`).
8. **Native Heap Runtime**: Fast user-mode heap management (`BosAllocateHeap`, `BosFreeHeap`).
9. **Native Synchronization**: Mutexes, events, and wait objects (`BosCreateEvent`, `BosCreateMutex`, `BosWaitObject`).
10. **Native Exception Runtime**: User-mode structured exception handling (`BosRaiseException`).
11. **Native IPC Runtime**: High-speed inter-process communication channels (`BosCreateIPC`).
12. **Native Virtual Memory**: Virtual memory page protection and mapping (`BosProtectVirtualMemory`).
13. **Native Security Transition**: User-to-kernel privilege and security token validation (`BosValidateSecurityToken`).
14. **Native Syscall Dispatcher**: Direct CPU x86_64 `SYSCALL` instruction gateway (`BosDispatchSyscall`).
15. **Native Timer Runtime**: High-resolution system time (`BosQuerySystemTime`).
16. **Native Performance Runtime**: High-frequency performance counter (`BosQueryPerformance`).
17. **Native CPU Context Runtime**: Thread CPU register state save/restore (`BosGetThreadContext`, `BosSetThreadContext`).
18. **Native Environment Runtime**: Environment variable block management (`BosGetEnvironment`).
19. **Native Diagnostics**: Low-level handle, memory, and performance diagnostic audit (`BosDumpDiagnostics`).
20. **Certification Battery**: 300-test production certification suite (`bosll_certification_tests.c`).
