# PHASE 14 ARCHITECTURE PLAN: MOJO / IPC INTEGRATION

**Document ID:** ATRIX-PHASE14-ARCH-001  
**Phase:** TASK 3 — ARCHITECTURAL SPECIFICATION & DESIGN PLAN  
**Target Subsystem:** Chromium Mojo Layer, Message Pipes, Handles, Serialization, Interface Bindings (`Remote`/`Receiver`), Async Message Dispatch, Disconnect Handling, Crash Recovery, Shared Memory Transport  
**Standard:** Rule 0 Phase Isolation Protocol (Investigate ➔ Plan ➔ Implement ➔ Build ➔ Runtime Verify ➔ Audit ➔ Certify)  
**Date:** 2026-08-26  
**Architect:** ATOMS OS Architecture & Quality Assurance Committee  

---

## 1. Executive Architectural Overview

Phase 14 replaces the temporary Phase 13 flat IPC channel with a genuine **Chromium-style Mojo IPC Subsystem** operating across all ATOMS OS browser processes:

```text
                  Browser UI Process (PID B)
                              │
                    ┌─────────┴─────────┐
                    │ mojo::Receiver<T> │  (e.g. mojom::RendererHost)
                    └─────────┬─────────┘
                              │
                    ┌─────────┴─────────┐
                    │ mojo::MessagePipe │  (Endpoint A)
                    └─────────┬─────────┘
                              │
               ═══════════════╪═══════════════ (Address Space / CR3 Boundary)
                              │
                    ┌─────────┴─────────┐
                    │ mojo::MessagePipe │  (Endpoint B)
                    └─────────┬─────────┘
                              │
                    ┌─────────┴─────────┐
                    │  mojo::Remote<T>  │  (e.g. mojom::RendererHost proxy)
                    └─────────┬─────────┘
                              │
                  Renderer Process (PID R)
```

---

## 2. Core Subsystems & Layering

```text
  ┌────────────────────────────────────────────────────────────────────────┐
  │ 5. Application Layer: ATRIX Browser / Blink / V8 / Skia / Net / Storage│
  ├────────────────────────────────────────────────────────────────────────┤
  │ 4. Mojom Interface Contracts: RendererHost, NetworkHost, StorageHost   │
  ├────────────────────────────────────────────────────────────────────────┤
  │ 3. Mojo Bindings: Remote<T>, Receiver<T>, PendingRemote, PendingReceiver│
  ├────────────────────────────────────────────────────────────────────────┤
  │ 2. Mojo System / Serialization: Message, MessageHeader, Serializer     │
  ├────────────────────────────────────────────────────────────────────────┤
  │ 1. Mojo Core C ABI: Handles, MessagePipe, SharedBuffer, HandleTable    │
  ├────────────────────────────────────────────────────────────────────────┤
  │ 0. ATOMS Kernel Transport: IPC Channels, Shared Memory (bos_shm_*)     │
  └────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Handle & Resource Ownership Model

1. **Handle Representation:** `MojoHandle` is an opaque `uint32_t` representing an entry in a process-local `HandleTable`.
2. **Handle Types:**
   - `MOJO_HANDLE_TYPE_MESSAGE_PIPE` (Message pipe endpoint)
   - `MOJO_HANDLE_TYPE_SHARED_BUFFER` (Shared memory buffer)
3. **Handle Rights & Ownership:**
   - Handles cannot be forged. Valid handles must exist in the active process's `HandleTable`.
   - `MojoClose(handle)` invalidates the handle and notifies the peer endpoint.
   - When a process terminates, all handles in its `HandleTable` are automatically closed.
4. **Handle Transfer Semantics:**
   - Handles may be transferred inside a `mojo::Message`.
   - The sender's handle is closed/invalidated immediately upon serialization.
   - The receiver mints a new valid `MojoHandle` upon deserialization.

---

## 4. Message Format & Deterministic Serialization

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Total Message Size                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Interface Name                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Method Ordinal                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                             Flags                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Request ID                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Payload Size                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                        Serialized Data                        |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Strict Validation Rules:
- Header length check: Must be at least `sizeof(MessageHeader)` (24 bytes).
- Total length check: `Total Message Size == Header Size + Payload Size`.
- Maximum message size cap: $1\text{ MB}$ (oversized messages are rejected with `MOJO_RESULT_RESOURCE_EXHAUSTED`).
- Buffer overrun check: Reading primitives verifies remaining payload length before consuming bytes.
- Type validation: Strings must be null-terminated within payload bounds; enums must fall within valid value ranges.

---

## 5. Mojom Interface Contracts

### 1. `mojom::RendererHost` (Implemented by Browser, Called by Renderer)
```cpp
class RendererHost {
public:
    virtual void FrameReady(mojo::ScopedSharedBufferHandle frame_buffer, uint32_t width, uint32_t height, uint32_t stride) = 0;
    virtual void TitleChanged(const std::string& new_title) = 0;
    virtual void RendererStatus(uint32_t status_code, const std::string& detail) = 0;
    virtual void RendererCrash(int32_t exit_code, const std::string& reason) = 0;
};
```

### 2. `mojom::RendererClient` (Implemented by Renderer, Called by Browser)
```cpp
class RendererClient {
public:
    virtual void Navigate(const std::string& url, const std::string& html_source) = 0;
    virtual void SendDOMEvent(uint32_t type, int32_t x, int32_t y, uint32_t key_code, uint32_t modifiers) = 0;
    virtual void SetViewportSize(uint32_t width, uint32_t height) = 0;
};
```

### 3. `mojom::NetworkHost` (Implemented by Network Process, Called by Browser/Renderer)
```cpp
class NetworkHost {
public:
    virtual void URLRequest(uint32_t request_id, const std::string& url, const std::string& method, const std::string& headers) = 0;
};

