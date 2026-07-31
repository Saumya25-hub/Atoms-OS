#ifndef BWE_LAYOUT_H
#define BWE_LAYOUT_H

#include "bwe_types.h"
#include <stdbool.h>
#include <stdint.h>

typedef uint32_t bwe_error_t;

// Container Layout Modes
typedef enum {
    BWE_LAYOUT_ABSOLUTE = 0,
    BWE_LAYOUT_DOCK,
    BWE_LAYOUT_STACK,
    BWE_LAYOUT_GRID,
    BWE_LAYOUT_SPLIT,
    BWE_LAYOUT_SCROLL
} BWELayoutMode;

// Docking Positions
typedef enum {
    BWE_DOCK_NONE = 0,
    BWE_DOCK_TOP,
    BWE_DOCK_BOTTOM,
    BWE_DOCK_LEFT,
    BWE_DOCK_RIGHT,
    BWE_DOCK_FILL
} BWEDockPosition;

// Stacking Orientation
typedef enum {
    BWE_ORIENT_VERTICAL = 0,
    BWE_ORIENT_HORIZONTAL
} BWEOrientation;

// Size Length Units
typedef enum {
    BWE_UNIT_PIXEL = 0,
    BWE_UNIT_AUTO,
    BWE_UNIT_STAR,
    BWE_UNIT_PERCENT
} BWSizeUnit;

typedef struct {
    BWSizeUnit unit;
    float value;
} BWLength;

// Helper Construct Macros
#define BWE_PIXELS(px) ((BWLength){ .unit = BWE_UNIT_PIXEL, .value = (float)(px) })
#define BWE_AUTO()     ((BWLength){ .unit = BWE_UNIT_AUTO,  .value = 0.0f })
#define BWE_STAR(val)  ((BWLength){ .unit = BWE_UNIT_STAR,  .value = (float)(val) })
#define BWE_PCT(pct)   ((BWLength){ .unit = BWE_UNIT_PERCENT, .value = (float)(pct) })

// Grid Track Definition
typedef struct {
    BWLength length;
    int32_t computed_size;
} BWETrackDef;

#define BWE_MAX_GRID_ROWS 16
#define BWE_MAX_GRID_COLS 16

typedef struct {
    BWELayoutMode mode;
    BWEDockPosition dock_pos;
    BWEOrientation orientation;
    int32_t spacing;
    
    // Grid Properties
    BWETrackDef rows[BWE_MAX_GRID_ROWS];
    uint32_t row_count;
    BWETrackDef cols[BWE_MAX_GRID_COLS];
    uint32_t col_count;
    uint32_t grid_row;
    uint32_t grid_col;
    uint32_t grid_row_span;
    uint32_t grid_col_span;

    // Desired Size output from Measure Pass
    BWE_Rect desired_size;
    bool layout_dirty;
} BWELayoutProps;

// Two-Pass Layout Lifecycle
void BWE_MeasureWindow(uint32_t window_id, int32_t avail_w, int32_t avail_h);
void BWE_ArrangeWindow(uint32_t window_id, const BWE_Rect* final_rect);
void BWE_InvalidateLayout(uint32_t window_id);

// Container Creation APIs
bwe_error_t BOS_CreateDockPanel(uint32_t parent_id, uint32_t* out_id);
bwe_error_t BOS_CreateStackPanel(uint32_t parent_id, BWEOrientation orientation, int32_t spacing, uint32_t* out_id);
bwe_error_t BOS_CreateGridPanel(uint32_t parent_id, uint32_t* out_id);
bwe_error_t BOS_CreateScrollViewer(uint32_t parent_id, uint32_t* out_id);

// Configuration & Positioning APIs
void BWE_SetDockPosition(uint32_t window_id, BWEDockPosition dock);
void BWE_Grid_AddRow(uint32_t grid_id, BWLength height);
void BWE_Grid_AddColumn(uint32_t grid_id, BWLength width);
void BWE_Grid_SetCell(uint32_t window_id, uint32_t row, uint32_t col);
void BWE_Grid_SetCellSpan(uint32_t window_id, uint32_t row_span, uint32_t col_span);

// Legacy Dispatches & Maximize Operations
void BWE_UpdateLayout(uint32_t parent_id);
void BWE_SetAnchorMode(uint32_t window_id, uint8_t anchor_flags);
void BWE_WindowMaximize(uint32_t window_id);
void BWE_WindowRestore(uint32_t window_id);
bool BWE_WindowIsMaximized(uint32_t window_id);

#endif // BWE_LAYOUT_H
