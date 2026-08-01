#include "explorer_breadcrumb.h"
#include "kernel/core/lib/include/string.h"

void explorer_breadcrumb_init(ExplorerBreadcrumb* bc) {
    if (!bc) return;
    memset(bc, 0, sizeof(ExplorerBreadcrumb));
}

void explorer_breadcrumb_parse(ExplorerBreadcrumb* bc, const char* path) {
    if (!bc || !path) return;
    explorer_breadcrumb_init(bc);
    
    // Always start with "This PC"
    BreadcrumbSegment* seg = &bc->segments[bc->segment_count++];
    strcpy(seg->label, "This PC");
    strcpy(seg->full_path, "/");
    
    if (strcmp(path, "/") == 0) {
        strcpy(bc->formatted_str, "This PC");
        return;
    }
    
    strcpy(bc->formatted_str, "This PC");
    
    char temp[256];
    strcpy(temp, path);
    char current_accum[256] = "";
    
    int start = 0;
    int len = strlen(temp);
    for (int i = 0; i <= len; i++) {
        if (temp[i] == '/' || temp[i] == '\0') {
            if (i > start) {
                char part[64];
                int p_len = i - start;
                if (p_len >= (int)sizeof(part)) p_len = sizeof(part) - 1;
                strncpy(part, &temp[start], p_len);
                part[p_len] = '\0';
                
                if (strlen(current_accum) == 0 || strcmp(current_accum, "/") == 0) {
                    strcpy(current_accum, "/");
                    strcat(current_accum, part);
                } else {
                    strcat(current_accum, "/");
                    strcat(current_accum, part);
                }
                
                if (bc->segment_count < BSEC_BREADCRUMB_MAX_SEGMENTS) {
                    BreadcrumbSegment* s = &bc->segments[bc->segment_count++];
                    strcpy(s->label, part);
                    strcpy(s->full_path, current_accum);
                    
                    strcat(bc->formatted_str, " > ");
                    strcat(bc->formatted_str, part);
                }
            }
            start = i + 1;
        }
    }
}

const char* explorer_breadcrumb_hit_test(const ExplorerBreadcrumb* bc, int32_t local_x) {
    if (!bc || bc->segment_count == 0) return "/";
    (void)local_x;
    return bc->segments[bc->segment_count - 1].full_path;
}