class NetworkClient {
public:
    virtual void URLResponse(uint32_t request_id, int32_t status_code, const std::string& headers, const std::string& body) = 0;
    virtual void NetworkError(uint32_t request_id, int32_t error_code, const std::string& message) = 0;
};
```

### 4. `mojom::StorageHost` (Implemented by Utility Process, Called by Browser/Renderer)
```cpp
class StorageHost {
public:
    virtual void StorageGet(const std::string& origin, const std::string& key, uint32_t request_id) = 0;
    virtual void StorageSet(const std::string& origin, const std::string& key, const std::string& value, uint32_t request_id) = 0;
    virtual void StorageRemove(const std::string& origin, const std::string& key, uint32_t request_id) = 0;
    virtual void StorageClear(const std::string& origin, uint32_t request_id) = 0;
};

class StorageClient {
public:
    virtual void StorageResponse(uint32_t request_id, bool success, const std::string& value) = 0;
};
```

---

## 6. Asynchronous Dispatch & Task Sequence

- Each `Receiver` registers a message pump handler that drains the endpoint message queue.
- Incoming messages are dispatched to interface stubs, which deserialize arguments and invoke the target methods.
- Responses are serialized into return messages carrying the original `request_id` and dispatched back over the pipe.

---

## 7. Disconnect Detection & Crash Containment

```text
Renderer Process Dies
        │
        ▼
Kernel marks PID R as TERMINATED/CRASHED
        │
        ▼
Mojo Message Pipe endpoint peer closed
        │
        ▼
Browser Host Receiver / Remote detects MOJO_RESULT_FAILED_PRECONDITION
        │
        ▼
Disconnect callback invoked (e.g. `set_disconnect_handler`)
        │
        ▼
Browser marks tab crashed, releases dead pipe handle
        │
        ▼
User clicks reload -> Fresh MessagePipe created -> Fresh Renderer spawned!
```

---

## 8. Implementation Steps & File Directory

| Step | File Path | Purpose |
|:---:|:---|:---|
| **1** | `mojo/public/c/system/types.h`<br>`mojo/public/c/system/message_pipe.h`<br>`mojo/public/c/system/buffer.h` | Mojo Core C ABI definitions (`MojoHandle`, `MojoResult`, pipe & buffer C functions). |
| **2** | `mojo/core/handle_table.h`<br>`mojo/core/handle_table.cpp` | Process-local handle table managing handle allocation, transfer, and teardown. |
| **3** | `mojo/core/message_pipe.h`<br>`mojo/core/message_pipe.cpp` | Bidirectional message pipe implementation with queueing and peer state tracking. |
| **4** | `mojo/core/shared_buffer.h`<br>`mojo/core/shared_buffer.cpp` | Shared memory buffer wrapper connecting to `bos_shm_*`. |
| **5** | `mojo/public/cpp/system/message.h`<br>`mojo/public/cpp/system/message.cpp` | Serialization and deserialization engine with strict bounds checking. |
| **6** | `mojo/public/cpp/bindings/pending_receiver.h`<br>`mojo/public/cpp/bindings/pending_remote.h`<br>`mojo/public/cpp/bindings/receiver.h`<br>`mojo/public/cpp/bindings/remote.h` | High-level C++ interface binding templates. |
| **7** | `mojo/public/mojom/renderer.mojom.h`<br>`mojo/public/mojom/network.mojom.h`<br>`mojo/public/mojom/storage.mojom.h` | Concrete Mojom interface contracts for ATRIX subsystems. |
| **8** | `third_party/chromium_process/browser_process_host.cpp`<br>`third_party/chromium_process/renderer_process_host.cpp`<br>`third_party/chromium_process/network_process_host.cpp`<br>`third_party/chromium_process/utility_process_host.cpp` | Integration of Mojo bindings into existing multi-process hosts. |
| **9** | `mojo/tests/mojo_test_suite.h`<br>`mojo/tests/mojo_test_suite.cpp`<br>`mojo/tests/mojo_test_main.cpp` | 28-test deterministic verification test suite. |
| **10** | `BUILD.gn` & `build.ps1` | Build pipeline integration. |

---

## 9. Rollback Plan

The Phase 13 process hosts retain backward-compatible interfaces. If any unexpected Mojo failure occurs, the fallback transport remains readily accessible, ensuring zero risk of unrecoverable regressions.
