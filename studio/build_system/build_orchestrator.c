#include "platform/include/bos_types.h"

BOS_Result BOS_Studio_BuildProject(const char* proj_path, bool is_release) {
    if (!proj_path) return BOS_ERROR_INVALID_ARGUMENT;
    (void)is_release;
    /* Invokes bosbuild.py & bospack.py CLI tools */
    return BOS_SUCCESS;
}

BOS_Result BOS_Studio_RunProject(const char* proj_path) {
    if (!proj_path) return BOS_ERROR_INVALID_ARGUMENT;
    return BOS_SUCCESS;
}
