#define BOS_DEBUG 1

#include "kernel/core/loader/elf/include/elf.h"
#include "kernel/core/loader/elf/include/elf_loader.h"
#include "kernel/drivers/display/display.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

void elf_print_program_headers(const Elf64_Phdr* phdrs, uint16_t phnum) {
    if (!phdrs || phnum == 0) {
        return;
    }

#ifdef BOS_DEBUG
    display_print("\n==============================\n");
    display_print("      PROGRAM HEADERS\n");
    display_print("==============================\n\n");
#endif
    for (uint16_t i = 0; i < phnum; i++) {
        const Elf64_Phdr* phdr = &phdrs[i];
        
        // We only care about PT_LOAD segments for basic execution mapping
        if (phdr->p_type == PT_LOAD) {
#ifdef BOS_DEBUG
            display_print("PT_LOAD Segment [");
            display_print_dec(i);
            display_print("]\n");

            display_print("Offset         : 0x");
            display_print_hex(phdr->p_offset);
            display_print("\n");

            display_print("Virtual Address: 0x");
            display_print_hex(phdr->p_vaddr);
            display_print("\n");

            display_print("File Size      : 0x");
            display_print_hex(phdr->p_filesz);
            display_print("\n");

            display_print("Memory Size    : 0x");
            display_print_hex(phdr->p_memsz);
            display_print("\n");

            display_print("Flags          : ");
            if (phdr->p_flags & PF_R) display_print("R ");
            if (phdr->p_flags & PF_W) display_print("W ");
            if (phdr->p_flags & PF_X) display_print("X ");
            display_print("\n\n");
#endif
        }
    }
}

#define USER_VIRTUAL_BASE 0x1000000

bool elf_segment_planner(const Elf64_Phdr* phdrs, uint16_t phnum) {
    if (!phdrs || phnum == 0) {
        display_print("[ELF] Error: Rule 222 - Program Header Count == 0\n");
        return false;
    }

#ifdef BOS_DEBUG
    display_print("\n[ELF] Running Segment Planner (Sprint 4A)...\n");
#endif

    for (uint16_t i = 0; i < phnum; i++) {
        const Elf64_Phdr* phdr = &phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            // Rule 226: Reject p_memsz < p_filesz
            if (phdr->p_memsz < phdr->p_filesz) {
                display_print("[ELF] Error: Rule 226 - p_memsz < p_filesz\n");
                return false;
            }

            // Rule 227: Reject p_align == 0
            if (phdr->p_align == 0) {
                display_print("[ELF] Error: Rule 227 - p_align == 0\n");
                return false;
            }

            // Rule 228: Virtual Address < User Base -> Reject
            if (phdr->p_vaddr < USER_VIRTUAL_BASE) {
                display_print("[ELF] Error: Rule 228 - Mapping below user base (0x400000)\n");
                return false;
            }

            // Rule 229: Check for overlapping segments
            for (uint16_t j = i + 1; j < phnum; j++) {
                const Elf64_Phdr* other = &phdrs[j];
                if (other->p_type == PT_LOAD) {
                    uint64_t start1 = phdr->p_vaddr;
                    uint64_t end1 = start1 + phdr->p_memsz;
                    uint64_t start2 = other->p_vaddr;
                    uint64_t end2 = start2 + other->p_memsz;

                    if (start1 < end2 && start2 < end1) {
                        display_print("[ELF] Error: Rule 229 - Overlapping PT_LOAD segments\n");
                        return false;
                    }
                }
            }

            // Calculate required pages (assume 4096 byte pages)
            uint64_t start_page = phdr->p_vaddr & ~0xFFFULL;
            uint64_t end_page = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFFULL;
            uint64_t pages_needed = (end_page - start_page) / 4096;

#ifdef BOS_DEBUG
            display_print("  -> Planner: Segment [");
            display_print_dec(i);
            display_print("] requires ");
            display_print_dec(pages_needed);
            display_print(" pages at Virtual 0x");
            display_print_hex(phdr->p_vaddr);
            display_print("\n");
#endif
        }
    }
    
#ifdef BOS_DEBUG
    display_print("[ELF] Planner Validation PASS\n");
#endif
    return true;
}

