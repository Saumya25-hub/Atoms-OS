/*
 * ATOMS OS — Google V8 Engine Platform Adapter Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_V8_PLATFORM_H_
#define ATOMS_V8_PLATFORM_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool AtomsV8_Initialize(void);
void AtomsV8_Shutdown(void);
bool AtomsV8_ExecuteScript(const char* js_source, char* out_buf, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_V8_PLATFORM_H_
