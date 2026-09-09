# PATCH PLAN: Elimination of Kernel Boot Stack Overflow and Protection of Embedded Application Payloads

## Reference
- Input Document: `FORENSIC_REPORT.md`
- Target Milestone: Real Chromium Browser Ring-3 Execution Certification

---

## 1. Modifications Planned

### Component 1: Kernel Boot Stack Expansion
- **File to Modify**: `kernel/kernel_entry.asm`
- **Target Location**: Lines 17–22 (`section .bss`)
- **Modification**:
  - Increase reservation size for `boot_stack_bottom` from `16384` (16 KB) to `262144` (256 KB).
  - Maintain 16-byte alignment (`align 16`).
- **Rationale**:
  - 16 KB is inadequate for kernel initialization sequences that include TLS 1.2 handshakes, RSA PKCS#1 v1.5 verification, XHCI context management, and deep VFS calls. Expanding to 256 KB provides a 16x safety factor, preventing any downward stack growth from escaping its designated `.bss` memory bounds.

### Component 2: Payload Relocation to Read-Only Data Section
- **Files to Modify**:
  - `kernel/embedded_chromium_elf.asm`
  - `kernel/embedded_desktop_elf.asm`
- **Modification**:
  - Change section header from `section .data` to `section .rodata`.
  - Maintain 16-byte alignment (`align 16`).
- **Rationale**:
  - Executable ELF image payloads are static, immutable binaries. Placing them in `.rodata` rather than `.data` positions them immediately after `.text` and well ahead of `.data` and `.bss` in the linker layout. This provides multi-megabyte physical separation from any stack or mutable global variables.

---

## 2. Expected Results
1. `g_embedded_chromium_elf` will be positioned within the `.rodata` section in `kernel.map` (around virtual address `0x360000`–`0x400000`), completely shielded from `.bss`.
2. The kernel boot stack will have 256 KB of dedicated contiguous memory.
3. During kernel boot and Chromium launch, all bytes of `g_embedded_chromium_elf` (including offset `0x7E40` representing `atoms_heap_init`) will remain completely uncorrupted.
4. When `chromium_browser_launch()` calls `elf_load_image_from_buffer()`, page 6 (`0x40006000`) and all subsequent pages will copy exact, pristine x86_64 machine code instructions.
5. In Ring 3, `PID=201` (`chromium_browser.elf`) will execute `atoms_heap_init` cleanly without `#PF` or page faults, proceed through libc runtime initialization (`call_init_array`), and invoke `chromium_browser_main()`.

---

## 3. Risk Assessment & Mitigations
- **Risk 1: Kernel binary size increase**:
  - Expanding `.bss` does NOT increase the size of `kernel.bin` on disk because `.bss` is uninitialized storage.
  - Relocating payloads from `.data` to `.rodata` does not alter payload size.
- **Risk 2: Memory footprint in physical RAM**:
  - Additional 240 KB for stack is negligible on target systems (2 GB in QEMU, 8 GB on target ASUS H81).
- **Risk 3: Linker section ordering**:
  - `kernel/linker.ld` already has `.rodata ALIGN(4096)` defined. Linking `.rodata` before `.data` is standard and already present in the linker script.

---

## 4. Rollback Plan
If regressions occur:
1. Revert `kernel/kernel_entry.asm` stack allocation to 16384 bytes.
2. Revert `kernel/embedded_chromium_elf.asm` and `kernel/embedded_desktop_elf.asm` to `section .data`.
3. Re-run `build.ps1` to restore the previous binary state.
