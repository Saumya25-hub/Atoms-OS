#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stdint.h>

void vmm_init(void);
void vmm_enable(void);

#define VMM_USER_MIN_ADDRESS 0x0000000001000000ULL
#define VMM_USER_MAX_ADDRESS 0x00007FFFFFFFFFFFULL
#define VMM_PAGE_SIZE 4096ULL

/* Page Flags */
#define VMM_FLAG_PRESENT  (1ULL << 0)
#define VMM_FLAG_WRITABLE (1ULL << 1)
#define VMM_FLAG_USER     (1ULL << 2)
#define VMM_FLAG_GLOBAL   (1ULL << 8)
#define VMM_FLAG_NX       (1ULL << 63)

typedef enum {
  VMM_ACCESS_READ = 1U << 0,
  VMM_ACCESS_WRITE = 1U << 1,
  VMM_ACCESS_EXECUTE = 1U << 2,
  VMM_ACCESS_USER = 1U << 3
} VMMAccess;

typedef struct {
  uint64_t virtual_address;
  uint64_t physical_address;
  uint64_t flags;
  bool present;
  bool writable;
  bool user;
  bool executable;
} VMMPageInfo;

// Core Mapping
void vmm_map_page(void *pml4, uint64_t phys_addr, uint64_t virt_addr, uint32_t flags);
void vmm_unmap_page(void *pml4, uint64_t virt_addr);
uint64_t vmm_translate(void *pml4, uint64_t virt_addr);
bool vmm_is_mapped(void *pml4, uint64_t virt_addr);

// PMM + VMM Combined Operations
void *vmm_alloc_mapped_page(void *pml4, uint64_t virt_addr, uint32_t flags);
void vmm_free_mapped_page(void *pml4, uint64_t virt_addr);

// Address Space Management
void *vmm_get_kernel_pml4(void);
void *vmm_get_active_pml4(void);
void *vmm_create_address_space(void);
bool vmm_destroy_address_space(void *pml4);
void vmm_switch_address_space(void *pml4_phys_addr);

// Translation and protection inspection
uint64_t vmm_get_physical_address(void *pml4, uint64_t virt_addr);
bool vmm_query_page(void *pml4, uint64_t virt_addr, VMMPageInfo *out);
bool vmm_validate_user_range(void *pml4, uint64_t address, uint64_t size, uint32_t access);
bool vmm_map_user_page(void *pml4, uint64_t virt_addr, uint32_t access);
bool vmm_map_guard_page(void *pml4, uint64_t virt_addr);
bool vmm_walk_and_verify(void *pml4, uint64_t virt_addr);
void vmm_dump_address_space(void *pml4, uint64_t start, uint64_t end);

void vmm_self_test(void);

#endif // VMM_H
