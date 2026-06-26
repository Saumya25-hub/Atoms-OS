# BOSurface API Specification (SDK Freeze)

**Version:** 1.0  
**Status:** Frozen  

This document serves as the Single Source of Truth for the BOSurface SDK.
It is designed for the **Human Developer**, the **ACE Generator**, and the **Visual Forge UI Editor**.

All generated applications and native applications MUST use these APIs for UI composition. No direct framebuffer drawing is permitted.

---

## 1. Surface Management APIs

### `BOS_CreateSurface`
Allocates and initializes a new top-level or child surface.

```c
bwe_error_t BOS_CreateSurface(
    uint32_t parent_id,
    uint32_t x, uint32_t y,
    uint32_t width, uint32_t height,
    uint32_t flags,
    uint32_t* out_surface_id
);
```
- **Parameters:**
  - `parent_id`: ID of the parent surface (0 for Desktop/Root).
  - `x, y, width, height`: Initial geometry relative to parent.
  - `flags`: Creation flags (e.g., `BWE_FLAG_VISIBLE`, `BWE_FLAG_ALPHA`).
  - `out_surface_id`: Pointer to store the generated unique SurfaceID.
- **Return Value:** `BWE_SUCCESS` (0) or error code (e.g., `BWE0004` Memory Allocation Failed).
- **Ownership Rules:** The created surface is owned by the BWE Kernel Heap. The parent surface tracks it as a child.

### `BOS_DestroySurface`
Safely destroys a surface and all its children.

```c
bwe_error_t BOS_DestroySurface(uint32_t surface_id);
```
- **Parameters:**
  - `surface_id`: ID of the surface to destroy.
- **Return Value:** `BWE_SUCCESS` or `BWE0001` (Invalid Surface Pointer).
- **Ownership Rules:** Recursively frees memory for all child surfaces and controls. Do not use `surface_id` after this call.

---

## 2. Control Generation APIs

These APIs are the primary targets for the ACE Generator when converting Visual Forge C# models.

### `BOS_CreatePanel`
```c
bwe_error_t BOS_CreatePanel(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color_bg, uint32_t* out_control_id);
```

### `BOS_CreateButton`
```c
bwe_error_t BOS_CreateButton(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* text, uint32_t* out_control_id);
```

### `BOS_CreateLabel`
```c
bwe_error_t BOS_CreateLabel(uint32_t parent_id, uint32_t x, uint32_t y, const char* text, uint32_t color_fg, uint32_t* out_control_id);
```

### `BOS_CreateTextbox`
```c
bwe_error_t BOS_CreateTextbox(uint32_t parent_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const char* placeholder, uint32_t* out_control_id);
```

**Common Rules for Controls:**
- Controls are tightly bound to their `parent_id` (must be a valid Surface).
- `out_control_id` returns a globally unique ID for the control.
- Controls are purely state containers. The BWE Theme Engine performs actual rendering.

---

## 3. Geometry and State APIs

### `BOS_SetBounds`
```c
bwe_error_t BOS_SetBounds(uint32_t target_id, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
```
- **Target:** Can be a SurfaceID or ControlID.
- **Action:** Triggers an Invalidation Queue event for the old and new region.

### `BOS_SetText`
```c
bwe_error_t BOS_SetText(uint32_t target_id, const char* text);
```
- **Target:** Button, Label, or Textbox ID.
- **Action:** Updates internal state and invalidates the control bounds.

### `BOS_Show` & `BOS_Hide`
```c
bwe_error_t BOS_Show(uint32_t target_id);
bwe_error_t BOS_Hide(uint32_t target_id);
```
- **Action:** Modifies visibility flags and triggers occlusion recalculation.

---

## 4. Error Codes Reference

| Code | Hex | Description |
|---|---|---|
| `BWE_SUCCESS` | `0x0000` | Operation successful |
| `BWE0001` | `0x1001` | Invalid Surface Pointer / ID |
| `BWE0002` | `0x1002` | Invalid Parent ID |
| `BWE0003` | `0x1003` | Render Failed |
| `BWE0004` | `0x1004` | Memory Allocation Failed |
| `BWE0005` | `0x1005` | Focus Error |
| `BWE0006` | `0x1006` | Dirty Region Overflow |
| `BWE0007` | `0x1007` | Invalid Control ID |
| `BWE0008` | `0x1008` | Surface Already Exists |

---

## 5. Thread Safety (Future)

In the current version, all `BOS_*` API calls must be executed sequentially on the main kernel thread. In future multithreaded iterations, these APIs will utilize IPC queues to submit commands safely to the BWE Compositor Thread.
