# ATOMS OS Mouse Subsystem Master Audit

This document provides a complete forensic map of the mouse input and presentation subsystem inside ATOMS OS, based on the current implementation in the repository. 

---

## 1. PS/2 Relative Mouse Path

```
Hardware Interrupt (Mouse)
   │
   ▼
IRQ 12 (mouse_irq_handler) in [drivers/input/ps2/mouse.c]
   │
   ▼
PS/2 Packet Assembly & Sync Checks
   │
   ▼ (dx, dy, buttons)
input_push_relative() in [kernel/drivers/input/input_abstraction.c]
   │
   ▼ (bottom-up Y inversion & coordinate clamping)
input_push_absolute() in [kernel/drivers/input/input_abstraction.c]
   │
   ▼ (abs_x, abs_y, buttons)
kernel_input_push_mouse_absolute() in [kernel/drivers/input/input.c]
   │
   ▼ (BV_EVENT_MOUSE_MOVE/DOWN/UP)
event_queue (Kernel global ring buffer)
   │
   ▼ (Pumped by kernel loop)
input_adapter_pump() in [kernel/drivers/input/core/input_adapter.c]
   │
   ▼ (InputCoreEvent -> input_core_push_event)
Input Core Queue
   │
   ▼ (input_core_dispatch_events)
pointer_engine_on_event() in [kernel/drivers/input/pointer/pointer_engine.c]
   │
   ▼
pointer_motion_process() in [kernel/drivers/input/pointer/pointer_motion.c]
   │
   ▼ (smoothed_event -> dispatcher_push_event)
Dispatcher Queue
   │
   ▼ (dispatcher_pump_events -> dispatcher_router_route)
input_adapter_dispatcher_cb() in [kernel/drivers/input/core/input_adapter.c]
   │
   ▼ (BOHeart_InputCapture -> BOS_ProcessEvent)
BWE_EventQueue
   │
   ▼ (BWE_PumpEvents)
Cursor Update / Hit-testing / Application Delivery
```

