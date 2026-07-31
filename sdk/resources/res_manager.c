#include "sdk/include/bos/bos_res.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_Result BOS_Resource_Load(const char* res_id, BOS_ResourceType type, BOS_Resource** out_res) {
    if (!res_id || !out_res) return BOS_ERROR_INVALID_ARGUMENT;

    BOS_Resource* res = (BOS_Resource*)kmalloc(sizeof(BOS_Resource));
    if (!res) return BOS_ERROR_OUT_OF_MEMORY;

    memset(res, 0, sizeof(BOS_Resource));
    strncpy(res->res_id, res_id, 63);
    res->res_id[63] = '\0';
    res->type = type;
    res->data_ptr = NULL;
    res->data_size = 0;

    *out_res = res;
    return BOS_SUCCESS;
}

void BOS_Resource_Free(BOS_Resource* res) {
    if (res) {
        kfree(res);
    }
}
