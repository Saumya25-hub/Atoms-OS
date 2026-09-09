# FORENSIC REPORT: Root Cause Analysis of Chromium Browser Ring-3 Crash (#PF at 0x40006E40)

## Executive Summary
During automated QEMU UEFI testing of the real Chromium browser launch (`chromium_browser.elf`), the application was cleanly spawned into userspace as `PID=201` at `CPL=3` (Ring 3), but immediately encountered a Page Fault `#PF` at RIP `0x40006E40` (`atoms_heap_init`) with `CR2 = 0x0000000000000000` and Error Code `0x0000000000000005`.

Forensic memory dump revealed that the physical frame backing virtual address `0x40006000` (`phys=0x23ED3000`) had its code bytes at offset `0xE40` (`0x40006E40`) wiped to all zeroes (`00 00 00 00 00 00 00 00 00 00`), causing the CPU to decode `add %al, (%rax)` with `%rax = 0`, provoking an access violation on address `0x0`.

Detailed forensic inspection of the linker map, memory layout, and runtime logs confirmed that the root cause is **Kernel Boot Stack Overflow clobbering the embedded ELF payload in memory prior to launch**.

---

## Forensic Evidence & Chain of Causality

### 1. The Binary ELF Payload on Disk is 100% Valid
Inspection of `build/chromium_browser.elf` using `llvm-objdump` and raw byte analysis:
- Entry point `_start` (`0x400001e0`) calls `_start_c` (`0x40000060`).
- `_start_c` calls `atoms_heap_init` at `0x40006e40`.
- At offset `0x7e40` in `build/chromium_browser.elf`, the opcode sequence is:
  `55 48 89 e5 48 c7 05 d1 41 00 00 00 00 00 00 c7 05 cf 41 00 00 00 00 00 00 5d c3`
  (`push %rbp; mov %rsp, %rbp; movq $0, s_free_list_head(%rip); movl $0, s_heap_lock(%rip); pop %rbp; retq`).
- These opcodes are valid, non-zero x86_64 instructions.

### 2. The Linker Map Layout Explains the Physical Memory Placement
From `build/kernel.map`:
```
Address          VSize     File / Section / Symbol
112a830          143e8     build/embedded_desktop_elf.o:(.data)
113ec20           ce68     build/embedded_chromium_elf.o:(.data)
113ec20                    g_embedded_chromium_elf
114ba80                    g_embedded_chromium_elf_end
114c000       21a97590     .bss (_bss_start)
114c000           4000     build/kernel_entry.o:(.bss)
114c000                    boot_stack_bottom
1150000                    boot_stack_top
```

Key Observations:
1. `g_embedded_chromium_elf` was defined in `kernel/embedded_chromium_elf.asm` under `section .data`.
2. As a consequence, it was placed at the very end of `.data` (`0x113EC20` .. `0x114BA80`).
3. Immediately adjacent to `.data` is `.bss`, starting at `0x114C000`.
4. The first object in `.bss` is `boot_stack_bottom` (`0x114C000`) with size `16384` bytes (16 KB), ending at `boot_stack_top` (`0x1150000`).
5. The distance between `g_embedded_chromium_elf_end` and `boot_stack_bottom` is only 1,408 bytes (`0x580`).

### 3. Runtime Kernel Boot Stack Overflow
1. When ATOMS OS boots, `kernel_entry.asm` initializes `RSP` to `boot_stack_top` (`0x1150000`).
2. The kernel executes deep nested initialization sequences:
   - ACPI, APIC, PIC, PIT, RTC
   - PMM stress test
   - VMM page table construction and stress test
   - PCI bus enumeration
   - USB subsystem (XHCI controller, DMA ring buffers, device enumeration, HID keyboard and mouse)
   - Realtek network drivers, IPv4, TCP, TLS 1.2 handshake, RSA cryptographic signature verification
   - VFS mount (MBR/GPT parsing, FAT32 mount)
   - BWE window manager and desktop shell initialization
   - Desktop Shell usermode process spawn (`PID=200`)
3. This extensive initialization call tree consumes significantly more than 16 KB of stack space.
4. Because the stack grows downwards (`RSP` decreases from `0x1150000`), when stack depth exceeded 16 KB, `RSP` crossed below `boot_stack_bottom` (`0x114C000`) and continued down into `0x114BA80` .. `0x1145C20`.
5. Pushes and local buffer writes on the kernel stack directly overwritten `g_embedded_chromium_elf` in memory.
6. The runtime log from `build/qmp_chromium_click.log` line 2160 provides irrefutable evidence:
   - Pages 0 to 5 of the ELF image (`0x1000` to `0x6000`, memory `0x113FC20` to `0x1144C20`) matched the ELF file on disk with 100% byte-for-byte fidelity.
   - At page 6 (`vaddr=0x40006000`, file offset `0x7000`, physical memory `0x1145C20`), the first word read was `0x22C7883C` (a kernel heap/stack pointer) instead of the ELF opcode word `0x100fe814100`.
   - At offset `0x7E40` (`atoms_heap_init`), the memory had been zeroed by stack frame allocation (`00 00 00 ...`).

---

## Files Involved
1. `kernel/kernel_entry.asm`:
   - Contains `boot_stack_bottom: resb 16384` (16 KB). Critically undersized for a monolithic kernel running full graphics, network, and TLS subsystems on the primary thread.
2. `kernel/embedded_chromium_elf.asm`:
   - Places `g_embedded_chromium_elf` in `section .data`, leaving it positioned directly beneath `.bss` and vulnerable to stack downward growth.
3. `kernel/embedded_desktop_elf.asm`:
   - Places `g_embedded_desktop_elf` in `section .data` with identical vulnerability.
4. `kernel/linker.ld`:
   - Does not isolate the boot stack with a protective guard gap or place embedded binary payloads in a safe read-only section.

---

## Risk Analysis
- **Severity**: Critical blocker. Any application payload located near the end of `.data` is silently corrupted at boot time before invocation.
- **Impact on Other Subsystems**: Low risk of regression if resolved cleanly. Expanding the kernel boot stack to 256 KB and moving immutable ELF payloads to `section .rodata` guarantees spatial isolation without altering any scheduling, memory management, or hardware device driver logic.

---

## Suspected Fix (High-Level Architecture - NO CODE)
1. **Move Embedded Payloads to `.rodata`**:
   - Change section definition in `kernel/embedded_chromium_elf.asm` and `kernel/embedded_desktop_elf.asm` from `.data` to `.rodata`.
   - In the linker script, `.rodata` precedes `.data` and `.bss`, placing the payload megabytes away from any stack structures.
2. **Expand Kernel Boot Stack**:
   - Increase `boot_stack` in `kernel/kernel_entry.asm` from 16 KB (`16384`) to 256 KB (`262144`).
3. **Add Stack Guard Isolation**:
   - Align stack boundaries and ensure ample headroom so that even the most complex TLS/crypto/VFS initialization sequences cannot exhaust stack memory.
