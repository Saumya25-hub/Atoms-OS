#include "bosx_loader.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/memory/vmm/include/paging.h"
#include "kernel/core/memory/pmm/include/pmm.h"
#include "kernel/core/process/include/process.h"
#include "kernel/core/process/include/process_image.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/core/scheduler/include/scheduler.h"
#include "kernel/core/syscall/include/syscall.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);
extern void com1_puts(const char* str);
extern void diag_put_hex64(uint64_t val);
extern void diag_put_dec(uint64_t val);

void BOSX_Init(void) {
    ATOMS_ProcessManager_Init();
    bwe_log("INFO", "ATOMS BOSX Enterprise Loader Initialized");
}

static void bosx_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static const char* bosx_basename(const char* path) {
    if (!path) return "Unknown";
    const char* last_slash = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') {
            last_slash = p + 1;
        }
    }
    return last_slash;
}

bosx_error_t BOSX_ValidateHeader(const BOSX_Header* header) {
    if (!header) return BOSX_ERR_BAD_HEADER;

    /* Stage 1: Magic Check */
    if (header->magic != BOSX_MAGIC) {
        return BOSX_ERR_BAD_MAGIC;
    }

    /* Stage 2: Version Validation */
    if (header->version_major != BOSX_VERSION_MAJOR) {
        return BOSX_ERR_BAD_HEADER;
    }

    /* Stage 3: Architecture Validation (Strict x86_64, Reject 32-bit) */
    if (header->architecture != BOSX_ARCH_X86_64) {
        return BOSX_ERR_UNSUPPORTED_ARCH;
    }

    /* Stage 4: Section Count Bounds */
    if (header->section_count == 0 || header->section_count > BOSX_MAX_SECTIONS) {
        return BOSX_ERR_BAD_HEADER;
    }

    /* Stage 5: Image Size Bounds (Must be > 0 and <= 64 MB) */
    if (header->image_size == 0 || header->image_size > (64 * 1024 * 1024)) {
        return BOSX_ERR_BAD_HEADER;
    }

    /* Stage 6: Image Base Canonical User Space Validation */
    if (header->image_base < VMM_USER_MIN_ADDRESS) {
        return BOSX_ERR_SECURITY_VIOLATION;
    }

    uint64_t image_end = header->image_base + (uint64_t)header->image_size;
    if (image_end < header->image_base || image_end > VMM_USER_MAX_ADDRESS) {
        return BOSX_ERR_OVERFLOW;
    }

    /* Reserved / Framebuffer memory check (0x80000000 .. 0xD0000000) */
    if ((header->image_base >= 0x80000000ULL && header->image_base < 0xD0000000ULL) ||
        (image_end > 0x80000000ULL && image_end <= 0xD0000000ULL)) {
        return BOSX_ERR_SECURITY_VIOLATION;
    }

    /* Page alignment check for base address */
    if ((header->image_base & 0xFFFULL) != 0) {
        return BOSX_ERR_BAD_HEADER;
    }

    return BOSX_SUCCESS;
}

