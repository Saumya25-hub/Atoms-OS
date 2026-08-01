#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

bool RegisterDragDrop(HWND hWnd, void* pDropTarget) {
    (void)hWnd; (void)pDropTarget;
    return true;
}

bool RevokeDragDrop(HWND hWnd) {
    (void)hWnd;
    return true;
}
