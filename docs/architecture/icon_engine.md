# ATOMS OS — ICON ENGINE ARCHITECTURAL SPECIFICATION

## 1. Overview & Vision
The **ATOMS Icon Engine** (`kernel/ui/icon_engine/`) is the authoritative, resolution-independent vector rendering subsystem for ATOMS OS. It provides lightweight, deterministic, zero-heap-allocation vector icon generation across the Taskbar, Start Menu, Desktop Shell, System Hub, and future application frameworks.

---

## 2. Core Architectural Principles
1. **Kernel-Safe & Deterministic**:
   - Zero heap allocations (`kmalloc`/`malloc`) in the per-frame render loop.
   - All state modulation and rasterization math execute statically on the stack with bounded execution time.
2. **Resolution Independence & Sub-pixel Anti-Aliasing**:
   - Procedural vector math normalized to floating/fixed point coordinates.
   - Beautiful, razor-sharp rendering at 16x16, 20x20, 24x24, 28x28, 32x32, 48x48, and arbitrary logical display DPIs.
3. **BWE Clipping & Dirty Rect Compatibility**:
   - Full integration with BWE clip rects (`BWE_GetClip`).
   - Never plots pixels outside the specified target bounds.
4. **State Machine Driven**:
   - Distinct, subtle visual state transitions: `NORMAL`, `HOVER`, `PRESSED`, `ACTIVE` (e.g. Start Menu Open), and `DISABLED`.

---

## 3. Subsystem Layout
```
kernel/ui/icon_engine/
├── include/
│   └── icon_engine.h       # Public Engine API, Icon IDs, State enums, and Context
├── src/
│   └── icon_engine.c       # Core Engine Dispatcher, Registry Table & Clipping Resolver
└── icons/
    └── atoms_start_icon.c  # Official ATOMS Start Emblem Procedural Vector Renderer
```

---

## 4. Public API Specification

### Data Structures
```c
typedef enum {
    ICON_ID_NONE = 0,
    ICON_ID_ATOMS_START,
    ICON_ID_EXPLORER,
    ICON_ID_TERMINAL,
    ICON_ID_NOTES,
    ICON_ID_CALCULATOR,
    ICON_ID_SETTINGS,
    ICON_ID_MEDIA_PLAYER,
    ICON_ID_ATRIX,
    ICON_ID_TASK_MANAGER,
    ICON_ID_CONTROL_PANEL,
    ICON_ID_DOOM,
    ICON_ID_GRAPH_3D,
    ICON_ID_MAX
} IconId;

typedef enum {
    ICON_STATE_NORMAL = 0,
    ICON_STATE_HOVER,
    ICON_STATE_PRESSED,
    ICON_STATE_ACTIVE,
    ICON_STATE_DISABLED
} IconState;

typedef struct {
    int32_t         x;            // Destination Top-Left X
    int32_t         y;            // Destination Top-Left Y
    int32_t         width;        // Width in pixels
    int32_t         height;       // Height in pixels
    IconState       state;        // Interaction State
    uint32_t        accent_color; // Optional custom accent override (0 = default)
    const BWE_Rect* clip;         // Active clipping bounds
} IconRenderContext;
```

### Core Functions
- `void IconEngine_Initialize(void)`: Registers built-in icon renderers into the static dispatch table.
- `bool IconEngine_Register(IconId id, IconRendererFn renderer)`: Registers or overrides an icon renderer.
- `bool IconEngine_Render(const BVFramebuffer* fb, IconId id, const IconRenderContext* ctx)`: Renders the requested icon into the target buffer under the active clipping bounds.

---

## 5. ATOMS Start Emblem Geometry & Visual Identity

The **ATOMS Start Emblem** is the visual identity of ATOMS OS:
1. **Central Quantum Nucleus**:
   - Multi-stage radial luminescence (Electric Cyan `#38BDF8` with a central high-contrast `#FFFFFF` core spark).
2. **Tri-Orbital Energy Arcs**:
   - Three interlocking geometric orbital rings oriented at symmetrical geodesic angles ($35^\circ, 155^\circ, 275^\circ$) with anti-aliased sub-pixel stroke calculations.
3. **Harmonic Valence Satellites**:
   - High-luminance quantum satellite nodes anchored on the orbital vertices.
4. **Interactive State Modulation**:
   - **NORMAL**: Crisp electric cyan and icy-white arcs with subtle 15% ambient glow.
   - **HOVER**: Brightened sky cyan (`#7DD3FC`), pure white nodes, and an expanded 30% ambient glow.
   - **PRESSED**: Compressed deep ocean blue (`#0284C7`) core.
   - **ACTIVE**: Full radial activation with pure radiant white orbits and 50% radiant halo.

---

## 6. Integration Points
- **Taskbar (`kernel/ui/task_panel.c`)**:
  - Start button capsule tile (42x38px) houses the 24x24px centered ATOMS emblem.
  - Passes dynamic hover/active states directly to `IconEngine_Render()`.
