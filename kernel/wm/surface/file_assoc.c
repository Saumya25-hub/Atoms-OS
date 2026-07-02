#include "file_assoc.h"
#include "kernel/drivers/display/display.h"
#include "kernel/core/lib/include/string.h"

static BOS_FileAssocEntry assoc_table[BOS_MAX_FILE_ASSOCS];
static int assoc_count = 0;

// Case-insensitive compare for extensions
static int ext_cmp(const char* a, const char* b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 1;
        a++; b++;
    }
    return (*a != *b) ? 1 : 0;
}

static void fa_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

void BOS_FileAssoc_Init(void) {
    for (int i = 0; i < BOS_MAX_FILE_ASSOCS; i++) {
        assoc_table[i].extension[0] = '\0';
        assoc_table[i].app_name[0] = '\0';
        assoc_table[i].open_func = 0;
    }
    assoc_count = 0;
    display_print("[FASSOC] File Association Engine Initialized\n");
    extern void bosx_loader_open(const char* filepath);
    BOS_RegisterFileAssociation("BOSX", "BOSX Loader", bosx_loader_open);
}

int BOS_RegisterFileAssociation(const char* extension, const char* app_name, BOS_FileOpenFunc open_func) {
    if (!extension) { display_print("[FASSOC] Error: NULL extension\n"); return -1; }
    if (!open_func) { display_print("[FASSOC] Error: NULL open_func for "); display_print(extension); display_print("\n"); return -1; }
    if (assoc_count >= BOS_MAX_FILE_ASSOCS) { display_print("[FASSOC] Error: max assoc reached\n"); return -1; }
    
    fa_strcpy(assoc_table[assoc_count].extension, extension, BOS_EXT_LEN);
    fa_strcpy(assoc_table[assoc_count].app_name, app_name ? app_name : "Unknown", 32);
    assoc_table[assoc_count].open_func = open_func;
    assoc_count++;
    
    display_print("[FASSOC] Registered: .");
    display_print(extension);
    display_print(" -> ");
    display_print(app_name);
    display_print("\n");
    
    return 0;
}

// Extract extension from filename (after last '.')
static const char* get_extension(const char* filename) {
    const char* dot = 0;
    while (*filename) {
        if (*filename == '.') dot = filename + 1;
        filename++;
    }
    return dot;
}

BOS_FileOpenFunc BOS_GetAssociatedApp(const char* filename) {
    const char* ext = get_extension(filename);
    if (!ext) return 0;
    
    for (int i = 0; i < assoc_count; i++) {
        if (ext_cmp(ext, assoc_table[i].extension) == 0) {
            return assoc_table[i].open_func;
        }
    }
    return 0;
}

int BOS_OpenFile(const char* filepath) {
    if (!filepath) return -1;
    
    // Extract just the filename from path for extension matching
    const char* name = filepath;
    const char* p = filepath;
    while (*p) {
        if (*p == '/') name = p + 1;
        p++;
    }
    
    BOS_FileOpenFunc open_func = BOS_GetAssociatedApp(name);
    if (open_func) {
        open_func(filepath);
        return 0;
    }
    
    display_print("[FASSOC] No association for: ");
    display_print(name);
    display_print("\n");
    return -1;
}
