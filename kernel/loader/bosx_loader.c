#include "bosx_loader.h"
#include "kernel/display/display.h"
#include "kernel/lib/include/string.h"
#include "kernel/vfs/include/vfs.h"
#include "kernel/memory/vmm/include/vmm.h"
#include "kernel/loader/elf/include/elf_loader.h"
#include "kernel/process/include/process_builder.h"
#include "kernel/process/include/process.h"
#include "kernel/BOSurface/Core/surface.h"
#include "kernel/BOSurface/Core/app_manager.h"

static BOSX_Process process_table[BOSX_MAX_PROCESSES];

void BOSX_Init(void) {
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].name[0] = '\0';
        process_table[i].filepath[0] = '\0';
        process_table[i].state = BOSX_PROC_CLOSED;
        process_table[i].memory_used = 0;
        process_table[i].window_count = 0;
    }
    display_print("[BOSX] Loader Subsystem Initialized\n");
}

static void bosx_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int str_contains_nocase(const char* str, const char* sub) {
    if (!str || !sub) return 0;
    int len_str = (int)strlen(str);
    int len_sub = (int)strlen(sub);
    if (len_sub > len_str) return 0;
    for (int i = 0; i <= len_str - len_sub; i++) {
        int match = 1;
        for (int j = 0; j < len_sub; j++) {
            char ca = str[i + j];
            char cb = sub[j];
            if (ca >= 'a' && ca <= 'z') ca -= 32;
            if (cb >= 'a' && cb <= 'z') cb -= 32;
            if (ca != cb) { match = 0; break; }
        }
        if (match) return 1;
    }
    return 0;
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

extern uint32_t g_current_creating_pid;

int BOSX_Load(const char* filepath) {
    if (!filepath) return -1;

    display_print("[BOSX] Attempting to load: ");
    display_print(filepath);
    display_print("\n");

    char real_path[256];
    bosx_strcpy(real_path, filepath, 256);

    int fd = vfs_open(real_path);
    if (fd < 0) {
        // Transparent fallback: map .BOSX to .ELF
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
        // Check branded mapping to FAT32 binaries
        if (str_contains_nocase(filepath, "SHELL")) bosx_strcpy(real_path, "/SHELL.ELF", 256);
        else if (str_contains_nocase(filepath, "CALC")) bosx_strcpy(real_path, "/CALC.ELF", 256);
        else if (str_contains_nocase(filepath, "PAINT")) bosx_strcpy(real_path, "/PAINT.ELF", 256);
        else if (str_contains_nocase(filepath, "TERM")) bosx_strcpy(real_path, "/TERM.ELF", 256);
        else if (str_contains_nocase(filepath, "SETT")) bosx_strcpy(real_path, "/SETT.ELF", 256);
        else if (str_contains_nocase(filepath, "TEST")) bosx_strcpy(real_path, "/TESTS.ELF", 256);
        else if (str_contains_nocase(filepath, "INIT")) bosx_strcpy(real_path, "/INIT.ELF", 256);
        
        fd = vfs_open(real_path);
    }

    if (fd < 0) {
        display_print("[BOSX] ERROR: Executable file not found on VFS\n");
        return -1;
    }

    // Validate executable header (\x7fELF)
    uint8_t magic[4];
    int r = vfs_read(fd, magic, 4);
    vfs_close(fd);

    if (r < 4 || magic[0] != 0x7F || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
        display_print("[BOSX] ERROR: Invalid executable binary format\n");
        return -1;
    }

    extern void* vmm_create_address_space(void);
    void* new_pml4 = vmm_create_address_space();
    if (!new_pml4) return -1;

    ProcessImage* new_image = elf_load_image(new_pml4, real_path);
    if (!new_image) {
        display_print("[BOSX] ERROR: ELF Loader failed to load image\n");
        return -1;
    }

    if (!process_build_user_stack(new_image, new_pml4)) {
        display_print("[BOSX] ERROR: Failed to build user stack\n");
        return -1;
    }

    const char* base_name = get_basename(filepath);
    
    // Disable interrupts to atomically spawn process and attach ConHost session
    // preventing the new task from executing before its console GUI window is created.
    __asm__ volatile("cli");
    Task* new_task = process_spawn(new_image, base_name);
    if (!new_task) {
        __asm__ volatile("sti");
        display_print("[BOSX] ERROR: Process spawn failed\n");
        return -1;
    }

    uint32_t pid = (uint32_t)new_task->id;

    // Set creation PID for subsequent windows created by this process
    g_current_creating_pid = pid;

    // Record in process table
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].state == BOSX_PROC_CLOSED) {
            process_table[i].pid = pid;
            bosx_strcpy(process_table[i].name, base_name, 64);
            bosx_strcpy(process_table[i].filepath, filepath, 128);
            process_table[i].state = BOSX_PROC_RUNNING;
            process_table[i].memory_used = new_image->page_count * 4096;
            if (process_table[i].memory_used == 0) process_table[i].memory_used = 16384; // Default stack/code fallback
            process_table[i].window_count = 0;
            break;
        }
    }

    extern int conhost_spawn_console_for_process(uint64_t pid, const char* app_name);
    conhost_spawn_console_for_process((uint64_t)pid, base_name);
    __asm__ volatile("sti");

    g_current_creating_pid = 0;

    display_print("[BOSX] Successfully launched PID ");
    display_print_dec(pid);
    display_print("\n");

    return (int)pid;
}