bool elf_load_segment(void* pml4, int fd, const Elf64_Phdr* phdr, uint16_t index) {
    if (phdr->p_type != PT_LOAD) return true;

    display_print("\n[ELF] Segment ");
    display_print_dec(index);
    display_print("\n");

    uint64_t start_page = phdr->p_vaddr & ~0xFFFULL;
    uint64_t end_page   = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFFULL;
    uint64_t pages      = (end_page - start_page) / 4096;

    display_print("Pages: "); display_print_dec(pages); display_print("\n");

    // ===========================================================================
    // SPRINT 4B+4C: Allocate physical frames and map them into the process PML4.
    // vmm_alloc_mapped_page internally uses the KERNEL PML4 for page-table walks.
    // No CR3 switch needed here.
    // ===========================================================================
    uint32_t map_flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    for (uint64_t vaddr = start_page; vaddr < end_page; vaddr += 4096) {
        if (!vmm_alloc_mapped_page(pml4, vaddr, map_flags)) {
            display_print("[ELF] Mapped: FAIL\n");
            return false;
        }
    }
    display_print("[ELF] Mapped: PASS\n");

    // ===========================================================================
    // SPRINT 4D: Load file data DIRECTLY into physical frames.
    //
    // ROOT CAUSE FIX: Do NOT switch CR3 to the process PML4. The kernel BSS
    // extends to ~10MB (PD entries 0-4 in the boot page tables), but the process
    // PML4 does not have those huge-page entries. Any kernel global variable
    // access above 2MB with the process PML4 active causes #PF -> double fault
    // -> triple fault -> Guru Meditation.
    //
    // Instead, use vmm_get_physical_address(pml4, vaddr) to find each page's
    // physical frame, then write file data directly to the physical address.
    // Physical addresses < 1GB are identity-mapped in the kernel PML4, so
    // (void*)phys_addr is a valid kernel virtual address. Zero CR3 switches.
    // ===========================================================================
    uint64_t file_remaining = phdr->p_filesz;
    uint64_t file_offset    = phdr->p_offset;
    uint64_t vaddr_cursor   = phdr->p_vaddr;

    display_print("[ELF] Loading file data...\n");

    while (file_remaining > 0) {
        uint64_t page_base    = vaddr_cursor & ~0xFFFULL;
        uint64_t page_offset  = vaddr_cursor & 0xFFFULL;
        uint64_t bytes_this   = 4096ULL - page_offset;
        if (bytes_this > file_remaining) bytes_this = file_remaining;

        uint64_t phys = vmm_get_physical_address(pml4, page_base);
        if (!phys) {
            display_print("[ELF] PhysAddr: FAIL\n");
            return false;
        }

        // Write directly to the physical frame (identity-mapped at kernel virt == phys)
        int got = vfs_pread(fd, (void*)(phys + page_offset), (uint32_t)bytes_this, file_offset);
        if (got < 0 || (uint64_t)got != bytes_this) {
            display_print("[ELF] Read: FAIL\n");
            return false;
        }

        file_remaining -= bytes_this;
        file_offset    += bytes_this;
        vaddr_cursor   += bytes_this;
    }
    display_print("[ELF] File data: PASS\n");

    // ===========================================================================
    // SPRINT 4E: BSS zeroing is NOT needed here.
    // vmm_alloc_mapped_page() calls memset(frame, 0, 4096) on every allocated
    // physical frame. All pages in the segment are pre-zeroed at allocation time.
    // The file data was loaded only for [p_vaddr .. p_vaddr+p_filesz), and the
    // remaining BSS region [p_vaddr+p_filesz .. p_vaddr+p_memsz) was zeroed
    // when the pages were allocated above.
    // ===========================================================================

    // ===========================================================================
    // SPRINT 4F: Apply final page permissions (remove write for read-only segments).
    // If the segment is writable (PF_W), final_flags == map_flags, so no remap needed.
    // Only non-writable segments (code, rodata) need the write bit stripped.
    // ===========================================================================
    uint32_t final_flags = PAGE_PRESENT | PAGE_USER;
    if (phdr->p_flags & PF_W) final_flags |= PAGE_WRITABLE;

#ifdef BOS_DEBUG
    display_print("Permissions: ");
    if (phdr->p_flags & PF_R) display_print("R");
    if (phdr->p_flags & PF_W) display_print("W");
    if (phdr->p_flags & PF_X) display_print("X");
    display_print("\n");
#endif

    if (final_flags != map_flags) {
        // Need to strip WRITABLE from read-only segments
        for (uint64_t vaddr = start_page; vaddr < end_page; vaddr += 4096) {
            uint64_t phys_addr = vmm_get_physical_address(pml4, vaddr);
            if (phys_addr) {
                vmm_map_page(pml4, phys_addr, vaddr, final_flags);
            }
        }
    }
    display_print("[ELF] Permissions: SET\n");

    return true;
}


