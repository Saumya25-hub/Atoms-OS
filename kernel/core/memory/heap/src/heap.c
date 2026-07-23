#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/crash_log.h"

#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align) - 1))

// Fixed initial heap region (e.g. at 256MB)
#define HEAP_START_VADDR 0x80000000ULL
#define KERNEL_HEAP_INITIAL_SIZE (64 * 1024 * 1024)

static uint64_t heap_current;
static uint64_t heap_end;

static heap_block_t* heap_head = NULL;
bool heap_trace_enabled = false;

// BMLE Telemetry Variables
uint64_t g_bmle_current_large_bytes = 0;
uint64_t g_bmle_peak_large_bytes = 0;
uint32_t g_bmle_total_large_allocations = 0;
uint32_t g_bmle_total_large_frees = 0;
uint32_t g_bmle_live_large_allocation_count = 0;
BMLE_AllocationRecord g_bmle_records[BMLE_MAX_RECORDS] = {0};

static inline uint32_t get_cpu_id(void);
static void validate_block_or_panic(heap_block_t* block, const char* caller);

void heap_audit_metadata_write(uint64_t target_addr, uint64_t old_val, uint64_t new_val, const char* field, const char* caller, uint64_t rip) {
    if (target_addr < HEAP_START_VADDR || target_addr >= (uint64_t)heap_end) return;
    bool is_magic = (field[0] == 'm');
    bool is_size  = (field[0] == 's');
    bool is_next  = (field[0] == 'n');
    bool is_prev  = (field[0] == 'p');

    bool illegal = false;
    if (is_magic && new_val != HEAP_MAGIC) illegal = true;
    if (is_size && new_val > (heap_end - HEAP_START_VADDR)) illegal = true;
    if (is_next && new_val != 0 && (new_val < HEAP_START_VADDR || new_val >= (uint64_t)heap_end || (new_val & 7) != 0)) illegal = true;
    if (is_prev && new_val != 0 && (new_val < HEAP_START_VADDR || new_val >= (uint64_t)heap_end || (new_val & 7) != 0)) illegal = true;

    if (illegal) {
        unsigned long flags;
        __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        display_print("\n=========================================\n");
        display_print(" [ILLEGAL HEAP METADATA WRITE DETECTED!]\n");
        display_print("=========================================\n");
        display_print("Field    : "); display_print(field); display_print("\n");
        display_print("Caller   : "); display_print(caller); display_print("\n");
        display_print("RIP      : "); display_print_hex(rip); display_print("\n");
        display_print("CPU ID   : "); display_print_dec(get_cpu_id()); display_print("\n");
        display_print("Target   : "); display_print_hex(target_addr); display_print("\n");
        display_print("Old Value: 0x"); display_print_hex(old_val); display_print("\n");
        display_print("New Value: 0x"); display_print_hex(new_val); display_print("\n");
        display_print("=========================================\n");
        while (1) { __asm__ volatile("hlt"); }
    }
}

void heap_check_external_write(uint64_t dst_addr, size_t len, const char* caller, uint64_t rip) {
    if (dst_addr < HEAP_START_VADDR || dst_addr >= (uint64_t)heap_end) return;
    
    uint64_t start = dst_addr & ~3ULL;
    for (uint64_t a = start; a < dst_addr + len && a + 4 <= (uint64_t)heap_end; a += 4) {
        uint32_t val = *(uint32_t*)a;
        if (val == HEAP_MAGIC) {
            heap_audit_metadata_write(a, val, 0xFF000000, "magic(EXTERNAL)", caller, rip);
        }
    }
}

#define WRITE_MAGIC(blk, val) do { \
    heap_audit_metadata_write((uint64_t)&((blk)->magic), (blk)->magic, (val), "magic", __func__, (uint64_t)__builtin_return_address(0)); \
    (blk)->magic = (val); \
} while(0)

#define WRITE_SIZE(blk, val) do { \
    heap_audit_metadata_write((uint64_t)&((blk)->size), (blk)->size, (val), "size", __func__, (uint64_t)__builtin_return_address(0)); \
    (blk)->size = (val); \
} while(0)

