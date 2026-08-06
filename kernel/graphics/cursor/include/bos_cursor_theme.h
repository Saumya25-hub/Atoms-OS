/**
 * @file bos_cursor_theme.h
 * @brief Cursor Theme Engine supporting 15 standard cursor types without reboot
 */

#ifndef BOS_CURSOR_THEME_H
#define BOS_CURSOR_THEME_H

#include "bos_cursor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[64];
    bce_cursor_t* cursors[BCE_CURSOR_TYPE_COUNT];
} bce_theme_t;

bce_error_t bos_cursor_theme_init(void);
void        bos_cursor_theme_shutdown(void);
bce_error_t bos_cursor_theme_load(const char* theme_name);
bce_cursor_t* bos_cursor_theme_get_type(bce_cursor_type_t type);
const char* bos_cursor_theme_get_current_name(void);

#ifdef __cplusplus
}
#endif

#endif /* BOS_CURSOR_THEME_H */
