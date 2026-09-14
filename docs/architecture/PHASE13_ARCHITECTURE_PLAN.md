# PHASE 13 ARCHITECTURE PLAN: MULTI-PROCESS BROWSER ARCHITECTURE

**Document ID:** ATRIX-PHASE13-ARCH-001  
**Phase:** TASK 2 — ARCHITECTURE SPECIFICATION & EXECUTION PLAN  
**Target Subsystem:** ATRIX Multi-Process Architecture, Browser Process, Renderer Process, Network Process, Utility Process, Process Isolation, Memory & CR3 Ownership, Phase 13 IPC Transport, Crash Recovery  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  
**Architect:** ATOMS OS Architecture & Quality Assurance Committee  

---

## 1. Executive Architectural Overview

Phase 13 transitions the ATRIX Browser on ATOMS OS from an in-process execution model to a genuine **Multi-Process Browser Architecture** modeled on Chromium's architecture:

```text
                               ┌────────────────────────┐
                               │     ATRIX Browser      │
                               │  Browser / UI Process  │
                               │   (PID: B, CR3: CR3_B) │
                               └───────────┬────────────┘
                                           │
             ┌─────────────────────────────┼─────────────────────────────┐
             │ Phase 13 IPC (Channel + SHM)│ Phase 13 IPC (Channel)      │ Phase 13 IPC (Channel)
             ▼                             ▼                             ▼
  ┌──────────────────────┐      ┌──────────────────────┐      ┌──────────────────────┐
  │   Renderer Process   │      │   Network Process    │      │   Utility Process    │
  │ (PID: R, CR3: CR3_R) │      │ (PID: N, CR3: CR3_N) │      │ (PID: U, CR3: CR3_U) │
  │                      │      │                      │      │                      │
  │ • Blink DOM / Parser │      │ • Chromium Net       │      │ • Chromium Storage   │
  │ • Google V8 JS Engine│      │ • DNS Resolver       │      │ • Local/Session Store│
  │ • Style & Box Layout │      │ • TCP / TLS 1.2      │      │ • ATOMS VFS Disk I/O │
  │ • Skia 2D Painting   │      │ • URLLoader Pipeline │      │ • Quota Enforcement  │
  └──────────────────────┘      └──────────────────────┘      └──────────────────────┘
             │                             │                             │
             └─────────────────────────────┼─────────────────────────────┘
                                           │
                                           ▼
                                 ┌───────────────────┐
                                 │   ATOMS Kernel    │
                                 │ • VMM / CR3 / PMM │
                                 │ • Scheduler / Task│
                                 │ • PCB / IPC / SHM │
                                 └───────────────────┘
```

---

## 2. Process Roles & Responsibility Matrix

### 1. Browser Process (UI & System Host)
- **Role:** Master coordinator, window manager interface, tab state, Omnibox, user input routing, permissions.
- **Address Space:** Host process address space (CR3_B).
- **Subsystems Hosted:**
  - BWE window management, menu bar, tab strip, address bar rendering.
  - Process host controllers: `RendererProcessHost`, `NetworkProcessHost`, `UtilityProcessHost`.
  - Crash detection, tab reloading, and UI fallback ("Sad Tab" screen).
- **Security:** Ring 3 userspace (or kernel browser authority during bring-up); orchestrates child permissions.

### 2. Renderer Process (Web Content Engine)
- **Role:** Parse HTML/CSS, construct DOM tree, execute untrusted JavaScript, compute layout geometry, paint to Skia surface.
- **Address Space:** Isolated per-tab address space (CR3_R).
- **Subsystems Hosted:**
  - `third_party/blink/` (DOM Core, HTML5 Parser, CSS Style, Layout Objects, Box Flow).
  - `third_party/v8/` (Isolate, Context, Ignition VM, Garbage Collector, JIT compiler).
  - `third_party/skia/` (SkCanvas, SkPaint, SkPath, Rasterizer).
  - Shared Memory surface buffer receiver (`bos_shm_*`).
- **Security:** Strictly untrusted. No direct disk I/O, no direct network sockets, no hardware access. All I/O is proxied via IPC.

### 3. Network Process (Networking & Protocols)
- **Role:** Handle all URL resolution, socket connections, TLS handshakes, HTTP transactions, caching, and cookie management.
- **Address Space:** Dedicated network address space (CR3_N).
- **Subsystems Hosted:**
  - `third_party/chromium_net/` (`GURL`, `SecurityOrigin`, `HttpRequestHeaders`, `HttpResponseHeaders`, `CookieStore`, `HttpCache`, `URLLoader`, `AtomsNetworkAdapter`).
- **Security:** Isolated network sandbox. Does not execute JavaScript or parse complex DOM.

### 4. Utility Process (Storage & VFS Persistence)
- **Role:** Manage origin-isolated persistent storage, session storage, quota calculations, and ATOMS VFS disk file serialization.
- **Address Space:** Dedicated utility address space (CR3_U).
- **Subsystems Hosted:**
  - `third_party/chromium_storage/` (`StorageArea`, `StorageNamespace`, `LocalStorageManager`, `SessionStorageManager`, `AtomsStorageVFSAdapter`).
