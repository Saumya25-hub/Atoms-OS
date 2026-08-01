#ifndef BSEC_EXPLORER_BREADCRUMB_H
#define BSEC_EXPLORER_BREADCRUMB_H

#include <stdint.h>
#include <stdbool.h>

#define BSEC_BREADCRUMB_MAX_SEGMENTS 16

typedef struct {
    char    label[64];
    char    full_path[256];
    int32_t click_x_start;
    int32_t click_x_end;
} BreadcrumbSegment;

typedef struct {
    BreadcrumbSegment segments[BSEC_BREADCRUMB_MAX_SEGMENTS];
    uint32_t          segment_count;
    char              formatted_str[256];
} ExplorerBreadcrumb;

void explorer_breadcrumb_init(ExplorerBreadcrumb* bc);
void explorer_breadcrumb_parse(ExplorerBreadcrumb* bc, const char* path);
const char* explorer_breadcrumb_hit_test(const ExplorerBreadcrumb* bc, int32_t local_x);

#endif // BSEC_EXPLORER_BREADCRUMB_H
