#ifndef BOTREE_NAV_H
#define BOTREE_NAV_H

#include "botree_types.h"

// Navigation Engine Public Functions
BDeNavHandle BDe_NavCreateSession(uint32_t owner_pid);
void         BDe_NavDestroySession(BDeNavHandle handle);
int32_t      BDe_NavOpen(BDeNavHandle handle, const char* path);
int32_t      BDe_NavBack(BDeNavHandle handle, char* out_path, size_t max_len);
int32_t      BDe_NavForward(BDeNavHandle handle, char* out_path, size_t max_len);
int32_t      BDe_NavUp(BDeNavHandle handle, char* out_path, size_t max_len);
const char*  BDe_NavGetCurrentDir(BDeNavHandle handle);
bool         BDe_NavCanBack(BDeNavHandle handle);
bool         BDe_NavCanForward(BDeNavHandle handle);

#endif // BOTREE_NAV_H