bosx_error_t BOSX_ValidateSections(const BOSX_Header* header, const BOSX_SectionHeader* sections, uint32_t file_size) {
    if (!header || !sections) return BOSX_ERR_BAD_HEADER;

    bool found_exec_section = false;

    for (uint32_t i = 0; i < header->section_count; i++) {
        const BOSX_SectionHeader* sec = &sections[i];

        /* Check 1: File bounds with checked 64-bit arithmetic */
        if (!(sec->flags & BOSX_SEC_BSS)) {
            uint64_t file_span = (uint64_t)sec->raw_data_offset + (uint64_t)sec->raw_data_size;
            if (file_span > file_size || file_span < sec->raw_data_offset) {
                return BOSX_ERR_OVERFLOW;
            }
        } else {
            if (sec->raw_data_size != 0) {
                return BOSX_ERR_BAD_HEADER;
            }
        }

        /* Check 2: Virtual memory bounds within image_size */
        uint64_t virt_span = (uint64_t)sec->virtual_addr + (uint64_t)sec->virtual_size;
        if (virt_span > header->image_size || virt_span < sec->virtual_addr) {
            return BOSX_ERR_OVERFLOW;
        }

        /* Check 3: Raw size cannot exceed virtual size */
        if (sec->raw_data_size > sec->virtual_size) {
            return BOSX_ERR_BAD_HEADER;
        }

        /* Check 4: W^X Security Enforcement */
        if ((sec->flags & BOSX_SEC_EXEC) && (sec->flags & BOSX_SEC_WRITE)) {
            /* Strict W^X violation: Section cannot be simultaneously writable and executable */
            return BOSX_ERR_SECURITY_VIOLATION;
        }

        if (sec->flags & BOSX_SEC_EXEC) {
            found_exec_section = true;
            /* Entry point check: Does entry_point lie inside this executable section? */
            if (header->entry_point >= sec->virtual_addr &&
                header->entry_point < (sec->virtual_addr + sec->virtual_size)) {
                /* Valid entry point inside executable section */
            }
        }

        /* Check 5: Pairwise Interval Disjointness (No overlapping sections) */
        for (uint32_t j = i + 1; j < header->section_count; j++) {
            const BOSX_SectionHeader* other = &sections[j];
            uint64_t start_a = sec->virtual_addr;
            uint64_t end_a   = sec->virtual_addr + sec->virtual_size;
            uint64_t start_b = other->virtual_addr;
            uint64_t end_b   = other->virtual_addr + other->virtual_size;

            if (start_a < end_b && start_b < end_a) {
                return BOSX_ERR_OVERLAP;
            }
        }
    }

    if (!found_exec_section) {
        return BOSX_ERR_BAD_ENTRY;
    }

    /* Verify entry point falls within an executable section */
    bool entry_valid = false;
    for (uint32_t i = 0; i < header->section_count; i++) {
        if (sections[i].flags & BOSX_SEC_EXEC) {
            if (header->entry_point >= sections[i].virtual_addr &&
                header->entry_point < (sections[i].virtual_addr + sections[i].virtual_size)) {
                entry_valid = true;
                break;
            }
        }
    }

    if (!entry_valid) {
        return BOSX_ERR_BAD_ENTRY;
    }

    return BOSX_SUCCESS;
}

