#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>

// Page Table Flags
#define PAGE_PRESENT       (1 << 0)
#define PAGE_WRITABLE      (1 << 1)
#define PAGE_USER          (1 << 2)
#define PAGE_WRITE_THROUGH (1 << 3)
#define PAGE_PAT_WC        PAGE_WRITE_THROUGH /* Reprogrammed PA1 in IA32_PAT MSR is Write-Combining */
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED      (1 << 5)
#define PAGE_DIRTY         (1 << 6)
#define PAGE_HUGE          (1 << 7)
#define PAGE_GLOBAL        (1 << 8)
#define PAGE_NX            (1ULL << 63)

#define PAGE_PHYS_ADDRESS_MASK  0x000FFFFFFFFFF000ULL

// Paging structure manipulation functions
uint64_t* vmm_get_pt_entry(void* pml4, uint64_t virt_addr, bool create_if_missing);
void vmm_flush_tlb(uint64_t virt_addr);
void* vmm_get_active_pml4(void);

#endif // PAGING_H
