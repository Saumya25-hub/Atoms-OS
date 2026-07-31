#include "studio/include/studio_project.h"
#include "kernel/core/lib/include/string.h"

BOS_Result BOS_Studio_CreateProject(const char* name, const char* dir_path, BOS_StudioProject* out_proj) {
    if (!name || !dir_path || !out_proj) return BOS_ERROR_INVALID_ARGUMENT;

    memset(out_proj, 0, sizeof(BOS_StudioProject));
    strncpy(out_proj->project_name, name, 63);
    strncpy(out_proj->project_path, dir_path, 127);
    strncpy(out_proj->target_sdk_version, "1.0.0-PHASE4", 31);
    strncpy(out_proj->active_form, "MainWindow.bosform", 63);
    return BOS_SUCCESS;
}

BOS_Result BOS_Studio_OpenProject(const char* proj_file_path, BOS_StudioProject* out_proj) {
    if (!proj_file_path || !out_proj) return BOS_ERROR_INVALID_ARGUMENT;
    return BOS_Studio_CreateProject("MyStudioApp", proj_file_path, out_proj);
}
