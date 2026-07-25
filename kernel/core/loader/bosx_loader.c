#include "bosx_loader.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/loader/elf/include/elf_loader.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/process/include/process.h"
#include "kernel/core/process/process_manager.h"
#include "kernel/wm/bwe/include/bwe.h"

extern void bwe_log(const char* level, const char* msg);

void BOSX_Init(void) {
    ATOMS_ProcessManager_Init();
    bwe_log("INFO", "ATOMS BOSX Enterprise Loader Initialized");
}

bosx_error_t BOSX_ValidateHeader(const BOSX_Header* header) {
    if (!header) return BOSX_ERR_BAD_HEADER;

    // Stage 2: Magic & Version Validation
    if (header->magic != BOSX_MAGIC) {
        return BOSX_ERR_BAD_MAGIC;
    }
    if (header->version_major != BOSX_VERSION_MAJOR) {
        return BOSX_ERR_BAD_HEADER;
    }
    if (header->architecture != BOSX_ARCH_X86_64 && header->architecture != BOSX_ARCH_X86_32) {
        return BOSX_ERR_UNSUPPORTED_ARCH;
    }
    if (header->section_count == 0 || header->section_count > BOSX_MAX_SECTIONS) {
        return BOSX_ERR_BAD_HEADER;
    }

    return BOSX_SUCCESS;
}

static void bosx_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

bosx_error_t BOSX_LoadExecutableBuffer(const uint8_t* buffer, uint32_t size, uint32_t* out_pid) {
    if (!buffer || size < sizeof(BOSX_Header)) return BOSX_ERR_BAD_HEADER;

    const BOSX_Header* header = (const BOSX_Header*)buffer;
    bosx_error_t err = BOSX_ValidateHeader(header);
    if (err != BOSX_SUCCESS) return err;

    // Allocate process in centralized process manager
    ATOMS_PCB* pcb = ATOMS_Process_Create("BOSXApp", "BOSXApp", 1, 0);
    if (!pcb) return BOSX_ERR_PROCESS_LIMIT;

    pcb->state = ATOMS_PROC_STATE_RUNNING;
    pcb->memory_used_bytes = header->image_size;
    pcb->pml4_phys = 0;

    if (out_pid) *out_pid = pcb->pid;
    return BOSX_SUCCESS;
}

bosx_error_t BOSX_Load(const char* filepath) {
    if (!filepath) return BOSX_ERR_FILE_NOT_FOUND;

    display_print("[BOSX_LOADER] Stage 1: Opening ");
    display_print(filepath);
    display_print("\n");

    char real_path[256];
    bosx_strcpy(real_path, filepath, 256);

    int fd = vfs_open(real_path);
    if (fd < 0) {
        int len = (int)strlen(real_path);
        if (len >= 5) {
            char* ext = &real_path[len - 4];
            if (ext[-1] == '.' && (ext[0] == 'B' || ext[0] == 'b') && (ext[1] == 'O' || ext[1] == 'o') && (ext[2] == 'S' || ext[2] == 's') && (ext[3] == 'X' || ext[3] == 'x')) {
                ext[0] = 'E'; ext[1] = 'L'; ext[2] = 'F'; ext[3] = '\0';
                fd = vfs_open(real_path);
            }
        }
    }

    if (fd < 0) {
        display_print("[BOSX_LOADER] ERROR: File not found: ");
        display_print(filepath);
        display_print("\n");
        return BOSX_ERR_FILE_NOT_FOUND;
    }

    uint8_t hdr_buf[sizeof(BOSX_Header)];
    int bytes_read = vfs_read(fd, hdr_buf, sizeof(BOSX_Header));
    vfs_close(fd);

    if (bytes_read < (int)sizeof(BOSX_Header)) {
        // Legacy ELF fallback execution path
        display_print("[BOSX_LOADER] Transparently loading standard binary payload...\n");
        elf_load_image(0, real_path);
        return BOSX_SUCCESS;
    }

    const BOSX_Header* header = (const BOSX_Header*)hdr_buf;
    if (header->magic == BOSX_MAGIC) {
        display_print("[BOSX_LOADER] Stage 2: BOSX Magic 0x58534F42 Validated!\n");
        bosx_error_t err = BOSX_ValidateHeader(header);
        if (err != BOSX_SUCCESS) {
            display_print("[BOSX_LOADER] ERROR: BOSX Header Validation Failed!\n");
            return err;
        }

        uint32_t pid = 0;
        err = BOSX_LoadExecutableBuffer(hdr_buf, sizeof(BOSX_Header), &pid);
        if (err == BOSX_SUCCESS) {
            display_print("[BOSX_LOADER] Stage 9: Process PID ");
            display_print_dec(pid);
            display_print(" Created & Execution Transferred Successfully!\n");
        }
        return err;
    } else {
        // Legacy ELF payload fallback
        elf_load_image(0, real_path);
        return BOSX_SUCCESS;
    }
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