bosx_error_t BOSX_LoadFromVFS(const char* filepath, uint32_t* out_pid) {
    if (!filepath) return BOSX_ERR_FILE_NOT_FOUND;

    /* Step 1: Stat file to check existence, size, and Phase 7 execution permission */
    atoms_stat_t st;
    memset(&st, 0, sizeof(atoms_stat_t));
    int stat_res = vfs_stat(filepath, &st);
    if (stat_res != 0) {
        return BOSX_ERR_FILE_NOT_FOUND;
    }

    /* Phase 7 Permission Check: Target file must have execute bit (0111) */
    if ((st.st_mode & 0111) == 0) {
        return BOSX_ERR_PERMISSION;
    }

    if (st.st_size < sizeof(BOSX_Header)) {
        return BOSX_ERR_BAD_HEADER;
    }

    /* Step 2: Open file via VFS */
    int fd = vfs_open(filepath);
    if (fd < 0) {
        return BOSX_ERR_FILE_NOT_FOUND;
    }

    /* Step 3: Read BOSX Header */
    BOSX_Header header;
    int read_bytes = vfs_read(fd, &header, sizeof(BOSX_Header));
    if (read_bytes < (int)sizeof(BOSX_Header)) {
        vfs_close(fd);
        return BOSX_ERR_BAD_HEADER;
    }

    /* Step 4: Validate BOSX Header */
    bosx_error_t err = BOSX_ValidateHeader(&header);
    if (err != BOSX_SUCCESS) {
        vfs_close(fd);
        return err;
    }

    /* Step 5: Read Section Headers */
    uint32_t sec_table_size = header.section_count * sizeof(BOSX_SectionHeader);
    BOSX_SectionHeader sections[BOSX_MAX_SECTIONS];
    read_bytes = vfs_read(fd, sections, sec_table_size);
    if (read_bytes < (int)sec_table_size) {
        vfs_close(fd);
        return BOSX_ERR_BAD_HEADER;
    }

    /* Step 6: Validate Section Headers */
    err = BOSX_ValidateSections(&header, sections, (uint32_t)st.st_size);
    if (err != BOSX_SUCCESS) {
        vfs_close(fd);
        return err;
    }

    /* Step 7: Create Isolated Address Space via VMM */
    void* new_pml4 = vmm_create_address_space();
    if (!new_pml4) {
        vfs_close(fd);
        return BOSX_ERR_MEMORY_ALLOC;
    }

    /* Step 8: Map Sections into Isolated Address Space with W^X */
    for (uint32_t s = 0; s < header.section_count; s++) {
        const BOSX_SectionHeader* sec = &sections[s];
        uint64_t v_start = header.image_base + sec->virtual_addr;
        uint64_t v_end   = v_start + sec->virtual_size;

        uint64_t page_start = v_start & ~0xFFFULL;
        uint64_t page_end   = (v_end + 4095ULL) & ~0xFFFULL;

        uint32_t access = VMM_ACCESS_USER | VMM_ACCESS_READ;
        if (sec->flags & BOSX_SEC_WRITE) access |= VMM_ACCESS_WRITE;
        if (sec->flags & BOSX_SEC_EXEC)  access |= VMM_ACCESS_EXECUTE;

        uint32_t flags = PAGE_USER;
        if (access & VMM_ACCESS_WRITE) flags |= PAGE_WRITABLE;

        /* Seek to raw data in file if non-BSS */
        if (!(sec->flags & BOSX_SEC_BSS) && sec->raw_data_size > 0) {
            vfs_seek(fd, sec->raw_data_offset, 0 /* SEEK_SET */);
        }

        uint32_t raw_bytes_remaining = (sec->flags & BOSX_SEC_BSS) ? 0 : sec->raw_data_size;

        for (uint64_t va = page_start; va < page_end; va += 4096ULL) {
            void* frame = vmm_alloc_mapped_page(new_pml4, va, flags);
            if (!frame) {
                vmm_destroy_address_space(new_pml4);
                vfs_close(fd);
                return BOSX_ERR_MEMORY_ALLOC;
            }

            /* Zero-fill entire frame to prevent stale physical memory leakage */
            memset(frame, 0, 4096);

            /* Copy initialized data into frame */
            if (raw_bytes_remaining > 0) {
                uint64_t frame_offset = (va < v_start) ? (v_start - va) : 0;
                uint64_t chunk_len = 4096ULL - frame_offset;
                if (chunk_len > raw_bytes_remaining) {
                    chunk_len = raw_bytes_remaining;
                }

                if (chunk_len > 0) {
                    int r = vfs_read(fd, (uint8_t*)frame + frame_offset, (uint32_t)chunk_len);
                    if (r > 0) {
                        raw_bytes_remaining -= (uint32_t)r;
                    }
                }
            }
        }
    }

    /* Step 9: User Stack Setup (128 KB) */
    uint64_t stack_bottom = 0x7FE00000ULL;
    uint64_t stack_top    = 0x7FE20000ULL;
    for (uint64_t sva = stack_bottom; sva < stack_top; sva += 4096ULL) {
        void* sframe = vmm_alloc_mapped_page(new_pml4, sva, PAGE_USER | PAGE_WRITABLE);
        if (!sframe) {
            vmm_destroy_address_space(new_pml4);
            vfs_close(fd);
            return BOSX_ERR_MEMORY_ALLOC;
        }
        memset(sframe, 0, 4096);
    }

    /* All file I/O complete, close file handle */
    vfs_close(fd);

    /* Step 10: Create Process in Process Manager */
    const char* proc_name = bosx_basename(filepath);
    ATOMS_PCB* pcb = ATOMS_Process_Create(proc_name, filepath, 1, 0);
    if (!pcb) {
        vmm_destroy_address_space(new_pml4);
        return BOSX_ERR_PROCESS_LIMIT;
    }

    pcb->pml4_phys = (uint64_t)new_pml4;
    pcb->memory_used_bytes = header.image_size + (128 * 1024);

    /* Step 11: Configure Process Image and Spawn Task */
    ProcessImage img;
    memset(&img, 0, sizeof(ProcessImage));
    img.pml4 = new_pml4;
    img.entry_point = header.image_base + header.entry_point;
    img.image_base  = header.image_base;
    img.image_end   = header.image_base + header.image_size;
    img.stack_bottom = stack_bottom;
    img.stack_top    = stack_top - 16; /* 16-byte aligned */
    img.pid = pcb->pid;

    Task* task = process_spawn(&img, proc_name);
    if (!task) {
        /* process_spawn terminates PCB on failure */
        return BOSX_ERR_MEMORY_ALLOC;
    }

    if (out_pid) {
        *out_pid = pcb->pid;
    }

    return BOSX_SUCCESS;
}

