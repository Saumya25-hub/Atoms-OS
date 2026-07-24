#include "abe_web_storage.h"
#include "../diagnostics/abe_diagnostics.h"
#include "kernel/core/lib/include/string.h"

static ABE_WebStorage g_local_storage;
static ABE_WebStorage g_session_storage;
static bool g_web_storage_initialized = false;

ABE_Error ABE_WebStorage_Init(void) {
    memset(&g_local_storage, 0, sizeof(ABE_WebStorage));
    memset(&g_session_storage, 0, sizeof(ABE_WebStorage));
    g_local_storage.handle = 1;
    g_local_storage.in_use = true;
    g_session_storage.handle = 2;
    g_session_storage.is_session = true;
    g_session_storage.in_use = true;

    g_web_storage_initialized = true;
    ABE_Log(ABE_LOG_INFO, "WEBSTORAGE", "ABE Web Storage & Cookie Jar Engine initialized");
    return ABE_SUCCESS;
}

ABE_Error ABE_WebStorage_Shutdown(void) {
    g_web_storage_initialized = false;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebStorage_GetLocal(ABE_StorageHandle* out_storage) {
    if (!out_storage) return ABE_ERR_INVALID_PARAM;
    *out_storage = g_local_storage.handle;
    return ABE_SUCCESS;
}

ABE_Error ABE_WebStorage_GetSession(ABE_StorageHandle* out_storage) {
    if (!out_storage) return ABE_ERR_INVALID_PARAM;
    *out_storage = g_session_storage.handle;
    return ABE_SUCCESS;
}

static ABE_WebStorage* GetStorageByHandle(ABE_StorageHandle handle) {
    if (handle == g_local_storage.handle) return &g_local_storage;
    if (handle == g_session_storage.handle) return &g_session_storage;
    return NULL;
}

ABE_Error ABE_WebStorage_Set(ABE_StorageHandle storage, const char* key, const char* val) {
    if (!g_web_storage_initialized || !key || !val) return ABE_ERR_INVALID_PARAM;
    ABE_WebStorage* st = GetStorageByHandle(storage);
    if (!st) return ABE_ERR_INVALID_PARAM;

    // Update existing key if found
    for (uint32_t i = 0; i < ABE_MAX_STORAGE_ITEMS; i++) {
        if (st->items[i].in_use && strcmp(st->items[i].key, key) == 0) {
            strncpy(st->items[i].val, val, sizeof(st->items[i].val) - 1);
            ABE_Diag_RecordStorageOp(true);
            return ABE_SUCCESS;
        }
    }

    // Insert new item
    for (uint32_t i = 0; i < ABE_MAX_STORAGE_ITEMS; i++) {
        if (!st->items[i].in_use) {
            strncpy(st->items[i].key, key, sizeof(st->items[i].key) - 1);
            strncpy(st->items[i].val, val, sizeof(st->items[i].val) - 1);
            st->items[i].in_use = true;
            st->count++;
            ABE_Diag_RecordStorageOp(true);
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_WEB_STORAGE_FULL;
}

ABE_Error ABE_WebStorage_Get(ABE_StorageHandle storage, const char* key, char* out_buf, size_t max_len) {
    if (!g_web_storage_initialized || !key || !out_buf || max_len == 0) return ABE_ERR_INVALID_PARAM;
    ABE_WebStorage* st = GetStorageByHandle(storage);
    if (!st) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_STORAGE_ITEMS; i++) {
        if (st->items[i].in_use && strcmp(st->items[i].key, key) == 0) {
            strncpy(out_buf, st->items[i].val, max_len - 1);
            ABE_Diag_RecordStorageOp(false);
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebStorage_Remove(ABE_StorageHandle storage, const char* key) {
    if (!g_web_storage_initialized || !key) return ABE_ERR_INVALID_PARAM;
    ABE_WebStorage* st = GetStorageByHandle(storage);
    if (!st) return ABE_ERR_INVALID_PARAM;

    for (uint32_t i = 0; i < ABE_MAX_STORAGE_ITEMS; i++) {
        if (st->items[i].in_use && strcmp(st->items[i].key, key) == 0) {
            st->items[i].in_use = false;
            if (st->count > 0) st->count--;
            return ABE_SUCCESS;
        }
    }
    return ABE_ERR_INVALID_PARAM;
}

ABE_Error ABE_WebStorage_Clear(ABE_StorageHandle storage) {
    if (!g_web_storage_initialized) return ABE_ERR_INVALID_PARAM;
    ABE_WebStorage* st = GetStorageByHandle(storage);
    if (!st) return ABE_ERR_INVALID_PARAM;

    memset(st->items, 0, sizeof(st->items));
    st->count = 0;
    return ABE_SUCCESS;
}
