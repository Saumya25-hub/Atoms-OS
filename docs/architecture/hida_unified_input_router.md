# ATOMS OS — HIDA Unified Input Router & Fail-Safe Supervisor Specification

## 1. Executive Overview

The **HIDA (Hardware Input Driver Abstraction) Unified Input Router** serves as the central input arbitration authority in ATOMS OS. It decouples low-level hardware input drivers (USB HID Mouse/Keyboard, PS/2 Mouse/Keyboard, VMware VMMouse) from high-level input consumers (`PointerEngine`, `EventDispatcher`, `KeyboardEngine`).

HIDA enforces **Dynamic Health Scoring** and **Silence Timeout Auto-Fallback**, guaranteeing that if a hardware driver stalls, disconnects, or suffers latency degradation, the input subsystem seamlessly migrates ownership to an active fallback driver without hanging the kernel.

---

## 2. Priority Scoring & Arbitration Matrix

```
+-------------------------------------------------------------------------+
| BACKEND ID            | DEVICE TYPE           | PRIORITY SCORE | STATUS |
+-------------------------------------------------------------------------+
| HIDA_BACKEND_VMMOUSE  | VMMouse Absolute      | 100            | Active |
| HIDA_BACKEND_USB_KBD  | USB HID Keyboard      | 85             | Active |
| HIDA_BACKEND_USB      | USB HID Mouse/Tablet  | 80             | Active |
| HIDA_BACKEND_PS2_KBD  | 8042 PS/2 Keyboard    | 65             | Fallbk |
| HIDA_BACKEND_PS2      | 8042 PS/2 Mouse       | 60             | Fallbk |
+-------------------------------------------------------------------------+
```

---

## 3. Silence Timeout Auto-Fallback Algorithm

```c
// Evaluated on every incoming input event packet
void hida_arbitrate(uint32_t incoming_backend_id) {
    uint64_t now = timer_get_ticks();
    
    for (int i = 0; i < device_count; i++) {
        InputDeviceDescriptor* dev = &registry[i];
        
        // Mark inactive if silence exceeds 3000ms (3 seconds)
        if ((now - dev->last_event_tick) >= 3000) {
            dev->is_eligible = false;
            dev->status = HIDA_STATE_FALLBACK;
        }
    }

    // Select eligible device with highest priority score
    uint32_t new_owner = select_highest_priority_eligible_device();
    
    if (new_owner != current_owner) {
        log_auto_fallback_event(current_owner, new_owner);
        current_owner = new_owner;
    }
}
```

---

## 4. Vizier Governance Integration

HIDA registers as a Tier 0 Consumer under the **Vizier Operating System Governance Architecture**:
- Subsystem ID: `110` (`HIDA`).
- Authoritative Capability Token: `VIZIER_CAP_INPUT_POINTER_RAW`.
- All input events pass through Vizier Contract validation before dispatching to Ring 3 userspace applications.
