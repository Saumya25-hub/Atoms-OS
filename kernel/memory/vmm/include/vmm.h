#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stdbool.h>

void vmm_init(void);

// Core Mapping
void vmm_map_page(void* pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags);
void vmm_unmap_page(void* pml4, uint64_t virt_addr);

// PMM + VMM Combined Operations (for convenience)
void* vmm_alloc_mapped_page(void* pml4, uint64_t virt_addr, uint32_t flags);
void vmm_free_mapped_page(void* pml4, uint64_t virt_addr);

// Address Space Management
void* vmm_create_address_space(void); 
void vmm_switch_address_space(void* pml4_phys_addr);

// Translation
uint64_t vmm_get_physical_address(void* pml4, uint64_t virt_addr);

#endif // VMM_H
