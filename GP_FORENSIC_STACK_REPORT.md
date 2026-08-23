# ATOMS OS — GENERAL PROTECTION FAULT STACK FORENSIC REPORT

## 1. Fault Register Frame

```text
Vector : #GP (13) — General Protection Fault
Error  : 0x0000000000006E90  (Low 16 bits of Stack Pointer offset 0x11086E90)
RIP    : 0x000000000019BC87  (iretq in isr_common_stub)
RSP    : 0x0000000011086E38  (Faulting stack pointer before iretq pop completion)
CS     : 0x0000000000000008  (Kernel Code Segment)
SS     : 0x0000000000000010  (Kernel Data Segment)
RFLAGS : 0x0000000000010002  (IOPL=0, IF=0)
CR2    : 0x0000000000000000
CR3    : 0x0000000011087000  (Active task PML4 page directory)
```

---

## 2. Stack Reconstruction Around `RSP = 0x11086E38`

```text
+-----------------------+--------------------+-------------------------------------------+
| Stack Memory Address  | Value Stored       | Architectural Meaning                      |
+-----------------------+--------------------+-------------------------------------------+
| 0x0000000011086E38    | 0x00000000001F7138 | Supposed RIP (Actually RIP of scheduler)  |
| 0x0000000011086E40    | 0x0000000000000008 | Supposed CS                              |
| 0x0000000011086E48    | 0x0000000000000216 | Supposed RFLAGS                          |
| 0x0000000011086E50    | 0x0000000011086E90 | Supposed RSP (Calculated from outer frame)|
| 0x0000000011086E58    | 0x0000000000006E90 | Corrupted SS QWORD (Error Code 0x6E90!)   |
+-----------------------+--------------------+-------------------------------------------+
```

---

## 3. Forensic Anatomy of the Stack Pointer and Error Code

1. **Stack Page Range**:
   - Kernel stack for this task: `0x11086000` - `0x11086FFF` (4096 / 32768 byte buffer).
   - `RSP` at fault: `0x11086E38`.
2. **Error Code `0x6E90`**:
   - `0x6E90` is exactly the lowest 16 bits of `0x11086E90`, an address pointing inside this task's kernel stack.
   - When x86_64 executes `iretq`, it attempts to load segment registers (including `SS`).
   - If the value in the `SS` slot is not a valid segment selector present in the GDT (e.g. `0x08`, `0x10`, `0x1B`, `0x23`), the CPU issues a `#GP` with the faulty value as the error code.
   - The CPU popped `0x6E90` from `[RSP + 32]`, triggering `#GP(0x6E90)`.

---

## 4. Stack Shift Sequence

1. A task was executing inside `sys_service_yield` $\to$ `scheduler_yield` with `sti; hlt`.
2. Hardware IRQ 0 (Timer) fired.
3. The interrupt handler entered `timer_tick_handler()`.
4. `timer_tick_handler()` called `bos_cursor_tick()` $\to$ `cursor_state_unlock()` $\to$ **`sti`**.
5. With `IF=1` active inside the ISR, a PS/2 Mouse IRQ (IRQ 12) arrived before `timer_tick_handler()` finished.
6. A nested interrupt frame was pushed on top of the incomplete ISR frame on the same stack.
7. Context save recorded the top-of-stack from the nested frame.
8. Upon context restore, the stack was read out of alignment by one frame, shifting the QWORDs read by `iretq` and popping a stack offset into `SS`.
