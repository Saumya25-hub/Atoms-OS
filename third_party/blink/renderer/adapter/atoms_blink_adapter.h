/*
 * ATOMS OS — Google Chromium / Blink Rendering Engine Adapter Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 */

#ifndef ATOMS_BLINK_ADAPTER_H_
#define ATOMS_BLINK_ADAPTER_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool AtomsBlink_RenderHTML(const char* html_source, void* bwe_window_ptr, int viewport_w, int viewport_h);

#ifdef __cplusplus
}
#endif

#endif // ATOMS_BLINK_ADAPTER_H_
