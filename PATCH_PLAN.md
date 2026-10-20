# ATOMS OS ARCHITECTURE PATCH PLAN — RESOLUTION OF BOUNDARY B14 (MOUNTROOT DEADLOCK)

**Target Subsystem**: ATOMS VMBSX Mount Engine (Hypervisor VirtIO Core & UFS2 Image Builder)  
**Input**: `FORENSIC_REPORT.md`  
**Protocol Phase**: TASK 2 — ARCHITECT TEAM  
**Date**: 2026-10-20  

---

## 1. What to Modify

### File 1: `kernel/core/hypervisor/src/hypervisor.c`
- **Function**: `atoms_hypervisor_try_inject_virtio_irq(vCPU *vcpu)`
- **Modification**:
  - Remove suppression check `if (vcpu->vm->virtio_irq_injected) return false;`.
  - Maintain injection logic to continually assert the level-triggered vector whenever `virtio_irq_pending` is true, guest `RFLAGS.IF == 1`, and `intr_state == 0`.
  - Ensure `VMCS_VM_ENTRY_INTR_INFO_FIELD` receives `0x80000000U | vector` on every VM-entry until the guest reads `VIRTIO_PCI_ISR` (port `0xC013`).

### File 2: `tools/make_minimal_ufs2.py`
- **Modification**:
  - Load the authentic 4096-byte FreeBSD 14.1 UFS2 superblock directly from `tools/freebsd_payload/freebsd_rootfs.ufs2` at offset 65536.
  - Scale filesystem geometry to 16MB (32768 sectors):
    - `fs_size = TOTAL_FRAGMENTS` (4096 frags = 16MB)
    - `fs_dsize = TOTAL_FRAGMENTS - DATA_START_FRAG` (3840 frags)
    - `fs_fpg = TOTAL_FRAGMENTS`
    - `fs_ipg = INODES_PER_GROUP` (2048)
  - Retain authentic Cylinder Group 0 header and inode table layout.
  - Regenerate `build/media.img`.

### File 3: Build Pipeline
- Rebuild `media.img`.
- Recompile `BOOTX64.EFI` via GCC/Clang with updated `media.img` embedded in `kernel_payload.S`.
- Rebuild `build/atoms_uefi_test.img` via `gpt_image_builder.exe`.

---

## 2. Why

1. Intel VMX architecture clears `VMCS_VM_ENTRY_INTR_INFO_FIELD` on every VM-exit. Suppressing injection while `virtio_irq_injected == true` drops the level-triggered interrupt when any intermediate exit occurs (such as serial COM1 output), causing GEOM to hang indefinitely in `biowait()`.
2. Manual struct packing in `make_minimal_ufs2.py` caused field displacement of `fs_bsize` and `fs_fsize`, preventing FreeBSD UFS from reading the filesystem geometry.

---

## 3. Expected Result

1. On Request 2 (READ sector 0x7FFF), the VirtIO IRQ vector 0x31 will be injected and serviced by FreeBSD's `vtpci_legacy_intr()`.
2. Port 0xC013 will be read, clearing `virtio_irq_pending`.
3. GEOM tasting will complete successfully.
4. FreeBSD `vfs_mountroot` will read sector 128 (UFS2 superblock at 64KB), mount the authentic root filesystem from `/dev/vtbd0`, and execute `/sbin/init` (Boundary B14 PASS).

---

## 4. Risk Analysis & Rollback Plan

- **Risk**: Very low. Interrupt injection occurs only when guest `RFLAGS.IF == 1` and `intr_state == 0`. When the IDT gate vectors, `RFLAGS.IF` is hardware-cleared to 0, preventing duplicate interrupts until execution returns or interrupts are re-enabled.
- **Rollback**: Revert `hypervisor.c` and `make_minimal_ufs2.py` to previous git commits if unexpected regressions occur.