bosx_error_t BOSX_Load(const char* filepath) {
    uint32_t pid = 0;
    return BOSX_LoadFromVFS(filepath, &pid);
}

bosx_error_t BOSX_LoadExecutableBuffer(const uint8_t* buffer, uint32_t size, uint32_t* out_pid) {
    if (!buffer || size < sizeof(BOSX_Header)) return BOSX_ERR_BAD_HEADER;

    const BOSX_Header* header = (const BOSX_Header*)buffer;
    bosx_error_t err = BOSX_ValidateHeader(header);
    if (err != BOSX_SUCCESS) return err;

    uint32_t sec_table_size = header->section_count * sizeof(BOSX_SectionHeader);
    if (size < sizeof(BOSX_Header) + sec_table_size) {
        return BOSX_ERR_BAD_HEADER;
    }

    const BOSX_SectionHeader* sections = (const BOSX_SectionHeader*)(buffer + sizeof(BOSX_Header));
    err = BOSX_ValidateSections(header, sections, size);
    if (err != BOSX_SUCCESS) return err;

    void* new_pml4 = vmm_create_address_space();
    if (!new_pml4) return BOSX_ERR_MEMORY_ALLOC;

    for (uint32_t s = 0; s < header->section_count; s++) {
        const BOSX_SectionHeader* sec = &sections[s];
        uint64_t v_start = header->image_base + sec->virtual_addr;
        uint64_t v_end   = v_start + sec->virtual_size;

        uint64_t page_start = v_start & ~0xFFFULL;
        uint64_t page_end   = (v_end + 4095ULL) & ~0xFFFULL;

        uint32_t flags = PAGE_USER;
        if (sec->flags & BOSX_SEC_WRITE) flags |= PAGE_WRITABLE;

        uint32_t raw_bytes_remaining = (sec->flags & BOSX_SEC_BSS) ? 0 : sec->raw_data_size;
        const uint8_t* raw_src = buffer + sec->raw_data_offset;

        for (uint64_t va = page_start; va < page_end; va += 4096ULL) {
            void* frame = vmm_alloc_mapped_page(new_pml4, va, flags);
            if (!frame) {
                vmm_destroy_address_space(new_pml4);
                return BOSX_ERR_MEMORY_ALLOC;
            }

            memset(frame, 0, 4096);

            if (raw_bytes_remaining > 0) {
                uint64_t frame_offset = (va < v_start) ? (v_start - va) : 0;
                uint64_t chunk_len = 4096ULL - frame_offset;
                if (chunk_len > raw_bytes_remaining) {
                    chunk_len = raw_bytes_remaining;
                }

                if (chunk_len > 0) {
                    memcpy((uint8_t*)frame + frame_offset, raw_src, chunk_len);
                    raw_src += chunk_len;
                    raw_bytes_remaining -= (uint32_t)chunk_len;
                }
            }
        }
    }

    uint64_t stack_bottom = 0x7FE00000ULL;
    uint64_t stack_top    = 0x7FE20000ULL;
    for (uint64_t sva = stack_bottom; sva < stack_top; sva += 4096ULL) {
        void* sframe = vmm_alloc_mapped_page(new_pml4, sva, PAGE_USER | PAGE_WRITABLE);
        if (!sframe) {
            vmm_destroy_address_space(new_pml4);
            return BOSX_ERR_MEMORY_ALLOC;
        }
        memset(sframe, 0, 4096);
    }

    ATOMS_PCB* pcb = ATOMS_Process_Create("BufferBOSX", "BufferBOSX", 1, 0);
    if (!pcb) {
        vmm_destroy_address_space(new_pml4);
        return BOSX_ERR_PROCESS_LIMIT;
    }

    pcb->pml4_phys = (uint64_t)new_pml4;
    pcb->memory_used_bytes = header->image_size + (128 * 1024);

    ProcessImage img;
    memset(&img, 0, sizeof(ProcessImage));
    img.pml4 = new_pml4;
    img.entry_point = header->image_base + header->entry_point;
    img.image_base  = header->image_base;
    img.image_end   = header->image_base + header->image_size;
    img.stack_bottom = stack_bottom;
    img.stack_top    = stack_top - 16;
    img.pid = pcb->pid;

    Task* task = process_spawn(&img, "BufferBOSX");
    if (!task) {
        return BOSX_ERR_MEMORY_ALLOC;
    }

    if (out_pid) *out_pid = pcb->pid;
    return BOSX_SUCCESS;
}

