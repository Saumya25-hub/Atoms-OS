# ATOMS OS — Lightweight Native Micro-Hypervisor
## Phase 2: Second-Level Address Translation (EPT/NPT) & Guest Memory Virtualization

---

## 1. Executive Summary & Architecture

Phase 2 of the **ATOMS Micro-Hypervisor** implements hardware-assisted **Second-Level Address Translation (SLAT)** for guest virtual machines running on the native **BOS Kernel**.

```text
+-------------------------------------------------------------------------+
|                              ATOMS OS                                   |
|        (BWE Desktop / BCM Compositor / Native Ring 3 Applications)      |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                              BOS Kernel                                 |
|          (PMM, VMM, Scheduler, VFS/BOFS, AHME, DGL, Network)            |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                     ATOMS Native Micro-Hypervisor                       |
|  ├── CPU Virtualization (Phase 1: VMX / SVM Core)                       |
|  └── Second-Level Memory Virtualization (Phase 2):                      |
|       ├── Intel EPT (Extended Page Tables)                              |
|       ├── AMD NPT (Nested Page Tables)                                  |
|       ├── Guest Physical Memory Container (`guest_memory_t`)            |
|       ├── GPA ➔ HPA Translation & Boundary Enforcement                  |
|       ├── 4 KB Baseline & 2 MB Large Page Support                       |
|       ├── Read / Write / Execute (R/W/X) Permission Controls            |
|       ├── Central EPT/NPT Violation & Fault Interceptor                 |
|       └── INVEPT / Second-Level TLB Invalidation Engine                 |
+-------------------------------------------------------------------------+
                                    │
                                    ▼
+-------------------------------------------------------------------------+
|                  Future FreeBSD Guest Container (Phase 4)               |
|  [Guest Virtual Address] ➔ (Guest Page Tables) ➔ [Guest Physical Addr] |
|                                                         │               |
|                                                         ▼ (EPT / NPT)   |
|                                                [Host Physical Address]  |
+-------------------------------------------------------------------------+
```

---

## 2. Memory Model & Address Space Separation

The hypervisor enforces strict separation across four distinct address planes:

```text
[ Guest Virtual Address (GVA) ]
               │
               ▼ (Controlled entirely by Guest OS CR3 & Guest Page Tables)
[ Guest Physical Address (GPA) ]
               │
               ▼ (Controlled exclusively by Hypervisor EPT / NPT Tables)
[ Host Physical Address (HPA) ]
               │
               ▼
[ Host Physical RAM (Allocated & bounded by BOS Kernel PMM) ]
```

### Invariants:
1. **Zero Host RAM Corruption**: A guest operating system can never map, translate, or access physical RAM belonging to the BOS Kernel or other ATOMS processes.
2. **Strict GPA Bounds**: Every GPA is strictly verified against the VM's registered window: `[gpa_base, gpa_base + gpa_size)`. Accesses outside this boundary immediately fail translation or trigger an EPT violation / NPT fault.
3. **Deterministic Cleanup**: When a VM is destroyed, all second-level page table frames (PML4, PDPT, PD, PT) and the backing physical RAM frames are recursively freed and returned to the BOS PMM.

---

## 3. Intel EPT Implementation Details

- **Hierarchy**: 4-Level Page Walk:
  - Level 4: EPT PML4 (Page Map Level 4)
  - Level 3: EPT PDPT (Page Directory Pointer Table)
  - Level 2: EPT PD (Page Directory) — Supports 2MB large pages (bit 7)
  - Level 1: EPT PT (Page Table) — 4KB page entries
- **EPT Pointer (EPTP)**:
  - Formatted as `(pml4_phys & 0x000FFFFFFFFFF000ULL) | 0x06 (WB) | (3 << 3) (4-level) | (1 << 6) (A/D enable)`.
  - Stored in VMCS field `0x0000201A` (`VMCS_EPT_POINTER`).
- **EPT Entry Format**:
  - Bit 0: Read Access
  - Bit 1: Write Access
  - Bit 2: Execute Access
  - Bits 5:3: Memory Type (6 = WB)
  - Bit 6: Ignore PAT memory type
  - Bit 7: Page size (2MB in PDE)
  - Bits 51:12: Physical Base Address
- **INVEPT Invalidation**:
  - Implements `invept_execute(type, desc)`.
  - Supports Type 1 (Single-Context) and Type 2 (All-Context) second-level TLB flushes.
- **EPT Violation Intercept**:
  - Intercepts `VMX_EXIT_REASON_EPT_VIOLATION` (48).
  - Decodes exit qualification for data read, data write, and instruction fetch faults.
  - Safely stops the vCPU to ensure host isolation.

---

## 4. AMD NPT Implementation Details

- **Hierarchy**: 4-Level Nested Paging:
  - PML4 ➔ PDPT ➔ PD ➔ PT.
- **Nested CR3 (`n_cr3`)**:
  - Points to the 4KB-aligned physical base address of the NPT PML4 table.
  - Configured in the AMD VMCB control area (`n_cr3` field) alongside `np_enable = 1`.
- **NPT Entry Format**:
  - Bit 0: Present
  - Bit 1: Read/Write
  - Bit 2: User / Supervisor
  - Bit 7: Page size (2MB in PDE)
  - Bit 63: No-Execute (NX)
  - Bits 51:12: Physical Base Address
- **Nested Page Fault (NPF) Intercept**:
  - Intercepts `SVM_EXIT_NPF` (`0x0400`).
  - Decodes `exit_info1` (error code) and `exit_info2` (faulting GPA).
  - Safely stops the vCPU on unauthorized access.

---

## 5. Unified Guest Memory Management Core

The unified API abstracts memory virtualization across both hardware platforms:

