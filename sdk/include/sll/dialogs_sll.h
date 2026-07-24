#ifndef ATOMS_SDK_DIALOGS_SLL_H
#define ATOMS_SDK_DIALOGS_SLL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ATOMS Native SDK — dialogs.sll
uint32_t SLL_ShowMessageBox(uint32_t app_id, const char* title, const char* message, uint32_t icon);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SDK_DIALOGS_SLL_H
