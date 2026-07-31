#ifndef STUDIO_PROJECT_H
#define STUDIO_PROJECT_H

#include "platform/include/bos_types.h"

typedef struct {
    char project_name[64];
    char project_path[128];
    char target_sdk_version[32];
    char active_form[64];
} BOS_StudioProject;

BOS_Result BOS_Studio_CreateProject(const char* name, const char* dir_path, BOS_StudioProject* out_proj);
BOS_Result BOS_Studio_OpenProject(const char* proj_file_path, BOS_StudioProject* out_proj);

#endif /* STUDIO_PROJECT_H */