ProcessImage* elf_load_image(void* pml4, const char* path) {
    display_print("\n[ELF]\n");
    int fd = vfs_open(path);
    if (fd < 0) {
        display_print("[FAIL] File Open\n");
        return NULL;
    }

    Elf64_Ehdr ehdr;
    if (vfs_pread(fd, &ehdr, sizeof(Elf64_Ehdr), 0) != sizeof(Elf64_Ehdr)) {
        display_print("[FAIL] Header Read\n");
        vfs_close(fd);
        return NULL;
    }

    if (!elf_verify_header(&ehdr)) {
        display_print("Header           FAIL\n");
        vfs_close(fd);
        return NULL;
    }
    display_print("Header            PASS\n");

    Elf64_Phdr* phdrs = kmalloc(ehdr.e_phnum * ehdr.e_phentsize);
    if (!phdrs) {
        display_print("[FAIL] Memory Alloc\n");
        vfs_close(fd);
        return NULL;
    }

    if (vfs_pread(fd, phdrs, ehdr.e_phnum * ehdr.e_phentsize, ehdr.e_phoff) != (int)(ehdr.e_phnum * ehdr.e_phentsize)) {
        display_print("[FAIL] Program Headers Read\n");
        kfree(phdrs);
        vfs_close(fd);
        return NULL;
    }

    elf_print_program_headers(phdrs, ehdr.e_phnum);
    display_print("Program Headers   PASS\n");
    
    if (!elf_segment_planner(phdrs, ehdr.e_phnum)) {
        kfree(phdrs);
        vfs_close(fd);
        return NULL;
    }

    ProcessImage* image = kmalloc(sizeof(ProcessImage));
    if (!image) {
        kfree(phdrs);
        vfs_close(fd);
        return NULL;
    }

    image->entry_point = ehdr.e_entry;
    image->pml4 = pml4;
    image->pid = 0; // Assigned by scheduler later
    image->image_base = 0xFFFFFFFFFFFFFFFFULL;
    image->image_size = 0;
    image->image_end = 0;
    image->segments_loaded = 0;
    image->page_count = 0;
    image->flags = 0;
    
    uint64_t max_vaddr_end = 0;

    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            if (phdrs[i].p_vaddr < image->image_base) {
                image->image_base = phdrs[i].p_vaddr;
            }
            uint64_t end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            if (end > max_vaddr_end) {
                max_vaddr_end = end;
            }
            image->segments_loaded++;
        }

        if (!elf_load_segment(pml4, fd, &phdrs[i], i)) {
            display_print("Memory Mapper    FAIL\n");
            kfree(image);
            kfree(phdrs);
            vfs_close(fd);
            return NULL;
        }
    }

    if (image->segments_loaded == 0) {
        image->image_base = 0;
        image->image_size = 0;
        image->image_end = 0;
        image->heap_start = 0;
    } else {
        image->image_size = max_vaddr_end - image->image_base;
        image->image_end = max_vaddr_end;
        image->heap_start = (max_vaddr_end + 0xFFF) & ~0xFFFULL; // Page align up
    }
    
    image->heap_end = image->heap_start; // Initially empty heap
    image->stack_bottom = 0;
    image->stack_top = 0; // Stack will be set up by User Stack Builder (Sprint 6)

    display_print("Memory Mapper     PASS\n");

    kfree(phdrs);
    vfs_close(fd);
    return image;
}

