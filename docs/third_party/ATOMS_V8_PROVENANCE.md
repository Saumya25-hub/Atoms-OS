# ATOMS OS V8 PROVENANCE & SOURCE ATTRIBUTION

**Document ID:** ATRIX-PHASE10-PROVENANCE-001  
**Phase:** Phase 10 — V8 Engine Source Provenance Record  
**Date:** 2026-08-26  

---

## 1. Classification & Attribution Matrix

| Component / File | Category | Upstream Project / Author | License | Description of ATOMS Adaptation |
|:---|:---|:---|:---|:---|
| `include/v8*.h` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Public V8 embedder headers (`Isolate`, `Context`, `Script`, `Value`, `HandleScope`). |
| `src/base/page-allocator.*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | `v8::PageAllocator` mapped to ATOMS `SYS_MMAP`, `SYS_MUNMAP`, `SYS_MPROTECT`. |
| `src/base/platform/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Platform timing (`clock_gettime`), CSPRNG, and futex-backed synchronization. |
| `src/heap/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Generational Young/Old spaces and Scavenger / Mark-Sweep garbage collector. |
| `src/objects/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Internal object model: `JSObject`, `JSArray`, `JSFunction`, `JSString`, `JSNumber`. |
| `src/interpreter/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Ignition bytecode set, AST compiler, and VM execution dispatch loop. |
| `src/codegen/x64/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Native AMD64/x86_64 assembler and JIT compilation pipeline with W^X enforcement. |
| `src/api/*` | **ADAPTED CODE** | Google V8 Project | BSD 3-Clause | Implementation of public V8 APIs. |
| `src/adapter/*` | **ATOMS INTEGRATION** | ATOMS OS Team | Proprietary / ATOMS | Platform bootstrap, embedder integration, and ATRIX routing. |
| `tests/*` | **ATOMS ORIGINAL** | ATOMS OS Team | Proprietary / ATOMS | 20-test comprehensive runtime verification suite. |
