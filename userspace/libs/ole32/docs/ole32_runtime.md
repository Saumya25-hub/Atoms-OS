# 🏛️ Phase 21 — OLE32.sll V1.0 COM Runtime & Component Infrastructure

## Architectural Overview

**OLE32.sll V1.0** is the official **Ring 3 COM Runtime and Component Infrastructure** inside **ATOMS OS**.
It provides the authoritative object component framework for Explorer Shell Objects, Context Menu Extensions, Property Pages, Clipboard Objects, Drag & Drop, File Preview Handlers, Thumbnail Providers, Namespace Extensions, Control Panel Modules, and Browser Plugins.

```
Applications (Explorer, Browser, Settings, Installer, Termina, Office)
      │
      ▼
   OLE32.sll (Ring 3 COM Runtime & Component Infrastructure)
      │
      ├── COM Runtime (CoInitialize, CoInitializeEx, CoUninitialize)
      ├── Object Factory & Instantiation (CoCreateInstance, CoRegisterClassObject)
      ├── Reference Counting & Lifetime (IUnknown, AddRef, Release, QueryInterface)
      ├── GUID / CLSID / IID Database (CoCreateGuid, StringFromCLSID, CLSIDFromString)
      ├── Apartment Model Engine (STA, MTA, Thread Contexts)
      ├── Interface Marshalling Engine (CoMarshalInterface, CoUnmarshalInterface)
      ├── Structured Storage & Stream Engine (StgCreateStorageEx, CreateStreamOnHGlobal)
      └── Data Transfer & Drag Drop Runtime (OleSetClipboard, DoDragDrop, RegisterDragDrop)
      │
      ▼
USER32.sll / GDI32.sll / SHELL32.sll / KERNEL32.sll / BOSLL.sll
      │
      ▼
ATOMS Kernel
```

---

## Technical Constraints & Design Principles

1. **Zero Window Ownership:** Windows belong strictly to `USER32.sll`.
2. **Zero Drawing Ownership:** Rendering belongs strictly to `GDI32.sll` / `OpenGL32.sll`.
3. **Zero Security Ownership:** Security belongs strictly to `ADVAPI32.sll`.
4. **Zero Networking Ownership:** Networking belongs strictly to `WS2_32.sll`.
5. **Single COM Authority:** All `IUnknown` interface pointers, VTables, GUIDs, CLSIDs, IIDs, Class Factories, and Structured Storage are owned and managed by `OLE32.sll`.

---

## 20 Core Engines Architecture

1. **Runtime Manager** (`ole_runtime.c`): Manages subsystem lifecycle and memory pools.
2. **COM Runtime** (`ole_com.c`): `CoInitializeEx`, `CoUninitialize`, system COM state.
3. **IUnknown Runtime** (`ole_iunknown.c`): `QueryInterface`, `AddRef`, `Release` default implementations.
4. **Class Factory Engine** (`ole_factory.c`): `CoCreateInstance`, `CoRegisterClassObject`, class registration.
5. **Object Manager** (`ole_objects.c`): Dynamic object lifetime tracking and active object table.
6. **Interface Manager** (`ole_interfaces.c`): Interface registry and vtable dispatch table.
7. **CLSID Manager** (`ole_clsid.c`): CLSID registry, GUID generators (`CoCreateGuid`), string conversions.
8. **IID Runtime** (`ole_iid.c`): Standard interface identifiers (IID_IUnknown, IID_IClassFactory, IID_IDataObject, IID_IPersist).
9. **Apartment Runtime** (`ole_apartment.c`): STA (Single-Threaded) and MTA (Multi-Threaded) apartment context management.
10. **Marshalling Engine** (`ole_marshal.c`): Cross-thread and cross-apartment proxy/stub object transport (`CoMarshalInterface`).
11. **Storage Engine** (`ole_storage.c`): Structured Storage implementation (`StgCreateStorageEx`, `StgOpenStorage`).
12. **Stream Engine** (`ole_stream.c`): Memory-backed and file-backed streams (`CreateStreamOnHGlobal`, `IStream`).
13. **Clipboard Objects** (`ole_clipboard.c`): OLE Clipboard integration (`OleSetClipboard`, `OleGetClipboard`).
14. **Drag Drop Runtime** (`ole_dragdrop.c`): `DoDragDrop`, `RegisterDragDrop`, `RevokeDragDrop`, `IDropSource`, `IDropTarget`.
15. **Data Object Runtime** (`ole_dataobject.c`): Uniform data transfer via `IDataObject` and `FORMATETC`.
16. **Moniker Runtime** (`ole_moniker.c`): Item, File, and Composite Monikers for object binding.
17. **Property Runtime** (`ole_property.c`): Property sets (`IPropertySetStorage`, `IPropertyStorage`).
18. **Persistence Runtime** (`ole_persist.c`): `IPersist`, `IPersistFile`, `IPersistStream` persistence interfaces.
19. **Diagnostics Engine** (`ole_diagnostics.c`): Reference leaks, memory leaks, and apartment deadlock tracking.
20. **Certification Suite** (`ole32_certification_tests.c`): 350-test production certification suite.
