#include "platform/include/bos_window.h"
#include "platform/include/bos_events.h"

extern bool BOS_ValidateUserPointer(const void* ptr, size_t size, bool check_writable);
extern bool BOS_ValidateUserString(const char* str, size_t max_len);

/* System Call Gateway Stubs */
BOS_Result sys_platform_create_window(const BOS_WindowConfig* user_config, BOS_WindowHandle* user_out_handle) {
    if (!BOS_ValidateUserPointer(user_config, sizeof(BOS_WindowConfig), false)) {
        return BOS_ERROR_INVALID_ADDRESS;
    }
    if (!BOS_ValidateUserPointer(user_out_handle, sizeof(BOS_WindowHandle), true)) {
        return BOS_ERROR_INVALID_ADDRESS;
    }
    if (user_config->title && !BOS_ValidateUserString(user_config->title, 128)) {
        return BOS_ERROR_INVALID_ARGUMENT;
    }

    return BOS_CreateWindow(user_config, user_out_handle);
}

BOS_Result sys_platform_destroy_window(BOS_WindowHandle handle) {
    return BOS_DestroyWindow(handle);
}

BOS_Result sys_platform_get_event(BOS_EventQueue* queue, BOS_Event* user_out_event) {
    if (!BOS_ValidateUserPointer(user_out_event, sizeof(BOS_Event), true)) {
        return BOS_ERROR_INVALID_ADDRESS;
    }
    return BOS_EventQueue_Pop(queue, user_out_event);
}
