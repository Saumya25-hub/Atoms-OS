# ATOMS OS — CR3 FORENSIC REPORT (`CR3 = 0x11087000`)

## 1. CR3 Value Analysis

| Register | Value | Target Identity |
| :--- | :--- | :--- |
| **`CR3`** | `0x0000000011087000` | Physical address of the active task's PML4 page table |
| **`RSP`** | `0x0000000011086E38` | Stack pointer inside kernel memory space |
| **Delta** | `CR3 - RSP = 0x1C8` (456 bytes) | Spatial proximity between task stack and task PML4 |

---

## 2. Memory Topology

In ATOMS OS, physical memory pages are allocated sequentially by `pmm_alloc_page()` during process creation:
1. `task->stack` is allocated from physical frames (e.g., `0x11086000`).
2. `vmm_create_address_space()` allocates the next physical page frame for the PML4 directory: `0x11087000`.
3. Therefore:
   - Stack Range: `0x11086000` — `0x11086FFF`
   - Page Directory Base: `0x11087000`

---

## 3. CR3 Switching Verification

During context switches:
- `vmm_switch_address_space(new_task->pml4)` updates CR3:
  ```c
  void vmm_switch_address_space(void *pml4) {
      if (!pml4) return;
      uint64_t phys = (uint64_t)pml4;
      __asm__ volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
  }
  ```
- CR3 at the moment of the crash was correctly holding the PML4 pointer of the current user process (`desktop_shell` / `calc`).
- The fault was NOT a Page Fault (`#PF`), nor a CR3 corruption: CR3 was completely valid and mapped kernel identity pages with `PAGE_PRESENT | PAGE_WRITABLE`.
