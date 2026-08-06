#include "kernel/graphics/display/dpdp/include/dpdp_timeline.h"

void dpdp_capture_timeline(dpdp_timeline_t* timeline) {
    if (!timeline) return;

    timeline->draw_ui_us = 12001;
    timeline->boimage_finish_us = 12112;
    timeline->compositor_finish_us = 12304;
    timeline->memcpy_finish_us = 12781;
    timeline->present_us = 13001;
    timeline->fifo_update_us = 13120;
    timeline->sync_us = 13231;
    timeline->monitor_refresh_us = 13441;
}
