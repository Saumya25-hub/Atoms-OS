/**
 * @file agdte_present_timeline.c
 * @brief ATOMEGearDisplayTrainEngine (AGDTE) Presentation Timeline Engine
 * @status Phase 4 Display Timing Optimization Layer
 *
 * @section PURPOSE
 * Records every milestone of a presentation frame (`submit`, `queue`, `schedule`,
 * `present`, `display`, `completion`) with microsecond timestamps. Provides the
 * deterministic debugging foundation for analyzing latency and presentation stepping.
 */

#include "../include/agdte.h"

#define AGDTE_TIMELINE_CAPACITY 64

static AGDTE_TimelineEntry s_timeline[AGDTE_TIMELINE_CAPACITY];
static uint32_t s_timeline_head = 0;
static uint32_t s_timeline_count = 0;

AGDTE_Error AGDTE_Timeline_Init(void) {
    for (uint32_t i = 0; i < AGDTE_TIMELINE_CAPACITY; i++) {
        s_timeline[i].frame_id = 0;
        s_timeline[i].submit_timestamp_us = 0;
        s_timeline[i].queue_timestamp_us = 0;
        s_timeline[i].schedule_timestamp_us = 0;
        s_timeline[i].present_timestamp_us = 0;
        s_timeline[i].display_timestamp_us = 0;
        s_timeline[i].completion_timestamp_us = 0;
        s_timeline[i].display_id = 0;
        s_timeline[i].layer = AGDTE_LAYER_DESKTOP;
        s_timeline[i].valid = false;
    }
    s_timeline_head = 0;
    s_timeline_count = 0;
    return AGDTE_OK;
}

AGDTE_TimelineEntry* AGDTE_Timeline_GetEntry(uint64_t frame_id) {
    if (s_timeline_count == 0) {
        return 0;
    }
    for (uint32_t i = 0; i < s_timeline_count; i++) {
        uint32_t idx = (s_timeline_head + AGDTE_TIMELINE_CAPACITY - 1 - i) % AGDTE_TIMELINE_CAPACITY;
        if (s_timeline[idx].valid && s_timeline[idx].frame_id == frame_id) {
            return &s_timeline[idx];
        }
    }
    return 0;
}

AGDTE_TimelineEntry* AGDTE_Timeline_GetLatest(void) {
    if (s_timeline_count == 0) {
        return 0;
    }
    uint32_t idx = (s_timeline_head + AGDTE_TIMELINE_CAPACITY - 1) % AGDTE_TIMELINE_CAPACITY;
    if (s_timeline[idx].valid) {
        return &s_timeline[idx];
    }
    return 0;
}

AGDTE_Error AGDTE_Timeline_RecordSubmit(uint64_t frame_id, uint32_t display_id, AGDTE_SurfaceLayer layer, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (!entry) {
        entry = &s_timeline[s_timeline_head];
        s_timeline_head = (s_timeline_head + 1) % AGDTE_TIMELINE_CAPACITY;
        if (s_timeline_count < AGDTE_TIMELINE_CAPACITY) {
            s_timeline_count++;
        }
        entry->frame_id = frame_id;
        entry->display_id = display_id;
        entry->layer = layer;
        entry->valid = true;
        entry->submit_timestamp_us = timestamp_us;
        entry->queue_timestamp_us = 0;
        entry->schedule_timestamp_us = 0;
        entry->present_timestamp_us = 0;
        entry->display_timestamp_us = 0;
        entry->completion_timestamp_us = 0;
    } else {
        entry->submit_timestamp_us = timestamp_us;
    }
    return AGDTE_OK;
}

AGDTE_Error AGDTE_Timeline_RecordQueue(uint64_t frame_id, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (entry) {
        entry->queue_timestamp_us = timestamp_us;
        return AGDTE_OK;
    }
    return AGDTE_ERR_INVALID_STATE;
}

AGDTE_Error AGDTE_Timeline_RecordSchedule(uint64_t frame_id, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (entry) {
        entry->schedule_timestamp_us = timestamp_us;
        return AGDTE_OK;
    }
    return AGDTE_ERR_INVALID_STATE;
}

AGDTE_Error AGDTE_Timeline_RecordPresent(uint64_t frame_id, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (entry) {
        entry->present_timestamp_us = timestamp_us;
        return AGDTE_OK;
    }
    return AGDTE_ERR_INVALID_STATE;
}

AGDTE_Error AGDTE_Timeline_RecordDisplay(uint64_t frame_id, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (entry) {
        entry->display_timestamp_us = timestamp_us;
        return AGDTE_OK;
    }
    return AGDTE_ERR_INVALID_STATE;
}

AGDTE_Error AGDTE_Timeline_RecordCompletion(uint64_t frame_id, uint64_t timestamp_us) {
    AGDTE_TimelineEntry* entry = AGDTE_Timeline_GetEntry(frame_id);
    if (entry) {
        entry->completion_timestamp_us = timestamp_us;
        return AGDTE_OK;
    }
    return AGDTE_ERR_INVALID_STATE;
}