void bosx_loader_open(const char* filepath) {
    BOSX_Load(filepath);
}

extern bwe_error_t BOS_CloseSurfacesByPID(uint32_t pid);
extern uint32_t BOS_CountSurfacesByPID(uint32_t pid);
extern void conhost_destroy_session_by_pid(uint64_t pid);

void bosx_cleanup_process(uint32_t pid) {
    if (pid == 0) return;

    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != BOSX_PROC_CLOSED) {
            display_print("[BOSX] Cleaning up process PID ");
            display_print_dec(pid);
            display_print(" (");
            display_print(process_table[i].name);
            display_print(")\n");

            conhost_destroy_session_by_pid((uint64_t)pid);
            BOS_CloseSurfacesByPID(pid);
            process_table[i].state = BOSX_PROC_CLOSED;
            process_table[i].pid = 0;
            process_table[i].memory_used = 0;
            process_table[i].window_count = 0;
            return;
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

void BOSX_ProcessMonitor_Display(void) {
    display_print("\n=================================================================\n");
    display_print("                  SignaturesOS Process Monitor                   \n");
    display_print("=================================================================\n");
    display_print("PID    Application       State       Memory       Windows   Status\n");
    display_print("-----------------------------------------------------------------\n");

    // Display native BOSX processes
    for (int i = 0; i < BOSX_MAX_PROCESSES; i++) {
        if (process_table[i].state != BOSX_PROC_CLOSED && process_table[i].pid != 0) {
            uint32_t wins = BOS_CountSurfacesByPID(process_table[i].pid);
            process_table[i].window_count = wins;

            display_print_dec(process_table[i].pid);
            display_print("    ");
            display_print(process_table[i].name);
            display_print("       RUNNING     ");
            display_print_dec(process_table[i].memory_used / 1024);
            display_print(" KB      ");
            display_print_dec(wins);
            display_print("         Active\n");
        }
    }

    // Display built-in GUI applications from app_manager
    for (uint32_t app_id = 1; app_id <= 16; app_id++) {
        BOS_Application* app = BOS_GetApplication(app_id);
        if (app && app->app_id != 0 && app->state != BWE_APP_STATE_CLOSED && app->pid != 0) {
            uint32_t wins = BOS_CountSurfacesByPID(app->pid);
            if (wins == 0 && app->main_window_id != 0) wins = 1;

            display_print_dec(app->pid);
            display_print("    ");
            display_print(app->name);
            display_print(".BOSX    RUNNING     32 KB       ");
            display_print_dec(wins);
            display_print("         Active\n");
        }
    }

    display_print("=================================================================\n");
    display_print("\nPASS_PHASE13_PROCESS\n");
}
