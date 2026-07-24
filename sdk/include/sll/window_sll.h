#ifndef ATOMS_SDK_WINDOW_SLL_H
#define ATOMS_SDK_WINDOW_SLL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ATOMS Native SDK — window.sll
typedef uint32_t SLL_WindowHandle;

SLL_WindowHandle SLL_CreateWindow(uint32_t app_id, int x, int y, int width, int height, const char* title);
void             SLL_DestroyWindow(uint32_t app_id, SLL_WindowHandle win);
void             SLL_SetWindowTitle(SLL_WindowHandle win, const char* title);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SDK_WINDOW_SLL_H