void bosx_loader_open(const char* filepath) {
    BOSX_Load(filepath);
}

void bosx_cleanup_process(uint32_t pid) {
    ATOMS_Process_Terminate(pid, 0);
}

BOSX_Process* BOSX_GetProcessByPID(uint32_t pid) {
    ATOMS_PCB* pcb = ATOMS_Process_GetByPID(pid);
    if (!pcb) return 0;

    static BOSX_Process translated;
    translated.pid = pcb->pid;
    bosx_strcpy(translated.name, pcb->name, 64);
    bosx_strcpy(translated.filepath, pcb->filepath, 128);
    
    switch (pcb->state) {
        case ATOMS_PROC_STATE_CLOSED:     translated.state = BOSX_PROC_CLOSED; break;
        case ATOMS_PROC_STATE_CREATING:   translated.state = BOSX_PROC_INIT; break;
        case ATOMS_PROC_STATE_READY:      translated.state = BOSX_PROC_INIT; break;
        case ATOMS_PROC_STATE_RUNNING:    translated.state = BOSX_PROC_RUNNING; break;
        case ATOMS_PROC_STATE_SUSPENDED:  translated.state = BOSX_PROC_SUSPENDED; break;
        case ATOMS_PROC_STATE_ZOMBIE:     translated.state = BOSX_PROC_TERMINATED; break;
        case ATOMS_PROC_STATE_TERMINATED: translated.state = BOSX_PROC_TERMINATED; break;
        default:                          translated.state = BOSX_PROC_CLOSED; break;
    }
    translated.memory_used = pcb->memory_used_bytes;
    translated.window_count = pcb->window_count;
    translated.entry_point = 0;
    translated.image_base = 0;
    translated.capabilities = pcb->capabilities_mask;

    return &translated;
}

uint32_t BOSX_GetProcessCount(void) {
    return ATOMS_Process_GetCount();
}

void BOSX_ProcessMonitor_Display(void) {
    display_print("\n=======================================================\n");
    display_print("        ATOMS OS BOSX Process Monitor                  \n");
    display_print("=======================================================\n");
    display_print("  PID    Process Name       State       RAM (KB)       \n");
    display_print("-------------------------------------------------------\n");

    for (uint32_t i = 0; i < ATOMS_MAX_PROCESSES; i++) {
        ATOMS_PCB* pcb = ATOMS_Process_GetByIndex(i);
        if (pcb) {
            display_print("  ");
            display_print_dec(pcb->pid);
            display_print("    ");
            display_print(pcb->name);
            display_print("         ");
            display_print(pcb->state == ATOMS_PROC_STATE_RUNNING ? "RUNNING" : "INIT");
            display_print("     ");
            display_print_dec(pcb->memory_used_bytes / 1024);
            display_print("\n");
        }
    }
    display_print("=======================================================\n\n");
}
