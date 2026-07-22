#ifndef BOMATRIX_H
#define BOMATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/wm/bwe/include/bwe.h"

#define BOMATRIX_CELL_WIDTH       110
#define BOMATRIX_CELL_HEIGHT      78
#define BOMATRIX_STEP_X           120
#define BOMATRIX_STEP_Y           85
#define BOMATRIX_MARGIN_X         15
#define BOMATRIX_MARGIN_Y         15
#define BOMATRIX_TASKBAR_HEIGHT   48
#define BOMATRIX_MAX_CELLS        64

typedef struct {
    int32_t  col;
    int32_t  row;
    int32_t  x;
    int32_t  y;
    uint32_t win_id;
    uint32_t app_id;
    bool     occupied;
} BOMatrixCell;

typedef struct {
    int32_t screen_w;
    int32_t screen_h;
    int32_t workspace_x;
    int32_t workspace_y;
    int32_t workspace_w;
    int32_t workspace_h;
    int32_t cell_w;
    int32_t cell_h;
    int32_t cell_step_x;
    int32_t cell_step_y;
    int32_t margin_x;
    int32_t margin_y;
    int32_t max_cols;
    int32_t max_rows;
} BOMatrixMetrics;

// Subsystem API
void bomatrix_init(void);
void bomatrix_update_metrics(uint32_t screen_w, uint32_t screen_h);
const BOMatrixMetrics* bomatrix_get_metrics(void);

void bomatrix_grid_to_pixel(int32_t col, int32_t row, int32_t* out_x, int32_t* out_y);
void bomatrix_pixel_to_grid(int32_t x, int32_t y, int32_t* out_col, int32_t* out_row);

bool bomatrix_get_next_free_cell(int32_t* out_col, int32_t* out_row);
bool bomatrix_register_icon(uint32_t win_id, uint32_t app_id, int32_t col, int32_t row);
void bomatrix_unregister_icon(uint32_t win_id);

void bomatrix_snap_icon(uint32_t win_id, int32_t raw_x, int32_t raw_y, int32_t* out_snapped_x, int32_t* out_snapped_y);
BOMatrixCell* bomatrix_find_cell_by_window(uint32_t win_id);
BOMatrixCell* bomatrix_find_cell_by_pos(int32_t col, int32_t row);

void bomatrix_rearrange_all(void);

#endif // BOMATRIX_H
