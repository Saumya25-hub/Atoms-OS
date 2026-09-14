# EXACT FAULT INSTRUCTION & EXECUTION CONTEXT REPORT

**Document ID:** `EXACT_FAULT_INSTRUCTION.md`  
**Classification:** Phase 2 Forensic Proof & Instruction-Level Disassembly  
**Target:** ATOMS OS x86_64 Kernel Execution Pipeline  

---

## 1. Faulting Call Chain & Virtual Memory Mapping

| Call Level | Function Symbol | Virtual Address (VMA) | Source Location | Execution Context | `RFLAGS.IF` |
|---|---|---|---|---|---|
| **Level 0 (IRQ Entry)** | `isr32` / `isr_common_handler` | `0x100E05` | `arch/x86_64/interrupt/isr_stubs.asm:L114` | Ring 0 Hardware ISR | `0` (Cleared by 0x8E gate) |
| **Level 1 (IRQ Dispatch)** | `irq_dispatch` | `0x19DE40` | `kernel/core/interrupt/src/irq.c:L30` | Ring 0 Hardware ISR | `0` |
| **Level 2 (Timer ISR)** | `timer_tick_handler` | `0x19DF10` | `kernel/core/timer/src/timer.c:L24` | Ring 0 Hardware ISR | `0` |
| **Level 3 (BRE Dispatch)** | `BRE_DispatchPending` | `0x1F86E0` | `kernel/core/brtsl/bre.c:L55` | Ring 0 Hardware ISR | `0` |
| **Level 4 (Input Pump Callback)** | `bre_input_pump_callback` | `0x1A8080` | `kernel/wm/bwe/src/bwe_core.c:L470` | Ring 0 Hardware ISR | `0` |
| **Level 5 (BWE Event Pump)** | `BWE_PumpEvents` | `0x1A80E0` | `kernel/wm/bwe/src/bwe_core.c:L488` | Ring 0 Hardware ISR | `0` |
| **Level 6 (Button Dispatch)** | `bwe_button_event` | `0x1D2150` | `kernel/ui/controls/button/bwe_button.c:L47` | Ring 0 Hardware ISR | `0` |
| **Level 7 (Calculator App)** | `calc_btn_clicked` | `0x23A4E0` | `kernel/shell/desktop_shell/apps.c:L660` | Ring 0 Hardware ISR | `0` |
| **Level 8 (Window Invalidate)** | `BWE_InvalidateWindow` | `0x1A79B0` | `kernel/wm/bwe/src/bwe_core.c:L190` | Ring 0 Hardware ISR | `0` |
| **Level 9 (Compositor Entry)** | `BWE_Compose` | `0x1A87D0` | `kernel/wm/bwe/src/bwe_core.c:L795` | Ring 0 Hardware ISR | `0` |
| **Level 10 (Frame Composition)** | `BWE_ComposeFrame` | `0x1ACFE0` | `kernel/wm/bwe/renderer/bwe_compositor.c:L771` | Ring 0 Hardware ISR | `0` |
| **Level 11 (VRAM Presentation)** | `BOVISUAL_Graphics_SwapFull` | `0x1A5690` | `bovisual/Graphics/graphics.c:L311` | Ring 0 Hardware ISR | `0` |
| **Level 12 (MMIO Memcpy)** | `BSPE_VRAM_CopyEffectiveDamage` | `0x1B1890` | `kernel/graphics/BSPE/Present/vram_copy.c:L150` | Ring 0 Hardware ISR | `0` |

---

## 2. Instruction-Level Disassembly of Critical Fault Boundary

### A. Timer Handler to BRE Dispatch Boundary
**Function:** `timer_tick_handler`  
**Address:** `0x19DF10` (`build/timer.o`)  
```assembly
000000000019df10 <timer_tick_handler>:
  19df10: 55                      pushq   %rbp
  19df11: 48 89 e5                movq    %rsp, %rbp
  19df14: 48 83 ec 50             subq    $0x50, %rsp
  19df18: 48 89 7d f8             movq    %rdi, -0x8(%rbp)
  ...
  19dfa9: e8 32 a7 05 00          callq   0x1f86e0 <BRE_DispatchPending>
  ...
  19dfec: 48 83 c4 50             addq    $0x50, %rsp
  19dff0: 5d                      popq    %rbp
  19dff1: c3                      retq
```

### B. BRE Dispatch to Input Pump Callback
**Function:** `BRE_DispatchPending`  
**Address:** `0x1F86E0` (`build/bre.o`)  
```assembly
00000000001f86e0 <BRE_DispatchPending>:
  1f86e0: 55                      pushq   %rbp
  1f86e1: 48 89 e5                movq    %rsp, %rbp
  1f86e4: 48 83 ec 30             subq    $0x30, %rsp
  ...
  1f8742: 8b 78 08                movl    0x8(%rax), %edi      ; budget parameter (16)
  1f8745: ff 10                   callq   *(%rax)              ; indirect call to bre_input_pump_callback
  ...
```

### C. Event Pump to Synchronous Compose
**Function:** `BWE_PumpEvents`  
**Address:** `0x1A80E0` (`build/bwe_core.o`)  
```assembly
  1a8682: 8b 05 b8 4a 8e 00       movl    0x8e4ab8(%rip), %eax ; g_dirty_rect_count
  1a8688: 85 c0                   testl   %eax, %eax
  1a868a: 75 0c                   jne     0x1a8698
  1a868c: e8 a3 fc ff ff          callq   0x1a8334 <BWE_HasDirtyWindows>
  1a8691: 84 c0                   testb   %al, %al
  1a8693: 74 05                   je      0x1a869a
  1a8695: e8 36 01 00 00          callq   0x1a87d0 <BWE_Compose>
```

---

## 3. Exact Fault Instruction & Hardware State

```text
RIP:                    0x00000000001ACFE0 - 0x00000000001AE089 (BWE_ComposeFrame range)
SYMBOL:                 BWE_ComposeFrame / compose_window_recursive / BSPE_VRAM_CopyEffectiveDamage
OFFSET:                 Inside multi-window Z-order clipping and 8MB PCIe MMIO loop
SOURCE:                 kernel/wm/bwe/renderer/bwe_compositor.c:L771-L1054
INSTRUCTION:            Recursive stack allocation / VRAM MMIO write inside ISR
REGISTERS:
  RFLAGS:               0x0000000000000086 (IF=0, IOPL=0, PF, ZF)
  CS:                   0x0000000000000008 (Ring 0 Kernel Code)
  SS:                   0x0000000000000010 (Ring 0 Kernel Data)
  CR0:                  0x0000000080010033 (Paging, Protection, WP active)
  CR2:                  Stack page boundary fault address
  CR3:                  0x0000000000001000 (Kernel PML4 Base)
  CR4:                  0x0000000000000620 (PAE, OSFXSR, OSXMMEXCPT)
INVALID ADDRESS:        Attempted stack frame push beyond current task stack envelope without IST.
WHY ADDRESS IS INVALID: When an exception or interrupt occurs while already in Ring 0 with ist=0,
                        the CPU pushes SS, RSP, RFLAGS, CS, RIP to the current RSP.
                        Because RSP was low or corrupted by deep recursion, the push failed,
                        causing #PF -> #DF -> TRIPLE FAULT -> vCPU SHUTDOWN.
```
