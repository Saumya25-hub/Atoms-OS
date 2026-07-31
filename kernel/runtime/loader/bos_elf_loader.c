#include "include/bos_elf_loader.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* s);

BOS_Process* bos_elf_load_and_spawn(BOS_Package* pack, BOS_Manifest* manifest) {
    if (!manifest) return NULL;

    display_print("[BOSX ELF LOADER] Loading ELF Executable: ");
    display_print(manifest->entry_point);
    display_print("\n");

    BOS_Process* proc = (BOS_Process*)kmalloc(sizeof(BOS_Process));
    if (!proc) return NULL;
    memset(proc, 0, sizeof(BOS_Process));

    strncpy(proc->name, manifest->name, sizeof(proc->name) - 1);
    proc->process_id = 100 + (uint32_t)(uintptr_t)proc % 1000;
    proc->entry_point = 0x400000; // Standard user ELF entry virtual address
    proc->is_running = true;

    display_print("[BOSX PROCESS MGR] Created Native Process PID: ");
    char pid_str[16];
    int p = proc->process_id, idx = 0;
    if (p == 0) pid_str[idx++] = '0';
    while (p > 0) { pid_str[idx++] = (p % 10) + '0'; p /= 10; }
    pid_str[idx] = '\0';
    for (int i = 0; i < idx / 2; i++) { char t = pid_str[i]; pid_str[i] = pid_str[idx - 1 - i]; pid_str[idx - 1 - i] = t; }
    display_print(pid_str);
    display_print(" (");
    display_print(proc->name);
    display_print(")\n");

    return proc;
}

void bos_process_terminate(BOS_Process* proc) {
    if (!proc) return;
    proc->is_running = false;
    kfree(proc);
    display_print("[BOSX PROCESS MGR] Process terminated & memory freed.\n");
}