bool elf_load_segment_from_buffer(void* pml4, const uint8_t* elf_data, uint64_t elf_size, const Elf64_Phdr* phdr, uint16_t index) {
    (void)index;
    if (phdr->p_type != PT_LOAD) return true;

    uint64_t start_page = phdr->p_vaddr & ~0xFFFULL;
    uint64_t end_page   = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFFULL;

    uint32_t map_flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
    for (uint64_t vaddr = start_page; vaddr < end_page; vaddr += 4096) {
        if (!vmm_alloc_mapped_page(pml4, vaddr, map_flags)) {
            return false;
        }
    }

    uint64_t file_remaining = phdr->p_filesz;
    uint64_t file_offset    = phdr->p_offset;
    uint64_t vaddr_cursor   = phdr->p_vaddr;

    while (file_remaining > 0) {
        uint64_t page_base    = vaddr_cursor & ~0xFFFULL;
        uint64_t page_offset  = vaddr_cursor & 0xFFFULL;
        uint64_t bytes_in_page = 4096 - page_offset;
        uint64_t chunk = (file_remaining < bytes_in_page) ? file_remaining : bytes_in_page;

        uint64_t phys_addr = vmm_get_physical_address(pml4, page_base);
        if (!phys_addr) return false;

        uint8_t* dst = (uint8_t*)phys_addr + page_offset;
        if (file_offset + chunk > elf_size) return false;

        memcpy(dst, elf_data + file_offset, chunk);

        file_remaining -= chunk;
        file_offset    += chunk;
        vaddr_cursor   += chunk;
    }

    if (phdr->p_memsz > phdr->p_filesz) {
        uint64_t bss_remaining = phdr->p_memsz - phdr->p_filesz;
        while (bss_remaining > 0) {
            uint64_t page_base    = vaddr_cursor & ~0xFFFULL;
            uint64_t page_offset  = vaddr_cursor & 0xFFFULL;
            uint64_t bytes_in_page = 4096 - page_offset;
            uint64_t chunk = (bss_remaining < bytes_in_page) ? bss_remaining : bytes_in_page;

            uint64_t phys_addr = vmm_get_physical_address(pml4, page_base);
            if (!phys_addr) return false;

            uint8_t* dst = (uint8_t*)phys_addr + page_offset;
            memset(dst, 0, chunk);

            bss_remaining -= chunk;
            vaddr_cursor  += chunk;
        }
    }

    return true;
}

ProcessImage* elf_load_image_from_buffer(void* pml4, const void* buffer, uint64_t size) {
    if (!pml4 || !buffer || size < sizeof(Elf64_Ehdr)) return NULL;
    const uint8_t* elf_data = (const uint8_t*)buffer;
    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)elf_data;

    if (!elf_verify_header(ehdr)) return NULL;

    if (ehdr->e_phoff + (ehdr->e_phnum * ehdr->e_phentsize) > size) return NULL;
    const Elf64_Phdr* phdrs = (const Elf64_Phdr*)(elf_data + ehdr->e_phoff);

    ProcessImage* image = (ProcessImage*)kmalloc(sizeof(ProcessImage));
    if (!image) return NULL;
    memset(image, 0, sizeof(ProcessImage));

    image->entry_point = ehdr->e_entry;
    image->pml4 = pml4;
    image->image_base = 0xFFFFFFFFFFFFFFFFULL;

    uint64_t max_vaddr_end = 0;
    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            if (phdrs[i].p_vaddr < image->image_base) {
                image->image_base = phdrs[i].p_vaddr;
            }
            uint64_t end = phdrs[i].p_vaddr + phdrs[i].p_memsz;
            if (end > max_vaddr_end) {
                max_vaddr_end = end;
            }
            image->segments_loaded++;
        }

        if (!elf_load_segment_from_buffer(pml4, elf_data, size, &phdrs[i], i)) {
            kfree(image);
            return NULL;
        }
    }

    if (image->segments_loaded == 0) {
        image->image_base = 0;
        image->image_size = 0;
        image->image_end = 0;
        image->heap_start = 0;
    } else {
        image->image_size = max_vaddr_end - image->image_base;
        image->image_end = max_vaddr_end;
        image->heap_start = (max_vaddr_end + 0xFFF) & ~0xFFFULL;
    }
    image->heap_end = image->heap_start;
    return image;
}

