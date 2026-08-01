# 🏛️ KERNEL32.sll V1.0 Architectural Specification

> **Subsystem:** KERNEL32.sll V1.0 Ring 3 System Runtime & Win32 Foundation Layer  
> **Target OS:** Signatures OS / ATOMS OS 64-Bit x86_64 Monolithic Kernel  
> **Layer:** Core Ring 3 System Runtime (Single Authority for all Apps & DLLs)  

---

## 1. Executive Summary & Architectural Philosophy

**KERNEL32.sll V1.0** is the official, single Ring 3 System Runtime and Win32 Foundation Layer inside **ATOMS OS**. Influenced by Windows NT KERNEL32.dll/NTDLL, ReactOS, Wine, Linux libc, musl, and glibc, KERNEL32.sll serves as the mandatory runtime gateway for every Ring 3 process (Explorer, Desktop, Terminal, Browser, AI, Games, USER32.sll, GDI32.sll, OpenGL32.sll).

### Core Architectural Mandates:
- **No Direct Syscalls**: No application or higher-level DLL communicates directly with kernel syscalls or BAR.
- **Single Runtime Gateway**: All process creation, thread scheduling, memory management, synchronization, file I/O, dynamic library loading, and diagnostics flow strictly through KERNEL32.sll.

```text
 ┌─────────────────────────────────────────────────────────────┐
 │       Ring 3 Applications (Explorer, Apps, USER32, GDI32)   │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Unified KERNEL32 Public API
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      KERNEL32.sll                           │
 │  ├── 1. Runtime Manager       ├── 13. Pipes Engine          │
 │  ├── 2. Process Runtime       ├── 14. Console Runtime       │
 │  ├── 3. Thread Runtime        ├── 15. Time Runtime          │
 │  ├── 4. Memory Runtime        ├── 16. Locale Runtime        │
 │  ├── 5. Heap Manager          ├── 17. Environment Runtime   │
 │  ├── 6. Synchronization       ├── 18. Dynamic Loader        │
 │  ├── 7. Mutex Engine          ├── 19. TLS Manager           │
 │  ├── 8. Semaphore Engine      ├── 20. Exception Runtime     │
 │  ├── 9. Critical Section      ├── 21. Atom Table            │
 │  ├── 10. Event Engine         ├── 22. Performance Runtime   │
 │  ├── 11. File Runtime         └── 23. Diagnostics Engine    │
 │  └── 12. Directory Runtime                                  │
 └──────────────────────────────┬──────────────────────────────┘
                                │ BAR Application Runtime Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                 BOS Application Runtime (BAR)               │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Shell Object Model Delegation
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │            BSOM (BOS Shell Object Model Engine)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ File System Engine
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │               BFS (BOS File System Engine)                  │
 └──────────────────────────────┬──────────────────────────────┘
                                │ Display & GPU Platform
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │              AGP (ATOMS Graphics Platform V1.0)             │
 └──────────────────────────────┬──────────────────────────────┘
                                │ System Calls & Hardware Drivers
                                ▼
 ┌─────────────────────────────────────────────────────────────┐
 │                      x86_64 Kernel                          │
 └─────────────────────────────────────────────────────────────┘
```

---

## 2. Complete Folder Tree Layout (`userspace/libs/kernel32/`)

```text
userspace/libs/kernel32/
├── include/
│   ├── kernel32_types.h
│   ├── kernel32_api.h
│   └── kernel32_public.h
├── core/
│   └── kernel32_runtime.c
├── process/
│   └── kernel32_process.c
├── thread/
│   └── kernel32_thread.c
├── memory/
│   └── kernel32_memory.c
├── heap/
│   └── kernel32_heap.c
├── sync/
│   └── kernel32_sync.c
├── mutex/
│   └── kernel32_mutex.c
├── semaphore/
│   └── kernel32_semaphore.c
├── critical/
│   └── kernel32_critical.c
├── events/
│   └── kernel32_events.c
├── files/
│   └── kernel32_files.c
├── directory/
│   └── kernel32_directory.c
├── pipes/
│   └── kernel32_pipes.c
├── console/
│   └── kernel32_console.c
├── time/
│   └── kernel32_time.c
├── locale/
│   └── kernel32_locale.c
├── environment/
│   └── kernel32_environment.c
├── loader/
│   └── kernel32_loader.c
├── tls/
│   └── kernel32_tls.c
├── exceptions/
│   └── kernel32_exceptions.c
├── atom/
│   └── kernel32_atoms.c
├── performance/
│   └── kernel32_performance.c
├── diagnostics/
│   └── kernel32_diagnostics.c
├── tests/
│   └── kernel32_certification_tests.c
└── docs/
    └── kernel32_runtime.md
```

---

## 3. Core Subsystem Engine Responsibilities

1. **Runtime Manager**: Subsystem lifecycle, handle tables, subsystem registration.
2. **Process Runtime**: `CreateProcess`, `ExitProcess`, `TerminateProcess`, handle duplication.
3. **Thread Runtime**: `CreateThread`, `ExitThread`, `Sleep`, `Yield`, priority, affinity.
4. **Memory Runtime**: `VirtualAlloc`, `VirtualFree`, `VirtualProtect`, memory copying.
5. **Heap Manager**: Multi-pool sub-allocator (`HeapAlloc`, `HeapFree`, `HeapReAlloc`).
6. **Synchronization**: SRW Locks, Condition Variables, Spinlocks, RW Locks.
7. **Mutex Engine**: Reentrant binary mutex primitives (`CreateMutex`, `ReleaseMutex`).
8. **Semaphore Engine**: Counting semaphores (`CreateSemaphore`, `ReleaseSemaphore`).
9. **Critical Section**: Low-overhead intra-process locks (`InitializeCriticalSection`).
10. **Event Engine**: Manual & Auto-reset events (`CreateEvent`, `SetEvent`, `ResetEvent`).
11. **File Runtime**: `CreateFile`, `ReadFile`, `WriteFile`, `DeleteFile`, `CopyFile`, `MoveFile` routed into BFS.
12. **Directory Runtime**: `CreateDirectory`, `RemoveDirectory`, `FindFirstFile`, `FindNextFile`.
13. **Pipes Engine**: Anonymous & Named Pipes for IPC routing.
14. **Console Runtime**: `CreateConsole`, `WriteConsole`, `ReadConsole`, `SetConsoleTitle`.
15. **Time Runtime**: High-resolution timers, tick counts (`GetTickCount`, `QueryPerformanceCounter`).
16. **Locale Runtime**: Codepages, UTF-8/UTF-16 conversion, Unicode case handling.
17. **Environment Runtime**: Environment block, working directory, command line parsing.
18. **Dynamic Loader**: Dynamic library loading (`LoadLibrary`, `FreeLibrary`, `GetProcAddress`).
19. **TLS Manager**: Thread Local Storage slot allocation (`TlsAlloc`, `TlsSetValue`).
20. **Exception Runtime**: Structured Exception Handling (SEH) and crash reporting.
21. **Atom Table**: Global and local string atom tables with reference counting.
22. **Performance Runtime**: Performance counter frequency and CPU profiling runtime.
23. **Diagnostics Engine**: Leak detection (Handles, Heaps, Threads), deadlock audit, metrics.
