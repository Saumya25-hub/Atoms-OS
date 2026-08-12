#include "usb_forensic_center.h"

void usb_forensic_log_stage(uint32_t stage, bool success, uint32_t code, uint32_t residual) {
    if (stage >= 20) return;
    ForensicTimelinePanel* timeline = &g_forensic_center.timeline;
    timeline->current_stage = stage;
    timeline->stage_passed[stage] = success;
    timeline->completion_code = code;
    timeline->residual_length = residual;
}

void usb_forensic_increment_retry(void) {
    g_forensic_center.timeline.retry_count++;
}
