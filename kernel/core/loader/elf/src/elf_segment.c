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

#define USER_VIRTUAL_BASE 0x400000

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

#ifdef BOS_DEBUG
    display_print("\n[ELF]\nSegment ");
    display_print_dec(index);
    display_print("\n");
#endif

    uint64_t start_page = phdr->p_vaddr & ~0xFFFULL;
    uint64_t end_page = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFFULL;
    uint64_t pages = (end_page - start_page) / 4096;
    
#ifdef BOS_DEBUG
    display_print("Pages : ");
    display_print_dec(pages);
    display_print("\n");
#endif
    
    uint32_t map_flags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

    // Sprint 4B & 4C: Memory Allocation & Virtual Mapping
    for (uint64_t vaddr = start_page; vaddr < end_page; vaddr += 4096) {
        if (!vmm_alloc_mapped_page(pml4, vaddr, map_flags)) {
#ifdef BOS_DEBUG
            display_print("Mapped : FAIL\n");
#endif
            return false;
        }
    }
#ifdef BOS_DEBUG
    display_print("Mapped : PASS\n");
#endif

    // Sprint 4D: Segment Loader
    // Sprint 4D: Segment Loader
    // We must disable interrupts so the scheduler doesn't run while we are in the new CR3.
    // Otherwise, the scheduler could switch back to the original CR3, breaking our loads!
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(rflags));
    
    void* active_pml4 = vmm_get_active_pml4();
    if (active_pml4 != pml4) {
        vmm_switch_address_space(pml4);
    }

    uint64_t read_bytes = vfs_pread(fd, (void*)phdr->p_vaddr, phdr->p_filesz, phdr->p_offset);

    if (read_bytes < 0 || (uint32_t)read_bytes != phdr->p_filesz) {
#ifdef BOS_DEBUG
        display_print("Copied : FAIL\n");
#endif
        if (active_pml4 != pml4) {
            vmm_switch_address_space(active_pml4);
        }
        __asm__ volatile("push %0; popfq" : : "r"(rflags));
        return false;
    }
#ifdef BOS_DEBUG
    display_print("Copied : PASS\n");
#endif

    // Sprint 4E: Finalize
    if (phdr->p_memsz > phdr->p_filesz) {
        uint64_t bss_start = phdr->p_vaddr + phdr->p_filesz;
        uint32_t bss_size = phdr->p_memsz - phdr->p_filesz;
        memset((void*)bss_start, 0, bss_size);
    }
#ifdef BOS_DEBUG
    display_print("Zero   : PASS\n");
#endif

    uint32_t final_flags = PAGE_PRESENT | PAGE_USER;
    if (phdr->p_flags & PF_W) final_flags |= PAGE_WRITABLE;

#ifdef BOS_DEBUG
    display_print("Permissions : ");
    if (phdr->p_flags & PF_R) display_print("R");
    if (phdr->p_flags & PF_W) display_print("W");
    if (phdr->p_flags & PF_X) display_print("X");
    display_print("\n");
#endif

    for (uint64_t vaddr = start_page; vaddr < end_page; vaddr += 4096) {
        uint64_t phys_addr = vmm_get_physical_address(pml4, vaddr);
        if (phys_addr) {
            vmm_map_page(pml4, phys_addr, vaddr, final_flags);
        }
    }

    if (phdr->p_filesz > 0) {
        uint8_t first_byte_file;
        if (vfs_pread(fd, &first_byte_file, 1, phdr->p_offset) == 1) {
            uint8_t first_byte_mem = *((uint8_t*)phdr->p_vaddr);
            if (first_byte_mem != first_byte_file) {
#ifdef BOS_DEBUG
                display_print("Verify : FAIL\n");
#endif
                if (active_pml4 != pml4) {
                    vmm_switch_address_space(active_pml4);
                }
                __asm__ volatile("push %0; popfq" : : "r"(rflags));
                return false;
            }
        }
    }
#ifdef BOS_DEBUG
    display_print("Verify : PASS\n");
#endif

    if (active_pml4 != pml4) {
        vmm_switch_address_space(active_pml4);
    }
    __asm__ volatile("push %0; popfq" : : "r"(rflags));

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

