#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/*
 * BOS Rule 18 (Mandatory):
 * The Kernel Heap NEVER owns memory.
 * The Heap ONLY manages allocations.
 * Only the VMM may grow or shrink the heap.
 * The Heap MUST NEVER call the PMM directly.
 */

#define HEAP_MAGIC 0x123890AB
#define HEAP_CANARY_FRONT 0xCAFEBABE8BADF00DULL
#define HEAP_CANARY_REAR 0xDEADBEEFDEADBEEFULL

typedef struct heap_block {
  uint64_t front_canary; // +0  (8 bytes) - Front red zone canary
  uint32_t magic;        // +8  (4 bytes) - Header magic
  uint32_t flags;        // +12 (4 bytes) - Flags/padding
  size_t size;     // +16 (8 bytes) - Total usable memory capacity in this block
  size_t req_size; // +24 (8 bytes) - Exact user requested allocation size (0 if
                   // free)
  uint64_t alloc_rip; // +32 (8 bytes) - RIP of caller who allocated this block
                      // (0 if free)
  bool is_free;       // +40 (1 byte)
  uint8_t pad[7];     // +41 (7 bytes) - Alignment padding
  struct heap_block *next; // +48 (8 bytes)
  struct heap_block *prev; // +56 (8 bytes)
  uint64_t front_canary2;  // +64 (8 bytes) - Second front red zone canary right
                           // before user data
} heap_block_t;

typedef struct HeapStats {
  uint32_t total_size;
  uint32_t used_size;
  uint32_t free_size;
  uint32_t block_count;
  uint32_t largest_free;
} HeapStats;

void heap_init(void);
void *heap_alloc_raw(size_t size, uint64_t alloc_rip);
void *heap_realloc_raw(void *ptr, size_t new_size, uint64_t alloc_rip);
void heap_free_raw(void *ptr);
bool heap_expand_raw(size_t minimum_bytes);
void *kmalloc_tracked(size_t size, uint64_t alloc_rip);
void *kmalloc_aligned_tracked(size_t size, size_t alignment,
                              uint64_t alloc_rip);
void *kcalloc_tracked(size_t num, size_t size, uint64_t alloc_rip);
void *krealloc_tracked(void *ptr, size_t new_size, uint64_t alloc_rip);
void kfree(void *ptr);
void kfree_aligned(void *ptr);

#define kmalloc(size)                                                          \
  kmalloc_tracked((size), (uint64_t)__builtin_return_address(0))
#define kmalloc_aligned(size, align)                                           \
  kmalloc_aligned_tracked((size), (align),                                     \
                          (uint64_t)__builtin_return_address(0))
#define kcalloc(num, size)                                                     \
  kcalloc_tracked((num), (size), (uint64_t)__builtin_return_address(0))
#define krealloc(ptr, new_size)                                                \
  krealloc_tracked((ptr), (new_size), (uint64_t)__builtin_return_address(0))

void heap_get_stats(HeapStats *stats);
void heap_dump_blocks(void);
void heap_stress_test(void);
void heap_stage_a_stress_test(void);
void heap_update_telemetry(const char *status_str);
void heap_validate(void);
void heap_walk(void);
void heap_trace_toggle(void);
void heap_audit_metadata_write(uint64_t target_addr, uint64_t old_val,
                               uint64_t new_val, const char *field,
                               const char *caller, uint64_t rip);
void heap_check_external_write(uint64_t dst_addr, size_t len,
                               const char *caller, uint64_t rip);

extern volatile uint64_t g_heap_alloc_count;
extern volatile uint64_t g_heap_free_count;
extern uint64_t g_heap_corruption_count;
extern uint64_t g_heap_last_alloc;
extern uint64_t g_heap_last_caller_rip;

// ==========================================
// BOS Memory Lifecycle Engine (BMLE) - Phase 1 & 2
// ==========================================
#define BMLE_LARGE_ALLOCATION_THRESHOLD (1024 * 1024) // 1 MB
#define BMLE_MAX_RECORDS 32

typedef struct {
  void *ptr;
  size_t size;
  uint64_t caller_rip;
  bool active;
} BMLE_AllocationRecord;

extern uint64_t g_bmle_current_large_bytes;
extern uint64_t g_bmle_peak_large_bytes;
extern uint32_t g_bmle_total_large_allocations;
extern uint32_t g_bmle_total_large_frees;
extern uint32_t g_bmle_live_large_allocation_count;
extern BMLE_AllocationRecord g_bmle_records[BMLE_MAX_RECORDS];

void bmle_dump_telemetry(void);

#endif // HEAP_H
