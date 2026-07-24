#ifndef ABE_CORE_H
#define ABE_CORE_H

#include "../api/abe_api.h"
#include "kernel/browser/engine/html/html_document.h"

#ifdef __cplusplus
extern "C" {
#endif

// Master Engine Lifecycle Container
struct ABE_Engine {
    uint32_t      engine_id;
    bool          is_active;
    char          current_url[256];
    HTMLDocument* active_document;
    uint32_t      dom_node_count;
    uint32_t      parse_time_ms;
    uint32_t      layout_time_ms;
    uint32_t      paint_time_ms;
    uint32_t      frame_count;
};

void ABE_Core_Init(void);

#ifdef __cplusplus
}
#endif

#endif // ABE_CORE_H
