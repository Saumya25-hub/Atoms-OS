#include "../include/kernel32_api.h"

BOOL kernel32_sync_init_rwlock(void* lock) {
    (void)lock;
    return true;
}
