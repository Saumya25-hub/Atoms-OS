#ifndef ATOMS_APP_LOADER_H
#define ATOMS_APP_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/surface/surface.h"
#include "kernel/application/app_manager/app_manager.h"

typedef struct {
    uint32_t magic;         // 'ATOP' = 0x504F5441 (ATOMS Executable Package Magic)
    uint16_t version_major; // 1
    uint16_t version_minor; // 0
    uint32_t entry_point;   // Offset to entry
    uint32_t image_size;    // Executable memory size
    uint32_t capabilities;  // Required Security Capabilities
    char     name[64];
} ATOMS_AppHeader;

void        ATOMS_AppLoader_Init(void);
bwe_error_t ATOMS_ValidateExecutable(const char* exec_path, ATOMS_AppHeader* out_header);
bwe_error_t ATOMS_LoadApplication(const char* exec_path, uint32_t* out_app_id);
bwe_error_t ATOMS_UnloadApplication(uint32_t app_id);

#endif // ATOMS_APP_LOADER_H
