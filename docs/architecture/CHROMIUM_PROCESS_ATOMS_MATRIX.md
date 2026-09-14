# CHROMIUM MULTI-PROCESS & IPC TO ATOMS OS MAPPING MATRIX

**Document ID:** ATRIX-PHASE13-MATRIX-001  
**Phase:** Phase 13 — Multi-Process Browser Architecture  
**Date:** 2026-08-26  

---

## 1. Process Architecture Mapping Matrix

| Chromium Architecture Concept | Chromium Source Concept | ATOMS OS Implementation | Status in ATOMS OS |
|:---|:---|:---|:---:|
| **Browser Process** | `content::BrowserMain` / `content::BrowserProcess` | `process::BrowserProcessHost` (`third_party/chromium_process/browser_process_host.cpp`) | **CERTIFIED** |
| **Renderer Process** | `content::RendererMain` / `content::RenderProcess` | `process::RendererProcessHost` (`third_party/chromium_process/renderer_process_host.cpp`) | **CERTIFIED** |
| **Network Process** | `services::network::NetworkService` | `process::NetworkProcessHost` (`third_party/chromium_process/network_process_host.cpp`) | **CERTIFIED** |
| **Utility Process** | `content::UtilityMain` / Storage Host | `process::UtilityProcessHost` (`third_party/chromium_process/utility_process_host.cpp`) | **CERTIFIED** |
| **Process IDs & Tracking** | POSIX PID / Windows Handle | `ATOMS_PCB` & PID Allocator (PIDs 200..65535, `kernel/core/process/process_manager.c`) | **CERTIFIED** |
| **Hardware Memory Isolation** | Independent MMU Page Tables | Unique PML4 per Process, CR3 hardware register switching (`kernel/core/memory/vmm/src/vmm.c`) | **CERTIFIED** |
| **Cross-Process IPC** | `IPC::Channel` / Chromium Mojo transport | `ipc::AtomsIPCChannel` (`third_party/chromium_ipc/atoms_ipc_channel.cpp`) | **CERTIFIED** |
| **Zero-Copy Framebuffer** | Shared Memory Pixel Buffer | `bos_shm_*` / `AtomsSkiaSurface` shared memory mapping (`kernel/ipc/shared_memory/shm_manager.c`) | **CERTIFIED** |
| **Crash Containment** | Child Process Watcher & "Sad Tab" | Browser Host crash detection, `about:crashed` view, and instant tab reload | **CERTIFIED** |
| **Process Termination** | `kill()` / `TerminateProcess()` | `ATOMS_Process_Terminate()`, task reclamation, PML4 freeing via `vmm_destroy_address_space()` | **CERTIFIED** |

---

## 2. Address Space & Boundary Comparison

```text
  Process Role    PID Space       Virtual Space         Hardware CR3        Hosted Engines
  ─────────────  ──────────  ───────────────────────  ────────────────  ────────────────────────
  Browser UI     PID: B      0x0000000001000000..7FFF  CR3_B (Unique)    BWE UI, Tabs, Omnibox
  Renderer       PID: R      0x0000000001000000..7FFF  CR3_R (Unique)    Blink DOM, V8, Skia
  Network        PID: N      0x0000000001000000..7FFF  CR3_N (Unique)    GURL, URLLoader, Cache
  Utility        PID: U      0x0000000001000000..7FFF  CR3_U (Unique)    Web Storage, VFS Store
```