#define WRITE_NEXT(blk, val) do { \
    heap_audit_metadata_write((uint64_t)&((blk)->next), (uint64_t)(blk)->next, (uint64_t)(val), "next", __func__, (uint64_t)__builtin_return_address(0)); \
    (blk)->next = (val); \
} while(0)

#define WRITE_PREV(blk, val) do { \
    heap_audit_metadata_write((uint64_t)&((blk)->prev), (uint64_t)(blk)->prev, (uint64_t)(val), "prev", __func__, (uint64_t)__builtin_return_address(0)); \
    (blk)->prev = (val); \
} while(0)

void heap_init(void) {
    void* active_pml4 = vmm_get_active_pml4();

    size_t pages = ALIGN_UP(KERNEL_HEAP_INITIAL_SIZE, 4096) / 4096;

    // Request initial pages from VMM
    for (size_t i = 0; i < pages; i++) {
        uint64_t vaddr = HEAP_START_VADDR + (i * 4096);
        void* frame = vmm_alloc_mapped_page(active_pml4, vaddr, PAGE_WRITABLE | PAGE_USER);
        if (!frame) {
            display_print("[HEAP V1] PANIC: Failed to allocate initial heap pages!\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    heap_current = HEAP_START_VADDR;
    heap_end = HEAP_START_VADDR + (pages * 4096);

    // Initialize the first free block
    heap_head = (heap_block_t*)HEAP_START_VADDR;
    heap_head->front_canary = HEAP_CANARY_FRONT;
    heap_head->front_canary2 = HEAP_CANARY_FRONT;
    heap_head->req_size = 0;
    heap_head->alloc_rip = 0;
    WRITE_MAGIC(heap_head, HEAP_MAGIC);
    WRITE_SIZE(heap_head, (heap_end - HEAP_START_VADDR) - sizeof(heap_block_t));
    heap_head->is_free = true;
    WRITE_NEXT(heap_head, NULL);
    WRITE_PREV(heap_head, NULL);

    display_print("\n[HEAP V1] Init OK\n");
    display_print("Heap Start: "); display_print_hex(heap_current); display_print("\n");
    display_print("Heap End:   "); display_print_hex(heap_end); display_print("\n\n");
    
    crash_log_add("[BOOT] Heap V1 Ready");
}

static void split_block(heap_block_t* block, size_t size) {
    validate_block_or_panic(block, "split_block");
    if (block->size > size + sizeof(heap_block_t) + 8) {
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);
        new_block->front_canary = HEAP_CANARY_FRONT;
        new_block->front_canary2 = HEAP_CANARY_FRONT;
        new_block->req_size = 0;
        new_block->alloc_rip = 0;
        WRITE_MAGIC(new_block, HEAP_MAGIC);
        WRITE_SIZE(new_block, block->size - size - sizeof(heap_block_t));
        new_block->is_free = true;
        
        WRITE_NEXT(new_block, block->next);
        WRITE_PREV(new_block, block);
        
        if (new_block->next) {
            validate_block_or_panic(new_block->next, "split_block->next");
            WRITE_PREV(new_block->next, new_block);
        }
        
        WRITE_NEXT(block, new_block);
        WRITE_SIZE(block, size);
    }
}

static void coalesce_block(heap_block_t* block) {
    if (!block || !block->is_free) return;
    validate_block_or_panic(block, "coalesce_block");

    if (block->next && block->next->is_free && block->next->magic == HEAP_MAGIC) {
        validate_block_or_panic(block->next, "coalesce_block->next");
        WRITE_SIZE(block, block->size + sizeof(heap_block_t) + block->next->size);
        WRITE_NEXT(block, block->next->next);
        if (block->next) {
            validate_block_or_panic(block->next, "coalesce_block->next->next");
            WRITE_PREV(block->next, block);
        }
    }

    if (block->prev && block->prev->is_free && block->prev->magic == HEAP_MAGIC) {
        validate_block_or_panic(block->prev, "coalesce_block->prev");
        WRITE_SIZE(block->prev, block->prev->size + sizeof(heap_block_t) + block->size);
        WRITE_NEXT(block->prev, block->next);
        if (block->next) {
            validate_block_or_panic(block->next, "coalesce_block->prev->next");
            WRITE_PREV(block->next, block->prev);
        }
    }
}

static inline uint32_t get_cpu_id(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    return (ebx >> 24) & 0xFF;
}

static void verify_block_canaries(heap_block_t* block, const char* caller_func, uint64_t current_rip) {
    if (!block) return;
    uint64_t addr = (uint64_t)block;
    bool canonical = ((addr >> 47) == 0 || (addr >> 47) == 0x1FFFFULL);
    bool in_heap = (addr >= HEAP_START_VADDR && addr < heap_end);
    bool aligned = ((addr & 7) == 0);
    if (!canonical || !in_heap || !aligned) {
        unsigned long flags;
        __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        display_print("\n==================================================\n");
        display_print(" [HEAP CORRUPTION DETECTED]\n");
        display_print("==================================================\n");
        display_print("Allocation Address : 0x"); display_print_hex(addr); display_print("\n");
        display_print("Allocation Size    : 0\n");
        display_print("Allocation Caller  : 0x0\n");
        display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
        display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
        display_print("Corrupted Offset   : +0 (Invalid Block Pointer)\n");
        display_print("==================================================\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    uint64_t alloc_addr = addr + sizeof(heap_block_t);
    size_t alloc_size = block->req_size;
    uint64_t alloc_rip = block->alloc_rip;

    if (block->magic != HEAP_MAGIC) {
        unsigned long flags;
        __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        display_print("\n==================================================\n");
        display_print(" [HEAP CORRUPTION DETECTED]\n");
        display_print("==================================================\n");
        display_print("Allocation Address : 0x"); display_print_hex(alloc_addr); display_print("\n");
        display_print("Allocation Size    : "); display_print_dec(alloc_size); display_print("\n");
        display_print("Allocation Caller  : 0x"); display_print_hex(alloc_rip); display_print("\n");
        display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
        display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
        display_print("Corrupted Offset   : -64 (Header Magic)\n");
        display_print("==================================================\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    uint64_t exp_front = HEAP_CANARY_FRONT;
    for (int i = 0; i < 8; i++) {
        if (((uint8_t*)&block->front_canary)[i] != ((uint8_t*)&exp_front)[i]) {
            unsigned long flags;
            __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
            display_print("\n==================================================\n");
            display_print(" [HEAP CORRUPTION DETECTED]\n");
            display_print("==================================================\n");
            display_print("Allocation Address : 0x"); display_print_hex(alloc_addr); display_print("\n");
            display_print("Allocation Size    : "); display_print_dec(alloc_size); display_print("\n");
            display_print("Allocation Caller  : 0x"); display_print_hex(alloc_rip); display_print("\n");
            display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
            display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
            display_print("Corrupted Offset   : -"); display_print_dec(72 - i); display_print(" (Front Canary Byte "); display_print_dec(i); display_print(")\n");
            display_print("==================================================\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    for (int i = 0; i < 8; i++) {
        if (((uint8_t*)&block->front_canary2)[i] != ((uint8_t*)&exp_front)[i]) {
            unsigned long flags;
            __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
            display_print("\n==================================================\n");
            display_print(" [HEAP CORRUPTION DETECTED]\n");
            display_print("==================================================\n");
            display_print("Allocation Address : 0x"); display_print_hex(alloc_addr); display_print("\n");
            display_print("Allocation Size    : "); display_print_dec(alloc_size); display_print("\n");
            display_print("Allocation Caller  : 0x"); display_print_hex(alloc_rip); display_print("\n");
            display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
            display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
            display_print("Corrupted Offset   : -"); display_print_dec(8 - i); display_print(" (Front Canary 2 Byte "); display_print_dec(i); display_print(")\n");
            display_print("==================================================\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    }

    if ((block->next != NULL && (((uintptr_t)block->next < (uintptr_t)HEAP_START_VADDR) || ((uintptr_t)block->next >= (uintptr_t)heap_end) || (((uintptr_t)block->next & 7) != 0))) ||
        (block->prev != NULL && (((uintptr_t)block->prev < (uintptr_t)HEAP_START_VADDR) || ((uintptr_t)block->prev >= (uintptr_t)heap_end) || (((uintptr_t)block->prev & 7) != 0)))) {
        unsigned long flags;
        __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
        display_print("\n==================================================\n");
        display_print(" [HEAP CORRUPTION DETECTED]\n");
        display_print("==================================================\n");
        display_print("Allocation Address : 0x"); display_print_hex(alloc_addr); display_print("\n");
        display_print("Allocation Size    : "); display_print_dec(alloc_size); display_print("\n");
        display_print("Allocation Caller  : 0x"); display_print_hex(alloc_rip); display_print("\n");
        display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
        display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
        display_print("Corrupted Offset   : -24 (Next/Prev Pointer)\n");
        display_print("==================================================\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    if (!block->is_free && alloc_size > 0) {
        uint8_t* user_data = (uint8_t*)block + sizeof(heap_block_t);
        uint8_t* rear_ptr = user_data + alloc_size;
        uint64_t exp_rear = HEAP_CANARY_REAR;
        for (int i = 0; i < 8; i++) {
            if (rear_ptr[i] != ((uint8_t*)&exp_rear)[i]) {
                unsigned long flags;
                __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
                display_print("\n==================================================\n");
                display_print(" [HEAP CORRUPTION DETECTED]\n");
                display_print("==================================================\n");
                display_print("Allocation Address : 0x"); display_print_hex(alloc_addr); display_print("\n");
                display_print("Allocation Size    : "); display_print_dec(alloc_size); display_print("\n");
                display_print("Allocation Caller  : 0x"); display_print_hex(alloc_rip); display_print("\n");
                display_print("Current Caller RIP : 0x"); display_print_hex(current_rip); display_print("\n");
                display_print("CPU ID             : "); display_print_dec(get_cpu_id()); display_print("\n");
                display_print("Corrupted Offset   : +"); display_print_dec(alloc_size + i); display_print(" (Rear Canary Byte "); display_print_dec(i); display_print(")\n");
                display_print("==================================================\n");
                while (1) { __asm__ volatile("hlt"); }
            }
        }
    }
}

static void validate_block_or_panic(heap_block_t* block, const char* caller) {
    verify_block_canaries(block, caller, (uint64_t)__builtin_return_address(0));
}

static volatile uint32_t heap_spinlock = 0;
static volatile uint32_t heap_spinlock_owner = 0xFFFFFFFF;

static inline unsigned long heap_lock(void) {
    unsigned long flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    
    uint32_t cpu = get_cpu_id();
    if (heap_spinlock && heap_spinlock_owner == cpu) {
        display_print("\n[HEAP PANIC] RECURSIVE LOCK / DEADLOCK DETECTED ON CORE ");
        display_print_dec(cpu);
        display_print("\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    uint64_t cycles = 0;
    while (__sync_lock_test_and_set(&heap_spinlock, 1)) {
        while (heap_spinlock) {
            __asm__ volatile("pause" : : : "memory");
            if (++cycles > 500000000ULL) {
                display_print("\n[HEAP PANIC] DEADLOCK TIMEOUT ON CORE ");
                display_print_dec(cpu);
                display_print("! Current Owner: ");
                display_print_dec(heap_spinlock_owner);
                display_print("\n");
                while (1) { __asm__ volatile("hlt"); }
            }
        }
    }
    heap_spinlock_owner = cpu;
    return flags;
}

static inline void heap_unlock(unsigned long flags) {
    heap_spinlock_owner = 0xFFFFFFFF;
    __sync_lock_release(&heap_spinlock);
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory");
}

void* kmalloc_tracked(size_t size, uint64_t alloc_rip) {
    if (size == 0) return NULL;

    unsigned long flags = heap_lock();

    if (heap_trace_enabled) {
        display_print("[CPU "); display_print_dec(get_cpu_id()); display_print("] enter kmalloc\n");
    }

    size_t needed_payload = ((size + 8) + 7) & ~7ULL;

    heap_block_t* current = heap_head;
    while (current) {
        validate_block_or_panic(current, "kmalloc");

        if (current->is_free && current->size >= needed_payload) {
            split_block(current, needed_payload);
            current->is_free = false;
            current->req_size = size;
            current->alloc_rip = alloc_rip;
            current->front_canary = HEAP_CANARY_FRONT;
            current->front_canary2 = HEAP_CANARY_FRONT;
            
            uint8_t* user_data = (uint8_t*)current + sizeof(heap_block_t);
            uint64_t rear_val = HEAP_CANARY_REAR;
            for (int i = 0; i < 8; i++) {
                user_data[size + i] = ((uint8_t*)&rear_val)[i];
            }
            
            if (heap_trace_enabled) {
                display_print("kmalloc("); display_print_dec(size);
                display_print(") -> Block "); display_print_hex((uint64_t)current); display_print("\n");
            }
            
            // BMLE Accounting
            if (size >= BMLE_LARGE_ALLOCATION_THRESHOLD) {
                g_bmle_current_large_bytes += size;
                if (g_bmle_current_large_bytes > g_bmle_peak_large_bytes) {
                    g_bmle_peak_large_bytes = g_bmle_current_large_bytes;
                }
                g_bmle_total_large_allocations++;
                g_bmle_live_large_allocation_count++;
                
                for (int r = 0; r < BMLE_MAX_RECORDS; r++) {
                    if (!g_bmle_records[r].active) {
                        g_bmle_records[r].active = true;
                        g_bmle_records[r].ptr = user_data;
                        g_bmle_records[r].size = size;
                        g_bmle_records[r].caller_rip = alloc_rip;
                        break;
                    }
                }
            }
            
            heap_unlock(flags);
            return (void*)user_data;
        }
        current = current->next;
    }

    heap_unlock(flags);
    bmle_dump_telemetry();
    display_print("[HEAP V1] PANIC: Out of Memory! Failed to allocate ");
    display_print_dec(size);
    display_print(" bytes.\n");
    while (1) { __asm__ volatile("hlt"); }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    unsigned long flags = heap_lock();

    if (heap_trace_enabled) {
        display_print("[CPU "); display_print_dec(get_cpu_id()); display_print("] enter kfree\n");
    }

    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    validate_block_or_panic(block, "kfree");

    if (block->is_free) {
        heap_unlock(flags);
        display_print("[HEAP V1] PANIC: Double free detected!\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    // BMLE Accounting
    if (block->req_size >= BMLE_LARGE_ALLOCATION_THRESHOLD) {
        g_bmle_current_large_bytes -= block->req_size;
        g_bmle_total_large_frees++;
        g_bmle_live_large_allocation_count--;
        
        uint8_t* udata = (uint8_t*)block + sizeof(heap_block_t);
        for (int r = 0; r < BMLE_MAX_RECORDS; r++) {
            if (g_bmle_records[r].active && g_bmle_records[r].ptr == (void*)udata) {
                g_bmle_records[r].active = false;
                break;
            }
        }
    }

    block->is_free = true;
    block->req_size = 0;
    block->alloc_rip = 0;
    coalesce_block(block);
    heap_unlock(flags);
}

void* kcalloc_tracked(size_t num, size_t size, uint64_t alloc_rip) {
    void* ptr = kmalloc_tracked(num * size, alloc_rip);
    if (ptr) {
        uint8_t* p = (uint8_t*)ptr;
        for (size_t i = 0; i < num * size; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void* krealloc_tracked(void* ptr, size_t new_size, uint64_t alloc_rip) {
    if (!ptr) return kmalloc_tracked(new_size, alloc_rip);
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    unsigned long flags = heap_lock();
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    validate_block_or_panic(block, "krealloc");
    size_t old_size = block->req_size;
    heap_unlock(flags);

    if (block->size >= (((new_size + 8) + 7) & ~7ULL)) {
        flags = heap_lock();
        validate_block_or_panic(block, "krealloc_inplace");
        block->req_size = new_size;
        block->alloc_rip = alloc_rip;
        uint8_t* user_data = (uint8_t*)block + sizeof(heap_block_t);
        uint64_t rear_val = HEAP_CANARY_REAR;
        for (int i = 0; i < 8; i++) {
            user_data[new_size + i] = ((uint8_t*)&rear_val)[i];
        }
        heap_unlock(flags);
        return ptr;
    }

    void* new_ptr = kmalloc_tracked(new_size, alloc_rip);
    if (new_ptr) {
        uint8_t* src = (uint8_t*)ptr;
        uint8_t* dst = (uint8_t*)new_ptr;
        for (size_t i = 0; i < old_size; i++) {
            dst[i] = src[i];
        }
        kfree(ptr);
    }
    return new_ptr;
}

#undef kmalloc
#undef kcalloc
#undef krealloc

void* kmalloc(size_t size) {
    return kmalloc_tracked(size, (uint64_t)__builtin_return_address(0));
}

void bmle_dump_telemetry(void) {
    display_print("\n=== BMLE MEMORY LIFECYCLE REPORT ===\n");
    display_print("LargeCurrentBytes="); display_print_dec((uint32_t)g_bmle_current_large_bytes); display_print("\n");
    display_print("LargePeakBytes="); display_print_dec((uint32_t)g_bmle_peak_large_bytes); display_print("\n");
    display_print("LargeAllocations="); display_print_dec(g_bmle_total_large_allocations); display_print("\n");
    display_print("LargeFrees="); display_print_dec(g_bmle_total_large_frees); display_print("\n");
    display_print("LiveLargeAllocations="); display_print_dec(g_bmle_live_large_allocation_count); display_print("\n");
    display_print("--- LIVE RECORDS ---\n");
    for (int r = 0; r < BMLE_MAX_RECORDS; r++) {
        if (g_bmle_records[r].active) {
            display_print(" ["); display_print_dec(r); display_print("] Ptr: 0x"); display_print_hex((uint64_t)g_bmle_records[r].ptr);
            display_print(" Size: "); display_print_dec(g_bmle_records[r].size);
            display_print(" RIP: 0x"); display_print_hex(g_bmle_records[r].caller_rip); display_print("\n");
        }
    }
    display_print("====================================\n\n");
}
void* kcalloc(size_t num, size_t size) {
    return kcalloc_tracked(num, size, (uint64_t)__builtin_return_address(0));
}
void* krealloc(void* ptr, size_t new_size) {
    return krealloc_tracked(ptr, new_size, (uint64_t)__builtin_return_address(0));
}

void heap_get_stats(HeapStats* stats) {
    if (!stats) return;

    unsigned long flags = heap_lock();

    if (heap_trace_enabled) {
        display_print("[CPU "); display_print_dec(get_cpu_id()); display_print("] enter heap_get_stats\n");
    }

    stats->total_size = (uint32_t)(heap_end - HEAP_START_VADDR);
    stats->used_size = 0;
    stats->free_size = 0;
    stats->block_count = 0;
    stats->largest_free = 0;

    heap_block_t* current = heap_head;
    while (current) {
        validate_block_or_panic(current, "heap_get_stats");
        stats->block_count++;
        if (current->is_free) {
            stats->free_size += current->size;
            if (current->size > stats->largest_free) {
                stats->largest_free = current->size;
            }
        } else {
            stats->used_size += current->size;
        }
        current = current->next;
    }

    heap_unlock(flags);
}

void heap_dump_blocks(void) {
    unsigned long flags = heap_lock();
    display_print("\n--- Kernel Heap Dump ---\n");
    heap_block_t* current = heap_head;
    int block_num = 0;
    while (current) {
        validate_block_or_panic(current, "heap_dump_blocks");
        display_print("Block "); display_print_dec(block_num++); display_print("\n");
        display_print("Address : "); display_print_hex((uint64_t)current); display_print("\n");
        display_print("Size    : "); display_print_dec(current->size); display_print(" bytes\n");
        display_print("State   : ");
        if (current->is_free) display_print("FREE\n\n");
        else display_print("USED\n\n");
        current = current->next;
    }
    display_print("------------------------\n");
    heap_unlock(flags);
}

void heap_stress_test(void) {
    display_print("\n--- Heap V1 Stress Test ---\n");
    HeapStats initial_stats;
    heap_get_stats(&initial_stats);
    
    display_print("Initial Free: "); display_print_dec(initial_stats.free_size); display_print("\n");
    
    void* ptrs[500];
    int alloc_count = 0;
    
    // Allocate
    for (int i = 0; i < 500; i++) {
        // Pseudo-random sizes between 16 and 512
        size_t size = 16 + ((i * 37) % 496);
        ptrs[i] = kmalloc(size);
        if (ptrs[i]) alloc_count++;
    }
    
    display_print("Allocated 500 blocks. Success: "); display_print_dec(alloc_count); display_print("\n");
    
    // Free
    for (int i = 0; i < 500; i++) {
        if (ptrs[i]) kfree(ptrs[i]);
    }
    
    HeapStats final_stats;
    heap_get_stats(&final_stats);
    
    display_print("Final Free:   "); display_print_dec(final_stats.free_size); display_print("\n");
    
    if (initial_stats.free_size == final_stats.free_size) {
        display_print("PASS: Coalescing OK. No Leaks detected.\n");
    } else {
        display_print("FAIL: Memory Leak detected!\n");
    }
    display_print("---------------------------\n");
}

void heap_validate(void) {
    unsigned long flags = heap_lock();
    display_print("\n--- Heap V1 Validation ---\n");
    heap_block_t* current = heap_head;
    int block_num = 0;
    bool any_fail = false;
    
    while (current) {
        validate_block_or_panic(current, "heap_validate");
        display_print("Block "); display_print_dec(block_num++); display_print(" ");
        
        if (current->magic != HEAP_MAGIC) {
            display_print("FAIL (Bad Magic: 0x");
            display_print_hex(current->magic);
            display_print(")\n");
            any_fail = true;
            break;
        } else {
            // Check prev link
            if (current->next && current->next->prev != current) {
                display_print("FAIL (Bad Next->Prev link)\n");
                any_fail = true;
                break;
            }
            if (current->prev && current->prev->next != current) {
                display_print("FAIL (Bad Prev->Next link)\n");
                any_fail = true;
                break;
            }
            display_print("PASS\n");
        }
        
        current = current->next;
    }
    
    if (!any_fail) {
        display_print("Validation SUCCESS. Heap intact.\n");
    } else {
        display_print("Validation FAILED.\n");
    }
    display_print("--------------------------\n");
    heap_unlock(flags);
}

void heap_walk(void) {
    unsigned long flags = heap_lock();
    display_print("\n--- Heap Walk ---\n");
    heap_block_t* current = heap_head;
    int block_num = 0;
    while (current) {
        validate_block_or_panic(current, "heap_walk");
        display_print("Block "); display_print_dec(block_num++); display_print("\n");
        display_print("Addr    : "); display_print_hex((uint64_t)current); display_print("\n");
        display_print("Size    : "); display_print_dec(current->size); display_print("\n");
        display_print("State   : ");
        if (current->is_free) display_print("FREE\n");
        else display_print("USED\n");
        display_print("Prev    : "); display_print_hex((uint64_t)current->prev); display_print("\n");
        display_print("Current : "); display_print_hex((uint64_t)current); display_print("\n");
        display_print("Next    : "); display_print_hex((uint64_t)current->next); display_print("\n\n");
        
        current = current->next;
    }
    display_print("-----------------\n");
    heap_unlock(flags);
}

void heap_trace_toggle(void) {
    heap_trace_enabled = !heap_trace_enabled;
    if (heap_trace_enabled) {
        display_print("Heap Trace ENABLED\n");
    } else {
        display_print("Heap Trace DISABLED\n");
    }
}

void* kmalloc_aligned_tracked(size_t size, size_t alignment, uint64_t alloc_rip) {
    if (size == 0) return NULL;
    if (alignment < sizeof(void*) || (alignment & (alignment - 1)) != 0) {
        alignment = 16;
    }

    size_t total_size = size + alignment + sizeof(void*);
    void* raw_ptr = kmalloc_tracked(total_size, alloc_rip);
    if (!raw_ptr) return NULL;

    uintptr_t raw_addr = (uintptr_t)raw_ptr + sizeof(void*);
    uintptr_t aligned_addr = (raw_addr + (alignment - 1)) & ~((uintptr_t)(alignment - 1));

    ((void**)aligned_addr)[-1] = raw_ptr;
    return (void*)aligned_addr;
}

void kfree_aligned(void* ptr) {
    if (!ptr) return;
    void* raw_ptr = ((void**)ptr)[-1];
    kfree(raw_ptr);
}