### Trace & Details:
*   **Exact File & Functions**: 
    *   [drivers/input/ps2/mouse.c:mouse_irq_handler](file:///D:/Signatures_OS/drivers/input/ps2/mouse.c#L85) - Interrupt service routine for IRQ 12.
    *   [kernel/drivers/input/input_abstraction.c:input_push_relative](file:///D:/Signatures_OS/kernel/drivers/input/input_abstraction.c#L60) - Performs relative accumulation and Y-coordinate sign inversion.
    *   [kernel/drivers/input/input_abstraction.c:input_push_absolute](file:///D:/Signatures_OS/kernel/drivers/input/input_abstraction.c#L35) - Clamps logical coordinates to screen boundaries.
    *   [kernel/drivers/input/input.c:kernel_input_push_mouse_absolute](file:///D:/Signatures_OS/kernel/drivers/input/input.c#L232) - Transitions absolute events to discrete packet structures.
    *   [kernel/drivers/input/core/input_adapter.c:input_adapter_pump](file:///D:/Signatures_OS/kernel/drivers/input/core/input_adapter.c#L77) - Adapts discrete packets for Input Core ingestion.
    *   [kernel/drivers/input/pointer/pointer_engine.c:pointer_engine_on_event](file:///D:/Signatures_OS/kernel/drivers/input/pointer/pointer_engine.c#L4) - Traverses pointer engine stages.
    *   [kernel/drivers/input/pointer/pointer_motion.c:pointer_motion_process](file:///D:/Signatures_OS/kernel/drivers/input/pointer/pointer_motion.c#L14) - Processes sub-pixel coordinates.
    *   [kernel/drivers/input/core/input_adapter.c:input_adapter_dispatcher_cb](file:///D:/Signatures_OS/kernel/drivers/input/core/input_adapter.c#L12) - Legacy event conversion bridge.
    *   [kernel/wm/bwe/src/bwe_core.c:BOHeart_InputCapture](file:///D:/Signatures_OS/kernel/wm/bwe/src/bwe_core.c#L635) - Enqueues to Window Manager.
*   **Global/Static State**:
    *   `mouse_cycle` (static `uint8_t` in `mouse.c`): Packet offset tracking index (`0..2`).
    *   `mouse_byte` (static `uint8_t[3]` in `mouse.c`): Accumulates active packet bytes.
    *   `last_byte_time` (static `uint64_t` in `mouse.c`): Used for 25ms packet timing sync checks.
    *   `g_latest_state` (static `InputState` in `input_abstraction.c`): Holds running cursor coords.
    *   `event_queue` (static `BVEvent[1024]` in `input.c`): Main kernel event ring buffer.
*   **Initialization**: `ps2_mouse_init()` handles device reset, configures sample rate to 200 Hz, sets resolution to 8 counts/mm, 1:1 scaling, and registers IRQ 12 vector.
*   **Drop Conditions**: 
    *   If byte synchronization flag is violated (bit 3 of byte 0 must be 1).
    *   If packet byte interval exceeds 25ms, resetting the assembly phase.
    *   If the target process's GUI event queue overflows.

---

## 2. VMMouse Absolute Path

```
Hypervisor Backdoor Query (inl on port 0x5658)
   │
   ▼
vmmouse_init() in [drivers/input/vmmouse/vmmouse.c] (Switches device to absolute mode)
   │
   ▼
IRQ 12 (mouse_irq_handler) in [drivers/input/ps2/mouse.c]
   │
   ▼ (vmmouse_is_active == true)
vmmouse_read() in [drivers/input/vmmouse/vmmouse.c]
   │
   ▼ (Reads ecx, edx, eax via backdoor command 39)
Raw Coordinate Scaling (0..0xFFFF to 1280x720)
   │
   ▼ (abs_x, abs_y, buttons)
input_push_absolute() in [kernel/drivers/input/input_abstraction.c]
   │
   ▼
kernel_input_push_mouse_absolute() -> event_queue -> InputCore -> BWE
```

### Trace & Details:
*   **Exact File & Functions**:
    *   [drivers/input/vmmouse/vmmouse.c:vmmouse_init](file:///D:/Signatures_OS/drivers/input/vmmouse/vmmouse.c#L44) - Performs hypervisor backdoor probe and enables absolute coordinates.
    *   [drivers/input/vmmouse/vmmouse.c:vmmouse_read](file:///D:/Signatures_OS/drivers/input/vmmouse/vmmouse.c#L94) - Queries mouse state from the backdoor port.
    *   [drivers/input/ps2/mouse.c:mouse_irq_handler](file:///D:/Signatures_OS/drivers/input/ps2/mouse.c#L107) - Calls `vmmouse_read` in a loop when absolute mode is active.
*   **Magic Commands**:
    *   Port: `0x5658` (backdoor IO communication port).
    *   Magic Number: `0x564D5868` (loaded into `EAX`).
    *   Command Status: `40` (BDOOR_CMD_ABSPOINTER_STATUS).
    *   Command Data: `39` (BDOOR_CMD_ABSPOINTER_DATA).
    *   Request Absolute: `0x53424152` (VMMOUSE_CMD_REQUEST_ABSOLUTE).
*   **Global/Static State**:
    *   `g_vmmouse_active` (static `bool` in `vmmouse.c`): Set to `true` on successful backdoor validation.
    *   `g_screen_w`/`h` (static `uint32_t` in `vmmouse.c`): Screen bounds used for scaling.
*   **Coordinate Space**: Raw coords are scaled from the VMMouse space (`0..0xFFFF`) to logical display pixels (`g_screen_w` x `g_screen_h`).
*   **Drop Conditions**: 
    *   If backdoor status register shows less than 4 words available (incomplete packet).

---

## 3. QEMU Actual Runtime Path

### Driver & Mode Selection:
*   **Selected Backend**: **VMMouse (Absolute mode)**.
*   **Backdoor Ingestion**: QEMU emulates the VMware backdoor interface. `vmmouse_init()` successfully validates the backdoor magic, activates absolute mode, and sets `g_vmmouse_active = true`.
*   **Interrupt Line**: IRQ 12.

### Click Mismatch / Drop Analysis:
*   **Why Movement Works**: 
    *   Visual cursor movement coordinates (`Current Cursor X/Y`) are driven by high-frequency hardware interrupts and updated by the BWE compositor loop. This operates independently of the application event loop.
*   **Why Clicks Fail in DOOM**:
    *   **Focus Mismatch**: When DOOM launches, it creates its window `4107` and makes it visible. However, `BOS_ShowWindow` does not set window focus. The active focus remains on the Taskbar Panel (`4105`, owned by PID 0). Key presses (ENTER/ESC) are sent to the Taskbar and dropped.
    *   **Coordinate Hit-Test Failure**: When the user clicks on the DOOM window, BWE's `BWE_ProcessMouseInteraction` is called with raw coordinates (`1920x1080` space) instead of scaled coordinates. The hit-test fails, preventing BWE from setting focus to the DOOM window.
    *   **Queue Saturation**: High-frequency mouse movement events fill DOOM's 64-slot GUI queue. Any mouse click events are dropped due to the queue being full (`PushRaw failed - queue full`).

---

## 4. VMware Actual Runtime Path

### Driver & Mode Selection:
*   **Selected Backend**: **VMMouse (Absolute mode)**.
*   **Backdoor Ingestion**: VMware provides native hardware emulation of the backdoor. `vmmouse_init()` validates the registers and sets `g_vmmouse_active = true`.
*   **Interrupt Line**: IRQ 12.

### Movement Loss Conditions:
*   Absolute pointer packets are only retrieved via the backdoor. If the hypervisor's cursor leaves the VM window boundaries, the backdoor stops receiving packets.
*   At the same time, relative PS/2 data packets from the emulated controller are ignored because `vmmouse_is_active()` is true.
*   If the mouse is released from the VM, coordinates will freeze. However, mouse button clicks can still trigger through the keyboard/aux interface if the hypervisor forwards them.

---

## 5. VirtualBox Actual Runtime Path

### Driver and Fallback Behaviors:
1.  **USB Tablet**:
    *   *Implementation Status*: **NOT SUPPORTED/UNIMPLEMENTED**.
    *   *Analysis*: While `usb_tablet_init()` prints a boot message, the OS has no USB host controller driver (UHCI/OHCI/EHCI) to read HID report descriptors or pull USB packets.
2.  **VMMouse (VMware Backdoor)**:
    *   VirtualBox supports the VMware backdoor, but it must be enabled in the VM configuration. If disabled or unsupported, the backdoor handshake in `vmmouse_init` fails.
3.  **PS/2 Mouse (Relative fallback)**:
    *   If VMMouse init fails, the OS logs `[INPUT] Mouse Device = PS2` and falls back to relative tracking.
    *   Coordinates are updated relative to the center of the screen, with Y-axis inversion handled by `input_push_relative`.

---

## 6. Mouse Initialization Timeline

```
kernel_main() [kernel/kernel.c]
   │
   ├──► 1. kernel_input_init() [kernel/drivers/input/input.c]
   │         ├── Reset queue_head/tail = 0
   │         ├── mouse_engine_init()
   │         ├── input_abstraction_init() -> sets g_latest_state at screen center (640, 360)
   │         ├── usb_tablet_init() -> prints initialization message
   │         ├── keyboard_register_callback()
   │         ├── input_adapter_init()
   │         ├── pointer_engine_init() -> initializes PointerState, accumulator, velocity, bounds, and consumers
   │         ├── dispatcher_init()
   │         ├── cursor_engine_init() -> initializes cursor state, cursor renderer, and software/hardware backends
   │         └── input_adapter_register_pointer_consumer()
   │
   ├──► 2. keyboard_init() [kernel/drivers/keyboard/src/keyboard.c]
   │         └── Hooks keyboard ISR on IRQ 1
   │
   ├──► 3. ps2_mouse_init() [drivers/input/ps2/mouse.c]
   │         ├── Sends 0xA8 to port 0x64 (Enables AUX device)
   │         ├── Sends 0x20/0x60 to port 0x64 (Enables IRQ12 in config byte)
   │         ├── Sends 0xFF (Reset command) -> waits for BAT response (0xAA) and ID (0x00)
   │         ├── Configures Sample Rate (200Hz), Resolution (8 counts/mm), 1:1 Scaling
   │         ├── Enables streaming (0xF4)
   │         └── Hooks mouse_irq_handler on IRQ 12
   │
   └──► 4. vmmouse_init() [drivers/input/vmmouse/vmmouse.c]
             ├── Performs hypervisor backdoor probe
             ├── Requests absolute coordinate mode (0x53424152)
             └── If successful, sets g_vmmouse_active = true and overrides mouse_irq_handler behavior
```

### Critical Override / Reset Rules:
*   `vmmouse_init` must run **after** `ps2_mouse_init`. `ps2_mouse_init` sends a hardware reset command (`0xFF`) which resets the mouse controller to standard 3-byte relative streaming mode. If VMMouse absolute mode was enabled before this reset, it would be overridden and reverted back to relative mode.
*   `input_abstraction_init` sets the starting position to logical `(640, 360)`. Later, the BWE Desktop Shell initialization can update or override the pointer bounds.

---

## 7. IRQ1 / IRQ12 Interaction

### Port Access Rules:
*   **Port `0x60` (Data Port)**: Read by both `keyboard_irq_handler` and `mouse_irq_handler` to retrieve scancodes and mouse packet bytes.
*   **Port `0x64` (Status Register / Command Port)**: Read to check status flags before reading or writing data. Written to send command bytes to the PS/2 controller.

### Keyboard / Mouse Byte Separation (AUX Bit):
The handlers use the status byte from port `0x64` to determine which device sent the data:
*   **AUX OBF Flag (Bit 5, mask `0x20`)**:
    *   If `status & 0x20` is **1**: The data in the buffer came from the mouse.
    *   If `status & 0x20` is **0**: The data came from the keyboard.

### Interrupt Lock-Step Safety:
To prevent one handler from consuming the other device's bytes:
*   **Keyboard ISR (IRQ 1)**:
    ```c
    uint8_t status = io_in8(PS2_STATUS_PORT);
    if (status & 0x20) return 0; // Mouse byte, exit and let IRQ 12 handle it
    ```
*   **Mouse ISR (IRQ 12)**:
    ```c
    uint8_t status = io_in8(PS2_STATUS_PORT);
    if (!(status & 0x20)) break; // Keyboard byte, exit and let IRQ 1 handle it
    ```

### Infinite-Loop / Desynchronization Risks:
*   **Choked Buffers**: If keyboard interrupts are disabled or blocked, keyboard bytes can accumulate in the output buffer. The mouse handler will see `status & 0x01` is true, but since `!(status & 0x20)` is also true, it will exit without draining the byte. This keeps the interrupt line active, causing an interrupt storm or hanging the mouse system.
*   **Byte Alignment Drift**: If the mouse sends a 4-byte packet (e.g. wheel packet) but the driver expects 3-byte packets, `mouse_cycle` will drift out of alignment. The driver will parse packet bytes in the wrong order, causing erratic movement and incorrect button states.

---

## 8. Mouse Packet Formats

### A. Standard PS/2 3-Byte Packet
Mapped in [drivers/input/ps2/mouse.c:mouse_irq_handler](file:///D:/Signatures_OS/drivers/input/ps2/mouse.c#L141-L157):

*   **Byte 0: Flags & Button States**
    *   `Bit 0 (0x01)`: Left Button State (1 = Pressed)
    *   `Bit 1 (0x02)`: Right Button State (1 = Pressed)
    *   `Bit 2 (0x04)`: Middle Button State (1 = Pressed)
    *   `Bit 3 (0x08)`: Always 1 (used for packet synchronization check)
    *   `Bit 4 (0x10)`: X Sign Bit (1 = Negative displacement)
    *   `Bit 5 (0x20)`: Y Sign Bit (1 = Negative displacement)
    *   `Bit 6 (0x40)`: X Overflow Flag
    *   `Bit 7 (0x80)`: Y Overflow Flag
*   **Byte 1: X Displacement (dx)**
    *   Relative change along X axis.
*   **Byte 2: Y Displacement (dy)**
    *   Relative change along Y axis.

### B. VMMouse Backdoor Data Packet
Mapped in [drivers/input/vmmouse/vmmouse.c:vmmouse_read](file:///D:/Signatures_OS/drivers/input/vmmouse/vmmouse.c#L119-L146):

*   **`EAX` (Flags & Buttons)**:
    *   `Bit 5 (0x20)`: Left Button State (1 = Pressed)
    *   `Bit 4 (0x10)`: Right Button State (1 = Pressed)
    *   `Bit 3 (0x08)`: Middle Button State (1 = Pressed)
*   **`EBX` (Size)**:
    *   Number of words to read (set to 4).
*   **`ECX` (Raw X)**:
    *   Absolute coordinate along X axis (`0..0xFFFF`).
*   **`EDX` (Raw Y)**:
    *   Absolute coordinate along Y axis (`0..0xFFFF`).

---

## 9. Button Pipeline

```
Raw Hardware Flags Word
   │
   ▼
[mouse.c / vmmouse.c] -> Decode button mask (buttons)
   │
   ▼
[input.c:kernel_input_push_mouse_absolute]
   │
   ├── Loop (0..2) -> Compare current and previous button masks
   ├── Transitions:
   │     ├── (is_pressed && !was_pressed) -> Set type = BV_EVENT_MOUSE_DOWN
   │     └── (!is_pressed && was_pressed) -> Set type = BV_EVENT_MOUSE_UP
   │
   ▼
[input_adapter.c:input_adapter_pump] -> core_ev.type = INPUT_EVENT_TYPE_BUTTON
   │
   ▼
[pointer_motion.c:pointer_motion_process]
   │
   ├── pointer_buttons_process() updates state machine (clicks count, dragging)
   └── commits to PointerState
   │
   ▼
smoothed_event -> Dispatcher -> input_adapter_dispatcher_cb()
   │
   ▼ (Re-wraps to BVEvent: BV_EVENT_MOUSE_DOWN / BV_EVENT_MOUSE_UP)
BOHeart_InputCapture -> BOS_ProcessEvent -> BWE_EventQueue
   │
   ▼
[bwe_core.c:BWE_PumpEvents]
   │
   ├── bwe_ev.type == BWE_EVENT_MOUSE_DOWN -> BOS_SetFocus(leaf_id)
   └── If owner_pid > 0 -> bos_gui_event_push_raw(owner_pid, &gev)
   │
   ▼ (GUI Event Queue)
[doomgeneric_signaturesos.c:pump_gui_events] -> Pop via sys_gui_get_event()
   │
   ▼ (Maps buttons mask to Doom event_t data1)
D_PostEvent(&ev) (Posts ev_mouse to DOOM event queue)
```

---

## 10. Movement Pipeline

### A. Relative Path (PS/2)
1.  **Ingest**: `mouse_irq_handler` reads `dx` and `dy` from the packet.
2.  **Y-axis Inversion**: `input_push_relative` in `input_abstraction.c` accumulates the offsets:
    ```c
    int32_t new_x = g_latest_state.mouse_x + dx;
    int32_t new_y = g_latest_state.mouse_y - dy; // Subtract dy to invert axis
    ```
3.  **Clamping**: `input_push_absolute` clamps the coordinates to screen bounds:
    ```c
    if (abs_x >= g_screen_width) abs_x = g_screen_width - 1;
    ```
4.  **Ingest to Pointer Engine**: Pushed to the queue as `INPUT_EVENT_TYPE_MOTION_ABSOLUTE`.

### B. Absolute Path (VMMouse)
1.  **Hypervisor Scaling**: `vmmouse_read` scales coordinates from `0..0xFFFF` to logical screen bounds:
    ```c
    *abs_x = (int32_t)(((uint64_t)x_raw * g_screen_w) / 0xFFFF);
    ```
2.  **Pointer Engine Accumulation**:
    *   In `pointer_motion_process()`: Computes deltas:
        ```c
        whole_dx = abs_x - prev_x;
        ```
    *   Subpixel coordinates are converted to 16.16 fixed-point:
        ```c
        sub_x = FP16_FROM_INT(abs_x);
        ```
3.  **Pointer Engine Clamping**: `pointer_bounds_clamp(&new_x, &new_y)` restricts the pointer to active display boundaries.
4.  **BWE Double-Scaling**: 
    *   In `BWE_PumpEvents()`, BWE scales coordinates again using:
        ```c
        mouse_x = (mouse_x * g_kernel_screen_width) / BOVISUAL_Graphics_GetWidth();
        ```
        Since the coordinates are already in logical `1280x720` space, this second scaling pass scales them down to **2/3** of their actual value.
5.  **BWE Click Handling Mismatch**: BWE calls `BWE_ProcessMouseInteraction` using raw coordinates (`bwe_ev.data.mouse.x`), which are in logical `1280x720` space, instead of the scaled coordinates. This causes a mismatch with the visual cursor position.

---

## 11. Pointer Engine

The Pointer Engine is the central manager for pointer coordinates, kinematics, and button states.

*   **Authoritative State Singleton**: Holds coordinates, sub-pixel offsets, velocity, and dragging state:
    ```c
    static PointerState g_pointer_state;
    ```
*   **Precision Accumulator**: Scales movement using fixed-point math:
    ```c
    int64_t scaled_dx = ((int64_t)FP16_FROM_INT(dx_int) * (int64_t)accel_fp16) >> 16;
    ```
*   **Velocity & Kinematics**: Computes movement speed and acceleration factor based on event timestamps in `pointer_velocity.c`.
*   **Clamping Bounds**: Restricts coordinates to active display bounds:
    ```c
    void pointer_bounds_clamp(int32_t* x, int32_t* y);
    ```
*   **Consumer Notification**: Notifies registered consumers when the pointer state changes:
    ```c
    void pointer_consumers_notify(const PointerState* state);
    ```

---

## 12. Cursor Rendering

```
Pointer Engine Coordinates (Logical 1280x720)
   │
   ▼
[cursor_engine_dispatch_cb] -> Update cursor_state position
   │
   ▼
[cursor_renderer_update] -> BSPE_CursorPresenter_UpdatePosition
   │
   ▼
Hypervisor / Software Branch:
   ├── If Hardware: BSPE_CursorPlane_SetPosition(x, y) [direct VRAM hardware overlay]
   └── If Software: BSPE_CursorPresenter_OnCompositorRedraw [blits to RAM backbuffer]
```

### Visual Cursor Independence:
*   The cursor rendering pipeline is driven by hardware interrupts and BWE's compositor loop.
*   Because BWE updates and redraws the cursor on every compositor frame, **the visual cursor moves smoothly on screen** even if the target application is hanging or its event queue is full.

---

## 13. Application Delivery

```
BWE Event (BWE_PumpEvents)
   │
   ▼
BWE_HitTest() -> Traverses Z-stack to find target window ID (leaf_id)
   │
   ▼
If target->owner_pid > 0:
   │
   ▼ (Constructs BOS_GUIEvent gev)
bos_gui_event_push_raw(target->owner_pid, &gev)
   │
   ▼ (Pushed to process's 64-slot ring buffer)
BOS_GUIEventQueue g_queues[pid]
   │
   ▼
Userspace: DOOM calls sys_gui_get_event(&gev) -> maps to Doom event_t -> D_PostEvent()
```

### Queue Details & Drop Conditions:
*   **Queue Capacity**: `MAX_GUI_EVENTS_PER_QUEUE` is defined as **`64`** in [kernel/ui/events/gui_events.h](file:///D:/Signatures_OS/kernel/ui/events/gui_events.h#L36).
*   **Queue Full Drop**: If the queue is full (`next == q->head` in `bos_gui_event_push_raw`), the event is dropped. High-frequency mouse movements can easily fill this queue, causing click events to be discarded.

---

## 14. Current Backend Selection Logic

The kernel decides which mouse backend driver to use during system boot:

```c
// Decided during boot in kernel_main [kernel/kernel.c]
void kernel_main(...) {
    // 1. Initialize PS/2 Mouse hardware registers
    ps2_mouse_init(); 

    // 2. Probe hypervisor backdoor for VMMouse
    bool vmmouse_ok = vmmouse_init(g_kernel_screen_width, g_kernel_screen_height);
    if (vmmouse_ok) {
        // vmmouse_init sets g_vmmouse_active = true
        display_print("[INPUT] Mouse Device = VMMouse (Absolute)\n");
    } else {
        // g_vmmouse_active remains false
        display_print("[INPUT] Mouse Device = PS2\n");
    }
}
```

*   **Global Driver Flag**: `g_vmmouse_active` (defined in `vmmouse.c`).
    *   Set to `true` by `vmmouse_init()` if the VMware backdoor validation passes.
    *   Read by `vmmouse_is_active()` in `vmmouse.c`.
    *   Used in `mouse_irq_handler` in `mouse.c` to choose between absolute backdoor reads or relative PS/2 packet decoding.

---

## 15. Cross-Hypervisor Comparison Table

| Hypervisor / VM Target | Emulated Mouse Device | IRQ Source | Selected ATOMS Driver | Movement Source | Button Source | Coordinate Type | Known Failure Boundary |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **QEMU** | VMMouse (Standard) | IRQ 12 | VMMouse (Absolute) | Backdoor Command 39 | Backdoor Command 39 | Absolute (`0..0xFFFF` scaled to screen) | BWE Window hit-testing failure due to scaling mismatch and GUI queue saturation. |
| **VirtualBox** | PS/2 Mouse (VMMouse fallback) | IRQ 12 | PS/2 (Relative) | Standard 3-Byte stream | Standard 3-Byte stream | Relative (accumulated to screen pixels) | Double scaling mismatch in BWE. |
| **VMware** | VMMouse (Native) | IRQ 12 | VMMouse (Absolute) | Backdoor Command 39 | Backdoor Command 39 | Absolute (`0..0xFFFF` scaled to screen) | Cursor coordinates freeze if focus leaves the window, but button clicks can still trigger. |

---

## 16. Dependency Graph & File Inventory

### Text-Based Module Dependency Graph
```
 ┌────────────────────────────────────────┐
 │            Hardware Drivers            │
 └───────┬─────────┬──────────────┬───────┘
         │         │              │
         │ (IRQ1)  │ (IRQ12)      │ (Backdoor)
         ▼         ▼              ▼
    ┌──────────┐┌──────────┐ ┌──────────┐
    │ Keyboard ││PS/2 Mouse│ │ VMMouse  │
    └────┬─────┘└────┬─────┘ └────┬─────┘
         │           │            │
         ▼           ▼            ▼
     ┌────────────────────────────────┐
     │      Input Abstraction         │
     │      [input_abstraction.c]     │
     └───────────────┬────────────────┘
                     │ (Discrete BVEvent packets)
                     ▼
     ┌────────────────────────────────┐
     │      Global Event Queue        │
     │        [input.c]               │
     └───────────────┬────────────────┘
                     │ (input_adapter_pump)
                     ▼
     ┌────────────────────────────────┐
     │      Input Core Adapter        │
     │       [input_adapter.c]        │
     └───────────────┬────────────────┘
                     │ (INPUT_EVENT_TYPE_*)
                     ▼
     ┌────────────────────────────────┐
     │        Pointer Engine          │
     │      [pointer_engine.c]        │
     └───────────────┬────────────────┘
                     │ (pointer_consumers_notify)
                     ▼
     ┌────────────────────────────────┐
     │       Event Dispatcher         │
     │        [dispatcher.c]          │
     └───────┬────────────────┬───────┘
             │                │
             │ (Tier 1)       │ (Tier 3)
             ▼                ▼
     ┌──────────────┐ ┌──────────────┐
     │Cursor Engine │ │  BWE Adapter │
     └───────┬──────┘ └───────┬──────┘
             │                │
             ▼                ▼
     ┌──────────────┐ ┌──────────────┐
     │Cursor Render │ │ BWE Core (WM)│
     └──────────────┘ └───────┬──────┘
                              │ (bos_gui_event_push_raw)
                              ▼
                      ┌──────────────┐
                      │  GUI Queues  │
                      └───────┬──────┘
                              │ (sys_gui_get_event)
                              ▼
                      ┌──────────────┐
                      │  DOOM App    │
                      └──────────────┘
```

### Source File Inventory

| File Name | Location | Architectural Responsibility |
| :--- | :--- | :--- |
| **`mouse.c`** | `drivers/input/ps2/` | Handles PS/2 mouse initialization, registers the IRQ 12 ISR, and decodes standard 3-byte relative packets. |
| **`vmmouse.c`** | `drivers/input/vmmouse/` | Detects hypervisor backdoor, switches device to absolute mode, and reads coordinates from registers. |
| **`usb_tablet.c`** | `drivers/input/usb_tablet/` | Dummy absolute input wrapper. Unused due to missing USB controller driver. |
| **`input.c`** | `kernel/drivers/input/` | Manages the global event queue and transitions raw coordinates to discrete events. |
| **`input_abstraction.c`** | `kernel/drivers/input/` | Normalizes coordinates, applies Y-axis sign inversion, and clamps bounds. |
| **`input_adapter.c`** | `kernel/drivers/input/core/` | Adapts discrete events for Input Core, dispatches to Pointer Engine, and pumps dispatcher. |
| **`pointer_engine.c`** | `kernel/drivers/input/pointer/` | Coordinates the 7-stage pointer kinematics pipeline. |
| **`pointer_motion.c`** | `kernel/drivers/input/pointer/` | Processes sub-pixel accumulation, ballistic acceleration, and clamping. |
| **`pointer_state.c`** | `kernel/drivers/input/pointer/` | Authoritative singleton holding current mouse coordinates and button states. |
| **`pointer_precision.c`**| `kernel/drivers/input/pointer/` | Implements 16.16 fixed-point subpixel accumulator. |
| **`pointer_velocity.c`** | `kernel/drivers/input/pointer/` | Calculates instant pointer velocity and ballistic acceleration factors. |
| **`pointer_buttons.c`** | `kernel/drivers/input/pointer/` | Evaluates button clicks, dragging state, and transition states. |
| **`pointer_bounds.c`** | `kernel/drivers/input/pointer/` | Enforces display boundaries on coordinates. |
| **`pointer_consumers.c`**| `kernel/drivers/input/pointer/` | Manages consumer registration and notifications. |
| **`dispatcher.c`** | `kernel/drivers/input/dispatcher/`| Coordinates event routing through prioritized consumer tiers. |
| **`cursor_engine.c`** | `kernel/drivers/input/cursor/` | Manages visual cursor state and updates backends. |
| **`cursor_backend.c`** | `kernel/drivers/input/cursor/` | Controls the hardware cursor plane or switches to software fallback. |
| **`cursor_renderer.c`** | `kernel/drivers/input/cursor/` | Updates cursor state and initiates software cursor blits. |
| **`bspe_cursor_present.c`**| `kernel/graphics/BSPE/Cursor/` | Draws the software cursor sprite or updates hardware plane. |
| **`bwe_core.c`** | `kernel/wm/bwe/src/` | Routes GUI events to owner PIDs and updates window focus on click. |
| **`gui_events.c`** | `kernel/ui/events/` | Implements thread-safe ring buffers for process GUI events. |
| **`doomgeneric_signaturesos.c`**| `userspace/apps/doom/` | OS-specific wrapper for DOOM that pops events and translates keycodes. |