```c
/* Guest Memory Descriptor */
typedef struct guest_memory {
    uint32_t            vm_id;
    HypervisorBackend   backend;
    uint64_t            gpa_base;
    uint64_t            gpa_size;
    uint64_t            page_count;
    void               *hva_backing;
    uint64_t            hpa_backing;
    void               *ept_pml4_virt;
    uint64_t            ept_pml4_phys;
    uint64_t            eptp;
    void               *npt_pml4_virt;
    uint64_t            npt_pml4_phys;
    uint64_t            n_cr3;
    bool                is_initialized;
} GuestMemory;
```

### Key Lifecycle & Translation Functions:
- `guest_memory_create(vm_id, backend, gpa_base, size)`: Allocates backing memory, creates EPT/NPT tables, and maps initial RAM.
- `guest_memory_destroy(mem)`: Recursively traverses and frees all table levels and backing memory.
- `guest_memory_map_page(mem, gpa, hpa, perms)`: Maps 4KB page with bounds checking.
- `guest_memory_map_2mb_page(mem, gpa, hpa, perms)`: Maps 2MB page with 2MB alignment validation.
- `guest_memory_unmap_page(mem, gpa)`: Clears entry and removes mapping.
- `guest_memory_set_permissions(mem, gpa, perms)`: Dynamically updates Read, Write, Execute flags.
- `guest_memory_translate_gpa(mem, gpa, &out_hpa)`: Resolves GPA to HPA via page table walk.
- `guest_memory_query_page(mem, gpa, &out_info)`: Returns full mapping status, page size, and permissions.
- `guest_memory_validate_gpa_range(mem, gpa, size, req_perms)`: Validates that an entire address range is valid and has requested permissions.

---

## 6. Synthetic Test Suite Results

The self-contained test suite in [`kernel/core/hypervisor/src/hypervisor.c`](file:///D:/Signatures_OS/kernel/core/hypervisor/src/hypervisor.c) executes during boot:

| Test # | Test Name | Description | Result |
|:---:|---|---|:---:|
| **01** | VM RAM Allocation | Allocate VM with 16 MB guest RAM container | **PASS** |
| **02** | EPT/NPT Root Creation | Allocate second-level PML4 root & compute EPTP/n_cr3 | **PASS** |
| **03** | Map First Page | Map GPA 0x0 ➔ HPA backing | **PASS** |
| **04** | Translate GPA | Translate GPA 0x0 ➔ HPA and verify exact match | **PASS** |
| **05** | Host Memory Access | Write 0xDEADBEEF to HVA backing and verify readback | **PASS** |
| **06** | Read Permission | Verify `GUEST_PERM_READ` flag set in entry | **PASS** |
| **07** | Write Permission | Verify `GUEST_PERM_WRITE` flag set in entry | **PASS** |
| **08** | Execute Permission | Verify `GUEST_PERM_EXEC` flag set in entry | **PASS** |
| **09** | Out-of-Bounds GPA | Verify rejection of GPA 32MB on 16MB VM | **PASS** |
| **10** | Unmapped GPA Query | Verify query fails for unmapped GPA 0x4000 | **PASS** |
| **11** | Permission Modification | Change permissions from RWX ➔ RX and verify bits | **PASS** |
| **12** | Unmap Page | Unmap GPA 0x0 and verify query returns unmapped | **PASS** |
| **13** | VM Destruction & Cleanup | Cleanly destroy VM and verify all tables reclaimed | **PASS** |
| **14** | Contiguous Multi-Page | Map and translate 8 contiguous 4KB pages | **PASS** |
| **15** | 2MB Large Page | Map 2MB page at GPA 0x200000 and verify query flags | **PASS** |
| **16** | TLB Invalidation | Execute `INVEPT` single-context and all-context flushes | **PASS** |
| **17** | EPT Violation Simulation | Simulate EPT write fault and verify security halt | **PASS** |
| **18** | NPT Violation Simulation | Simulate AMD NPF fault and verify security halt | **PASS** |
| **19** | Full Lifecycle with SLAT | Run complete VM lifecycle with EPT/NPT attached | **PASS** |
| **20** | BOS Memory Integrity | Verify host kernel heap and RAM uncorrupted after VM exit | **PASS** |

---

## 7. Resource Overhead & 4 GB RAM Assessment

- **EPT/NPT Paging Overhead**:
  - 16 MB Guest RAM: 1 PML4 (4KB) + 1 PDPT (4KB) + 1 PD (4KB) + 8 PTs (32KB) = **44 KB total table overhead**.
  - 512 MB FreeBSD Container: ~1.2 MB total table overhead.
- **Host RAM Budget on 4 GB Machine**:
  - BOS Kernel + Desktop + Drivers: ~150 MB.
  - Isolated FreeBSD Guest Partition: 512 MB – 1024 MB.
  - Remaining Host RAM for ATOMS Native Apps: ~2.8 GB – 3.3 GB.
- **Verdict**: Fully viable on 4 GB target hardware with zero host memory starvation.

---

## 8. Phase Roadmap & Subsystem Boundaries

```text
[Phase 1] ➔ VMX/SVM CPU Virtualization & Micro-Hypervisor Core (COMPLETED)
[Phase 2] ➔ Second-Level Address Translation (EPT/NPT) & Memory (COMPLETED)
    │
[Phase 3] ➔ VirtIO Block, Network, Console & Input Virtual Hardware (NEXT)
    │
[Phase 4] ➔ Lightweight FreeBSD Guest Kernel Bootloader & ELF Loader
    │
[Phase 5] ➔ FreeBSD Minimal Userspace Rootfs & IPC Bridge
    │
[Phase 6] ➔ Headless Chromium Engine & BCM Compositor Surface Sharing
```