- **Security:** Dedicated storage accessor.

---

## 3. Real Address-Space & Memory Isolation Architecture

```text
                  Browser Process                     Renderer Process
                Virtual Memory (CR3_B)              Virtual Memory (CR3_R)
        0x0000_0000_0000   ┌─────────────────┐       ┌─────────────────┐
                           │   Null Guard    │       │   Null Guard    │
        0x0000_0100_0000   ├─────────────────┤       ├─────────────────┤
                           │ Browser Code    │       │ Renderer Code   │ (Blink, V8, Skia)
                           │   [R-X] [User]  │       │   [R-X] [User]  │
                           ├─────────────────┤       ├─────────────────┤
                           │ Browser Heap    │       │ Renderer Heap   │ (V8 GC / Blink DOM)
                           │   [RW-] [User]  │       │   [RW-] [User]  │
                           ├─────────────────┤       ├─────────────────┤
                           │ Browser Stack   │       │ Renderer Stack  │
                           │   [RW-] [User]  │       │   [RW-] [User]  │
                           ├─────────────────┤       ├─────────────────┤
        0x0000_6000_0000   │ SHM Surface Buf │◄─────►│ SHM Surface Buf │ (Zero-copy Skia surface)
                           │   [RW-] [User]  │       │   [RW-] [User]  │
        0x0000_7FFF_FFFF   └─────────────────┘       └─────────────────┘
        0xFFFF_8000_0000   ┌───────────────────────────────────────────┐
                           │       Shared Kernel Higher-Half           │ (CR3 Independent)
                           │     [RW-] [Supervisor Only] (CPL0)        │
        0xFFFF_FFFF_FFFF   └───────────────────────────────────────────┘
```

### Memory Rules & Invariants:
1. **CR3 Divergence:** $CR3_B \ne CR3_R \ne CR3_N \ne CR3_U$.
2. **No Accidental Shared Memory:** User pages mapped in Renderer ($CR3_R$) are physically disjoint from Browser ($CR3_B$), except for explicitly attached `bos_shm` surface handles.
3. **W^X Enforcement:** No page is simultaneously Writable and Executable. V8 JIT compiler requests page transition via `mprotect(PROT_READ | PROT_EXEC)` before executing generated code.
4. **Pointer Sanitization:** Kernel validates all user pointers against $0x0000000001000000 \le \text{addr} \le 0x00007FFFFFFFFFFF$.

---

## 4. Process Lifecycle & State Machine

```text
                  ┌─────────────────┐
                  │     CREATE      │  ATOMS_Process_Create() -> Allocates PCB & unique PID
                  └────────┬────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │      LOAD       │  vmm_create_address_space() -> Allocates unique PML4 / CR3
                  └────────┬────────┘  Loads process code, heap base, user stack
                           │
                           ▼
                  ┌─────────────────┐
                  │      START      │  scheduler_create_task(pml4, entry_point, pid)
                  └────────┬────────┘  Task placed in READY queue
                           │
                           ▼
                  ┌─────────────────┐
                  │       RUN       │  Scheduler context switch activates CR3
                  └────────┬────────┘  Executes Blink / V8 / Skia in Ring 3
                           │
            ┌──────────────┴──────────────┐
            ▼                             ▼
   ┌─────────────────┐           ┌─────────────────┐
   │ NORMAL EXIT     │           │ ABNORMAL CRASH  │  Page Fault #PF, Division by zero, etc.
   │ (SYS_EXIT)      │           │ (Fault Vector)  │
   └────────┬────────┘           └────────┬────────┘
            │                             │
            └──────────────┬──────────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │     CLEANUP     │  scheduler_terminate_tasks_by_pid(pid)
                  └────────┬────────┘  BOS_CloseSurfacesByPID(pid)
                           │           Reclaims SHM handles & frees PML4
                           ▼
                  ┌─────────────────┐
                  │   RECOVERY      │  Browser Process displays "Sad Tab"
                  │   (RELOAD)      │  User clicks Reload -> Fresh Renderer Process created!
                  └─────────────────┘
```

---

## 5. IPC Architecture & Abstraction Boundary (Phase 13 Transport)

To cleanly anticipate Phase 14 (Chromium Mojo IPC), Phase 13 introduces `AtomsIPCChannel`:

