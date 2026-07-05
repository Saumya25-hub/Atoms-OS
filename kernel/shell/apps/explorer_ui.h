#ifndef BOS_EXPLORER_UI_H
#define BOS_EXPLORER_UI_H

#include "explorer.h"

int explorer_ui_init(ExplorerContext* ctx);
void explorer_ui_update_pathbar(ExplorerContext* ctx, const char* path);
void explorer_ui_update_status(ExplorerContext* ctx, int item_count);

#endif // BOS_EXPLORER_UI_H
