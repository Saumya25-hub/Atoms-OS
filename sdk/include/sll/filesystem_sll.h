#ifndef ATOMS_SDK_FILESYSTEM_SLL_H
#define ATOMS_SDK_FILESYSTEM_SLL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ATOMS Native SDK — filesystem.sll
typedef int32_t SLL_FileHandle;

SLL_FileHandle SLL_FileOpen(const char* path, const char* mode);
int32_t        SLL_FileRead(SLL_FileHandle handle, void* buffer, uint32_t bytes);
int32_t        SLL_FileWrite(SLL_FileHandle handle, const void* buffer, uint32_t bytes);
void           SLL_FileClose(SLL_FileHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_SDK_FILESYSTEM_SLL_H