```cpp
namespace ipc {

enum MessageType {
    MSG_NAVIGATE           = 0x0010,  // Browser -> Renderer: Render URL / HTML
    MSG_RENDER_FRAME_READY = 0x0011,  // Renderer -> Browser: Frame painted in SHM surface
    MSG_DOM_EVENT          = 0x0012,  // Browser -> Renderer: Click / Key / Mouse move
    MSG_FETCH_REQUEST      = 0x0020,  // Renderer -> Network: Fetch resource
    MSG_FETCH_RESPONSE     = 0x0021,  // Network -> Renderer: HTTP Response data
    MSG_STORAGE_SET        = 0x0030,  // Renderer -> Utility: Set item
    MSG_STORAGE_GET        = 0x0031,  // Renderer -> Utility: Get item
    MSG_PROCESS_PING       = 0x0001,  // Heartbeat / liveness check
    MSG_PROCESS_PONG       = 0x0002,  // Heartbeat response
    MSG_PROCESS_CRASH      = 0x000F,  // Crash notification
};

struct IPCMessage {
    uint32_t message_id;
    uint32_t type;
    uint32_t src_pid;
    uint32_t dst_pid;
    uint32_t payload_size;
    uint8_t  payload[1024];
};

class AtomsIPCChannel {
public:
    static AtomsIPCChannel* Create(const char* name, uint32_t src_pid, uint32_t dst_pid);
    bool Send(uint32_t type, const void* data, size_t size);
    bool Receive(IPCMessage* out_msg, bool non_blocking);
    void Close();
};

} // namespace ipc
```

---

## 6. Implementation Checklist & Files to Modify

| Step | Target Subsystem / File | Purpose of Modification |
|:---:|:---|:---|
| **1** | `kernel/browser_engine/process/abe_process.h`<br>`kernel/browser_engine/process/abe_process.c` | Upgrade legacy ABE process layer to real ATOMS kernel process instantiation (`ATOMS_Process_Create`, `vmm_create_address_space`, `scheduler_create_task`). |
| **2** | `third_party/chromium_ipc/atoms_ipc_channel.h`<br>`third_party/chromium_ipc/atoms_ipc_channel.cpp` | Phase 13 IPC message passing and channel transport implementation. |
| **3** | `third_party/chromium_process/browser_process_host.h`<br>`third_party/chromium_process/browser_process_host.cpp` | Browser Process coordinator managing child process tables, PIDs, CR3 tracking, and crash recovery. |
| **4** | `third_party/chromium_process/renderer_process_host.h`<br>`third_party/chromium_process/renderer_process_host.cpp` | Renderer Process host managing Blink + V8 + Skia lifecycle and SHM surface painting. |
| **5** | `third_party/chromium_process/network_process_host.h`<br>`third_party/chromium_process/network_process_host.cpp` | Network Process host managing Chromium Net requests via IPC. |
| **6** | `third_party/chromium_process/utility_process_host.h`<br>`third_party/chromium_process/utility_process_host.cpp` | Utility Process host managing Chromium Storage and VFS disk I/O via IPC. |
| **7** | `third_party/chromium_process/tests/process_test_suite.h`<br>`third_party/chromium_process/tests/process_test_suite.cpp`<br>`third_party/chromium_process/tests/process_test_main.cpp` | 20-test deterministic verification suite. |
| **8** | `kernel/apps/atrix/atrix_browser.c` | Add `about:processes`, `about:multiprocess`, `about:crashed` diagnostic endpoints. |
| **9** | `BUILD.gn` & `build.ps1` | Integrate `chromium_ipc`, `chromium_process`, and `chromium_process_test_runner`. |

---

## 7. Risk Analysis & Rollback Plan

- **Risk 1: CR3 Switch Latency in Scheduler:**
  - *Mitigation:* `scheduler.c` already checks `if (old_task->pml4 != new_task->pml4)` before executing `mov %cr3`, ensuring zero overhead for same-process threads.
- **Risk 2: Shared Memory Leak on Renderer Crash:**
  - *Mitigation:* `ATOMS_Process_Terminate()` automatically invokes `BOS_CloseSurfacesByPID(pid)` and reclaims attached SHM buffers.
- **Risk 3: Build Pipeline Complexity:**
  - *Mitigation:* All new components follow modular GN static library architecture (`static_library("chromium_ipc")`, `static_library("chromium_process")`).
- **Rollback Plan:**
  - If multi-process bring-up experiences an unresolvable kernel dependency, the existing certified Phase 12 in-process rendering path remains 100% functional and can be toggled via `g_atrix_multiprocess_enabled = false`.

---

## 8. Verification Strategy & Exit Criteria

1. **Deterministic Test Matrix:** 20/20 test vectors must pass.
2. **Runtime Proof:** QEMU serial trace must demonstrate:
   - $\text{PID}_{\text{Browser}} \ne \text{PID}_{\text{Renderer}} \ne \text{PID}_{\text{Network}} \ne \text{PID}_{\text{Utility}}$.
   - $CR3_{\text{Browser}} \ne CR3_{\text{Renderer}} \ne CR3_{\text{Network}} \ne CR3_{\text{Utility}}$.
3. **Crash Containment Proof:** Intentionally terminating a Renderer Process must not crash the Browser UI Process.
4. **Zero Regressions:** CPU, GDT, SMP, IDT, PIC, PMM, VMM, Heap, Skia, V8, Blink, Chromium Net, and Storage must remain 100% operational.
