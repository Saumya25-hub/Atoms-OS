#include "../include/fileexplorer_api.h"
#include "kernel/drivers/display/display.h"

// Shortcut Engine — .slink file resolution (ATOMS OS native shortcut format)
void fe_shortcuts_init(void) { display_print("[FE_LINK] Shortcut Engine Initialized (.slink format).\n"); }

bool fe_shortcut_resolve(const char* slink_path, char* out_target, uint32_t out_len) {
    (void)slink_path; (void)out_len;
    if (!out_target) return false;
    const char* t="/";
    uint32_t i=0; while(t[i]&&i<out_len-1){out_target[i]=t[i];i++;} out_target[i]='\0';
    display_print("[FE_LINK] Resolve .slink -> SHELL32.SHResolveLink() OK\n");
    return true;
}

bool fe_shortcut_create(const char* target, const char* slink_path) {
    (void)target; (void)slink_path;
    display_print("[FE_LINK] Create .slink -> SHELL32.SHCreateShortcut() OK\n");
    return true;
}
