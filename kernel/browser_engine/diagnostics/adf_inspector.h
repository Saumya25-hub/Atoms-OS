#ifndef ADF_INSPECTOR_H
#define ADF_INSPECTOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool show_layout_boxes;
    bool show_repaint_regions;
    bool show_dirty_rects;
    bool show_layer_borders;
    bool show_clip_regions;
    bool show_fps_hud;
    bool show_gpu_uploads;
} ABE_InspectorOverlayFlags;

void ABE_DebugOverlayEnable(void);
void ABE_DebugOverlayDisable(void);
bool ABE_DebugOverlayIsActive(void);
ABE_InspectorOverlayFlags ABE_DebugGetOverlayFlags(void);
bool ABE_DebugProcessCommand(const char* cmd_str);

#ifdef __cplusplus
}
#endif

#endif // ADF_INSPECTOR_H
