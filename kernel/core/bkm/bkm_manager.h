#ifndef ATOMS_BKM_MANAGER_H
#define ATOMS_BKM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// ATOMS Kernel Module (BKM) Framework (Phase 10)
// ============================================================

#define ATOMS_MAX_BKM_MODULES 32

typedef enum {
    BKM_DRIVER_DISPLAY,
    BKM_DRIVER_AUDIO,
    BKM_DRIVER_STORAGE,
    BKM_DRIVER_NETWORK,
    BKM_DRIVER_INPUT,
    BKM_DRIVER_SYSTEM
} BKM_DriverClass;

typedef struct {
    char            name[64];
    char            version[16];
    BKM_DriverClass driver_class;
    bool            initialized;
    int             (*on_init)(void);
    void            (*on_shutdown)(void);
} BKM_Module;

void       ATOMS_BKM_Init(void);
bool       ATOMS_BKM_RegisterModule(const char* name, const char* version, BKM_DriverClass driver_class, int (*on_init)(void), void (*on_shutdown)(void));
uint32_t   ATOMS_BKM_GetModuleCount(void);
void       ATOMS_BKM_ShutdownAll(void);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_BKM_MANAGER_H
