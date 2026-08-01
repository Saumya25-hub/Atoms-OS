#ifndef BOTREE_PATH_H
#define BOTREE_PATH_H

#include "botree_types.h"

// Path Engine Public Functions
int32_t BDe_PathNormalize(const char* in_path, char* out_buf, size_t max_len);
int32_t BDe_PathCanonicalize(const char* base_path, const char* rel_path, char* out_buf, size_t max_len);
int32_t BDe_PathJoin(const char* path_a, const char* path_b, char* out_buf, size_t max_len);
int32_t BDe_PathGetDirname(const char* in_path, char* out_buf, size_t max_len);
int32_t BDe_PathGetBasename(const char* in_path, char* out_buf, size_t max_len);
int32_t BDe_PathGetExtension(const char* in_path, char* out_buf, size_t max_len);
bool    BDe_PathIsAbsolute(const char* path);
bool    BDe_PathEquals(const char* path_a, const char* path_b);

#endif // BOTREE_PATH_H
