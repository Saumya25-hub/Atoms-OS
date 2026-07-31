#include "include/bos_manifest.h"
#include "kernel/core/lib/include/string.h"
#include "kernel/core/memory/heap/include/heap.h"

extern void display_print(const char* s);

static const char* k_strchr(const char* str, char ch) {
    if (!str) return NULL;
    while (*str) {
        if (*str == ch) return str;
        str++;
    }
    return (ch == '\0') ? str : NULL;
}

static const char* parse_json_string_field(const char* json, const char* key, char* out_val, size_t max_len) {
    if (!json || !key || !out_val) return NULL;
    
    char key_pattern[64];
    strcpy(key_pattern, "\"");
    strcat(key_pattern, key);
    strcat(key_pattern, "\"");

    const char* pos = strstr(json, key_pattern);
    if (!pos) return NULL;

    const char* colon = k_strchr(pos, ':');
    if (!colon) return NULL;

    const char* start_quote = k_strchr(colon, '"');
    if (!start_quote) return NULL;

    const char* end_quote = k_strchr(start_quote + 1, '"');
    if (!end_quote) return NULL;

    size_t len = (size_t)(end_quote - start_quote - 1);
    if (len >= max_len) len = max_len - 1;

    strncpy(out_val, start_quote + 1, len);
    out_val[len] = '\0';
    return out_val;
}

BOS_Manifest* bos_manifest_parse(const char* json_content) {
    display_print("[BOSX MANIFEST] Parsing manifest.json...\n");

    BOS_Manifest* m = (BOS_Manifest*)kmalloc(sizeof(BOS_Manifest));
    if (!m) return NULL;
    memset(m, 0, sizeof(BOS_Manifest));

    if (json_content) {
        parse_json_string_field(json_content, "name", m->name, sizeof(m->name));
        parse_json_string_field(json_content, "identifier", m->identifier, sizeof(m->identifier));
        parse_json_string_field(json_content, "version", m->version, sizeof(m->version));
        parse_json_string_field(json_content, "author", m->author, sizeof(m->author));
        parse_json_string_field(json_content, "entry_point", m->entry_point, sizeof(m->entry_point));
        parse_json_string_field(json_content, "target_platform", m->target_platform, sizeof(m->target_platform));
    }

    if (strlen(m->name) == 0) strcpy(m->name, "BOS Application");
    if (strlen(m->entry_point) == 0) strcpy(m->entry_point, "app.elf");
    m->window_width = 800;
    m->window_height = 600;
    m->is_parsed = true;

    display_print("[BOSX MANIFEST] Parsed App: ");
    display_print(m->name);
    display_print(" (Entry: ");
    display_print(m->entry_point);
    display_print(")\n");

    return m;
}

void bos_manifest_free(BOS_Manifest* manifest) {
    if (manifest) kfree(manifest);
}
