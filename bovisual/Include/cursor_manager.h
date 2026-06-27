#ifndef BV_CURSOR_MANAGER_H
#define BV_CURSOR_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

void BVCursor_Init(uint32_t screen_width, uint32_t screen_height);
void BVCursor_RestoreBG(void);
void BVCursor_Draw(int32_t cx, int32_t cy);

#endif
