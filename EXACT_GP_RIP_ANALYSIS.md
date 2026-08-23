# ATOMS OS — EXACT RIP FORENSIC AUTOPSY (`0x19BC87`)

## 1. Executive Summary

| Parameter | Forensic Value |
| :--- | :--- |
| **Fault Vector** | `#GP (13) — General Protection Fault` |
| **Faulting RIP** | `0x000000000019BC87` |
| **Source File** | [`arch/x86_64/interrupt/isr_stubs.asm`](file:///d:/Signatures_OS/arch/x86_64/interrupt/isr_stubs.asm#L171) |
| **Source Line** | Line 171 |
| **Assembly Instruction** | `iretq` (`48 cf`) |
| **Containing Function** | `isr_common_stub` |
| **Containing Object** | `build/isr_stubs.o` |
| **Error Code** | `0x0000000000006E90` / `0x0000000000009F00` |
| **Privilege Level** | CPL 0 (Kernel Mode) |

---

## 2. Linker Map & Object Resolution

From `build/kernel.map`:
```text
build/isr_stubs.o:(.text)
  0x19bbf1      isr_common_stub
  0x19bc18      isr_common_stub.no_switch
  0x19bc60      isr_common_stub.kernel_segments
  0x19bc6c      isr_common_stub.restore_gprs
  0x19bc87      <iretq instruction>
  0x19bc89      isr_stub_table
```

Disassembly of `build/isr_stubs.o` (`llvm-objdump -d`):
```nasm
0000000000000a41 <isr_common_stub>:
     a41: 50                            push    rax
     a42: 53                            push    rbx
     a43: 51                            push    rcx
     a44: 52                            push    rdx
     a45: 56                            push    rsi
     a46: 57                            push    rdi
     a47: 55                            push    rbp
     a48: 41 50                         push    r8
     a4a: 41 51                         push    r9
     a4c: 41 52                         push    r10
     a4e: 41 53                         push    r11
     a50: 41 54                         push    r12
     a52: 41 55                         push    r13
     a54: 41 56                         push    r14
     a56: 41 57                         push    r15
     a58: 48 89 e7                      mov     rdi, rsp
     a5b: e8 00 00 00 00                call    0xa60 <isr_common_stub+0x1f>
     a60: 48 85 c0                      test    rax, rax
     a63: 74 03                         je      0xa68 <.no_switch>
     a65: 48 89 c4                      mov     rsp, rax

0000000000000a68 <.no_switch>:
     a68: f6 84 24 90 00 00 00 03       test    byte ptr [rsp + 144], 3
     a70: 74 3e                         je      0xab0 <.kernel_segments>
     a72: 48 c7 84 24 90 00 00 00 23 00 00 00 mov qword ptr [rsp + 144], 35
     a7e: 48 c7 84 24 a8 00 00 00 1b 00 00 00 mov qword ptr [rsp + 168], 27
     a8a: 48 81 a4 24 98 00 00 00 d5 0c 00 00 and qword ptr [rsp + 152], 3285
     a96: 48 81 8c 24 98 00 00 00 02 02 00 00 or  qword ptr [rsp + 152], 514
     aa2: 66 b8 1b 00                   mov     ax, 27
     aa6: 8e d8                         mov     ds, eax
     aa8: 8e c0                         mov     es, eax
     aaa: 8e e0                         mov     fs, eax
     aac: 8e e8                         mov     gs, eax
     aae: eb 0e                         jmp     0xabe <.restore_gprs>

0000000000000ab0 <.kernel_segments>:
     ab0: 66 b8 10 00                   mov     ax, 16
     ab4: 8e d8                         mov     ds, eax
     ab6: 8e c0                         mov     es, eax
     ab8: 8e e0                         mov     fs, eax
     aba: 8e e8                         mov     gs, eax

0000000000000abe <.restore_gprs>:
     abe: 41 5f                         pop     r15
     ac0: 41 5e                         pop     r14
     ac2: 41 5d                         pop     r13
     ac4: 41 5c                         pop     r12
     ac6: 41 5b                         pop     r11
     ac8: 41 5a                         pop     r10
     aca: 41 59                         pop     r9
     acc: 41 58                         pop     r8
     ace: 5d                            pop     rbp
     acf: 5f                            pop     rdi
     ad0: 5e                            pop     rsi
     ad1: 5a                            pop     rdx
     ad2: 59                            pop     rcx
     ad3: 5b                            pop     rbx
     ad4: 58                            pop     rax
     ad5: 48 83 c4 10                   add     rsp, 16
     ad7: 48 cf                         iretq
```

---

## 3. Disassembly in Context (`0x19BC00 - 0x19BD20`)

```text
0x000000000019bc6c <.restore_gprs>:
0x0019bc6c: 41 5f             pop    r15
0x0019bc6e: 41 5e             pop    r14
0x0019bc70: 41 5d             pop    r13
0x0019bc72: 41 5c             pop    r12
0x0019bc74: 41 5b             pop    r11
0x0019bc76: 41 5a             pop    r10
0x0019bc78: 41 59             pop    r9
0x0019bc7a: 41 58             pop    r8
0x0019bc7c: 5d                pop    rbp
0x0019bc7d: 5f                pop    rdi
0x0019bc7e: 5e                pop    rsi
0x0019bc7f: 5a                pop    rdx
0x0019bc80: 59                pop    rcx
0x0019bc81: 5b                pop    rbx
0x0019bc82: 58                pop    rax
0x0019bc83: 48 83 c4 10       add    rsp, 0x10
0x0019bc87: 48 cf             iretq  <-- [FAULT OCCURS HERE]
0x0019bc89:                   isr_stub_table: ...
```

---

## 4. Exact Fault Mechanism

When the CPU executes `iretq` at `0x19BC87`:
1. It pops `RIP` from `[RSP + 0]`.
2. It pops `CS` from `[RSP + 8]`.
3. It pops `RFLAGS` from `[RSP + 16]`.
4. It pops `RSP` from `[RSP + 24]`.
5. It pops `SS` from `[RSP + 32]`.

Because the stack had been misaligned due to a nested interrupt re-entry (caused by `cursor_state_unlock()` emitting `sti` during an ISR), the `iretq` instruction popped a stack address offset (`0x6E90` / `0x9F00`) as a segment selector into `SS` / `CS`.

Loading the non-existent selector `0x6E90` into `SS` triggered `#GP(0x6E90)`.
