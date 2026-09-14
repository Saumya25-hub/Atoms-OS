# CHROMIUM MOJO IPC TO ATOMS OS MAPPING MATRIX

**Document ID:** ATRIX-PHASE14-MATRIX-001  
**Phase:** Phase 14 — Mojo / IPC Integration  
**Date:** 2026-08-26  

---

## 1. Chromium Mojo to ATOMS OS Concept Mapping

| Chromium Mojo Concept | Upstream Chromium Location | ATOMS OS Implementation | Status |
|:---|:---|:---|:---:|
| **Mojo Core Types** | `mojo/public/c/system/types.h` | `mojo/public/c/system/types.h` | **CERTIFIED** |
| **Message Pipe C ABI** | `mojo/public/c/system/message_pipe.h` | `mojo/public/c/system/message_pipe.h` | **CERTIFIED** |
| **Shared Buffer C ABI** | `mojo/public/c/system/buffer.h` | `mojo/public/c/system/buffer.h` | **CERTIFIED** |
| **Handle Table** | `mojo/core/handle_table.h` | `mojo/core/handle_table.h` & `handle_table.cpp` | **CERTIFIED** |
| **Message Pipe Dispatcher** | `mojo/core/message_pipe_dispatcher.h` | `mojo/core/message_pipe.h` & `message_pipe.cpp` | **CERTIFIED** |
| **Shared Buffer Dispatcher** | `mojo/core/shared_buffer_dispatcher.h` | `mojo/core/shared_buffer.h` & `shared_buffer.cpp` | **CERTIFIED** |
| **Message Serialization** | `mojo/public/cpp/system/message.h` | `mojo/public/cpp/system/message.h` & `message.cpp` | **CERTIFIED** |
| **PendingReceiver<T>** | `mojo/public/cpp/bindings/pending_receiver.h` | `mojo/public/cpp/bindings/pending_receiver.h` | **CERTIFIED** |
| **PendingRemote<T>** | `mojo/public/cpp/bindings/pending_remote.h` | `mojo/public/cpp/bindings/pending_remote.h` | **CERTIFIED** |
| **Receiver<T>** | `mojo/public/cpp/bindings/receiver.h` | `mojo/public/cpp/bindings/receiver.h` | **CERTIFIED** |
| **Remote<T>** | `mojo/public/cpp/bindings/remote.h` | `mojo/public/cpp/bindings/remote.h` | **CERTIFIED** |
| **Renderer Mojom Contract** | `third_party/blink/public/mojom/` | `mojo/public/mojom/renderer.mojom.h` | **CERTIFIED** |
| **Network Mojom Contract** | `services/network/public/mojom/` | `mojo/public/mojom/network.mojom.h` | **CERTIFIED** |
| **Storage Mojom Contract** | `components/services/storage/public/mojom/` | `mojo/public/mojom/storage.mojom.h` | **CERTIFIED** |
| **Kernel Low-Level Transport** | POSIX domain sockets / Mach ports | ATOMS Kernel IPC Channels & Shared Memory (`kernel/ipc/`) | **CERTIFIED** |

---

## 2. Mojom Interface Topology

```text
       Browser UI Process (PID B)
                   │
       ┌───────────┼───────────┐
       │           │           │
       ▼           ▼           ▼
  mojom::      mojom::     mojom::
RendererHost  NetworkHost StorageHost
       │           │           │
       │ (Mojo)    │ (Mojo)    │ (Mojo)
       ▼           ▼           ▼
  mojom::      mojom::     mojom::
RendererClient NetworkClient StorageClient
       │           │           │
       ▼           ▼           ▼
   Renderer     Network     Utility
   Process      Process     Process
   (PID R)      (PID N)     (PID U)
```
