#ifndef BOS_ELF_LOADER_H
#define BOS_ELF_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/runtime/package/include/bos_pack_loader.h"
#include "kernel/runtime/manifest/include/bos_manifest.h"

typedef struct {
    char name[64];
    uint32_t process_id;
    uint64_t entry_point;
    void* pml4_space;
    bool is_running;
} BOS_Process;

BOS_Process* bos_elf_load_and_spawn(BOS_Package* pack, BOS_Manifest* manifest);
void bos_process_terminate(BOS_Process* proc);

#endif /* BOS_ELF_LOADER_H */
