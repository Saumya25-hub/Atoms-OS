# ARCHITECTURE PLAN — ATOMS OS SYSCALL SECURITY FORENSIC DEBUG & RECOVERY

**Document ID**: `PATCH_PLAN_20260903_SYSCALL_SECURITY`  
**Architect**: Architecture & Security Team  
**Input Document**: `FORENSIC_REPORT.md`  
**Physical Target**: ASUS B750M-K (Intel Core i3-14100F, LGA1700 Architecture)  

---

## 1. What to Create & Modify

### A. New Debug Subsystem Files (Isolated Debug Mode Only)
1. **`kernel/debug/syscall_security_debug.h`**:
   - Structured telemetry state: discovered syscalls, pointer syscalls, current test info, memory validation parameters, fault monitor counters (`#PF`, CPL0, CPL3, panics), live heartbeat.
   - Prototypes for `syscall_security_debug_init()`, `syscall_security_debug_render()`, and `syscall_security_debug_run()`.
2. **`kernel/debug/syscall_security_debug.c`**:
   - Full-screen ABDE forensic dashboard titled `ATOMS SYSCALL SECURITY FORENSIC`.
   - Ring 3 controlled test engine executing Test A through Test J.
   - Deep structured serial logging (`[SYSCALL-SEC] ...`).
   - Non-blocking continuous heartbeat spinner (`| / - \`).

### B. Kernel Debug Mode Switch
- **`kernel/kernel.c`**:
  - Add `#define ATOMS_DEBUG_MODE_SYSCALL_SECURITY 5`
  - Switch `ATOMS_ACTIVE_DEBUG_MODE` to `ATOMS_DEBUG_MODE_SYSCALL_SECURITY` for this isolated debug cycle.
  - Call `syscall_security_debug_run(boot_info)` in the debug mode dispatch block.

### C. Syscall Validation & Safe User Copy Subsystem (Phase 6 / 8 Fix)
- **`kernel/core/syscall/src/validation.c`**:
  - Enhance `syscall_validate_user_ptr()` to query the active task's `pml4` via `vmm_validate_user_range()`.
  - Validate `PAGE_PRESENT`, `PAGE_USER`, and `PAGE_WRITABLE` (for output buffers) across every page in `[ptr, ptr + size)`.
  - Enhance `syscall_validate_user_string()` to inspect page mappings before reading characters, ensuring null-termination search never probes unmapped pages.

---

## 2. Why This Architecture

1. **Phase Isolation**: Stable desktop, USB HID keyboard/mouse, VMM lifecycle, PMM, scheduler, and networking subsystems remain completely untouched and frozen.
2. **Native VMM Integration**: ATOMS OS already possesses a battle-tested page-table validator (`vmm_validate_user_range`) that inspects PML4/PDPT/PD/PT entries, canonical addresses, and permissions. Wiring the syscall boundary to this existing engine is architecturally consistent and avoids foreign code patterns.
3. **Multi-Stage Verification**:
   - Step 1: Prove the pre-fix vulnerability under controlled telemetry.
   - Step 2: Verify the surgical fix in QEMU.
   - Step 3: Verify and stress-test on physical bare-metal ASUS B750M-K hardware.

---

## 3. Test Cases (Phase 2 Matrix)

- **Test A (Valid Pointer)**: Normal user buffer. Expected: `SYSCALL_OK`, kernel alive.
- **Test B (NULL Pointer)**: Address `0x0`. Expected: `SYSCALL_BAD_ADDRESS`, no fault.
- **Test C (Unmapped User Pointer)**: Unmapped address in `[0x40000000, 0x80000000)`. Expected: `SYSCALL_BAD_ADDRESS`, zero CPL0 `#PF`, no panic.
- **Test D (Read-Only User Page)**: Read-only user page passed to output syscall. Expected: `SYSCALL_BAD_ADDRESS`, zero CPL0 `#PF`.
- **Test E (Page Boundary Continuity)**: First page mapped, second page unmapped. Expected: `SYSCALL_BAD_ADDRESS`.
- **Test F (Huge Size)**: Size exceeding user space or wrapping. Expected: `SYSCALL_BAD_ADDRESS`.
- **Test G (Address Overflow)**: `ptr + size` integer wrap. Expected: `SYSCALL_BAD_ADDRESS`.
- **Test H (Kernel-Space Address)**: Address `0xC0000000+` or `0xFFFF...`. Expected: `SYSCALL_BAD_ADDRESS`.
- **Test I (Non-Canonical Address)**: Invalid x86_64 address bits. Expected: `SYSCALL_BAD_ADDRESS`.
- **Test J (Invalid String)**: Unterminated string touching an unmapped boundary. Expected: `SYSCALL_BAD_ADDRESS`, validator does not fault.

---

## 4. Expected Results

1. All 10 controlled test cases yield `PASS`.
2. Fault monitor records `CPL0 Faults = 0`, `Kernel Panic = 0`.
3. Heartbeat spinner continues rotating continuously with zero freezes.
4. Clean build with zero warnings or errors.

---

## 5. Risk & Rollback Plan

- **Risk**: Very low. Isolated to debug mode and syscall pointer validation.
- **Rollback**: Set `ATOMS_ACTIVE_DEBUG_MODE` back to `ATOMS_DEBUG_MODE_KEYBOARD_LED` or `ATOMS_DEBUG_MODE_NONE`. Revert modified files via git.
