#include "../include/user32_api.h"
#include "kernel/bar/include/bar_api.h"

bool User32RegisterDragDrop(HWND hWnd, void* pDropTarget) {
    (void)hWnd; (void)pDropTarget;
    return true;
}

bool User32RevokeDragDrop(HWND hWnd) {
    (void)hWnd;
    return true;
}
