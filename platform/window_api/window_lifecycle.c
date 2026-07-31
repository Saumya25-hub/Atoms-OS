#include "platform/include/bos_window.h"

typedef enum {
    BOS_WINDOW_STATE_UNINITIALIZED = 0,
    BOS_WINDOW_STATE_CREATED,
    BOS_WINDOW_STATE_VISIBLE,
    BOS_WINDOW_STATE_HIDDEN,
    BOS_WINDOW_STATE_CLOSING,
    BOS_WINDOW_STATE_DESTROYED
} BOS_WindowState;

typedef struct {
    BOS_WindowHandle handle;
    BOS_WindowState  state;
} BOS_WindowLifecycleNode;

BOS_Result BOS_WindowLifecycle_Transition(BOS_WindowHandle handle, uint32_t target_state) {
    if (handle == BOS_INVALID_WINDOW_HANDLE) return BOS_ERROR_INVALID_HANDLE;
    (void)target_state;
    return BOS_SUCCESS;
}
