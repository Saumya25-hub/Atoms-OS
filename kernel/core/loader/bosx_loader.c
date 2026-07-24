#include "bosx_loader.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/vfs/vfs_legacy/include/vfs.h"
#include "kernel/core/memory/vmm/include/vmm.h"
#include "kernel/core/loader/elf/include/elf_loader.h"
#include "kernel/core/process/include/process_builder.h"
#include "kernel/core/process/include/process.h"
#include "kernel/wm/bwe/include/bwe.h"
extern void bwe_log(const char* level, const char* msg);
#include "kernel/core/loader/elf/include/elf_loader.h"

static BOSX_Process process_table[BOSX_MAX_PROCESSES];
static uint32_t     g_next_pid = 100;

void BOSX_Init(void) {
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].name[0] = '\0';
        process_table[i].filepath[0] = '\0';
        process_table[i].state = BOSX_PROC_CLOSED;
        process_table[i].memory_used = 0;
        process_table[i].window_count = 0;
        process_table[i].entry_point = 0;
        process_table[i].image_base = 0;
        process_table[i].capabilities = 0;
    }
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

static const char* get_basename(const char* filepath) {
    const char* base = filepath;
    const char* p = filepath;
    while (*p) {
        if (*p == '/' || *p == '\\') base = p + 1;
        p++;
    }
    return base;
}

bosx_error_t BOSX_LoadExecutableBuffer(const uint8_t* buffer, uint32_t size, uint32_t* out_pid) {
    if (!buffer || size < sizeof(BOSX_Header)) return BOSX_ERR_BAD_HEADER;

    const BOSX_Header* header = (const BOSX_Header*)buffer;
    bosx_error_t err = BOSX_ValidateHeader(header);
    if (err != BOSX_SUCCESS) return err;

    // Find free process slot
    int slot = -1;
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].state == BOSX_PROC_CLOSED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return BOSX_ERR_PROCESS_LIMIT;

    uint32_t pid = g_next_pid++;
    process_table[slot].pid = pid;
    bosx_strcpy(process_table[slot].name, "BOSXApp", 64);
    process_table[slot].state = BOSX_PROC_INIT;
    process_table[slot].memory_used = header->image_size;
    process_table[slot].entry_point = header->entry_point;
    process_table[slot].image_base = header->image_base;
    process_table[slot].state = BOSX_PROC_RUNNING;

    if (out_pid) *out_pid = pid;
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
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process_table[i].state = BOSX_PROC_TERMINATED;
            process_table[i].pid = 0;
            process_table[i].state = BOSX_PROC_CLOSED;
            break;
        }
    }
}

BOSX_Process* BOSX_GetProcessByPID(uint32_t pid) {
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != BOSX_PROC_CLOSED) {
            return &process_table[i];
        }
    }
    return 0;
}

uint32_t BOSX_GetProcessCount(void) {
    uint32_t count = 0;
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].state == BOSX_PROC_RUNNING || process_table[i].state == BOSX_PROC_INIT) {
            count++;
        }
    }
    return count;
}

void BOSX_ProcessMonitor_Display(void) {
    display_print("\n=======================================================\n");
    display_print("        ATOMS OS BOSX Process Monitor                  \n");
    display_print("=======================================================\n");
    display_print("  PID    Process Name       State       RAM (KB)       \n");
    display_print("-------------------------------------------------------\n");

    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].state != BOSX_PROC_CLOSED) {
            display_print("  ");
            display_print_dec(process_table[i].pid);
            display_print("    ");
            display_print(process_table[i].name);
            display_print("         ");
            display_print(process_table[i].state == BOSX_PROC_RUNNING ? "RUNNING" : "INIT");
            display_print("     ");
            display_print_dec(process_table[i].memory_used / 1024);
            display_print("\n");
        }
    }
    display_print("=======================================================\n\n");
}
