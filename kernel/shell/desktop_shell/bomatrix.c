#include "bomatrix.h"
#include "kernel/core/lib/include/string.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern void display_print(const char* s);

static BOMatrixMetrics s_metrics;
static BOMatrixCell    s_cells[BOMATRIX_MAX_CELLS];
static int32_t         s_spatial_grid[BOMATRIX_MAX_COLS][BOMATRIX_MAX_ROWS];
static uint32_t        s_cell_count = 0;

void bomatrix_init(void) {
    memset(&s_metrics, 0, sizeof(s_metrics));
    memset(s_cells, 0, sizeof(s_cells));
    memset(s_spatial_grid, -1, sizeof(s_spatial_grid));
    s_cell_count = 0;

    uint32_t sw = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1280;
    uint32_t sh = g_kernel_screen_height > 0 ? g_kernel_screen_height : 720;
    
    bomatrix_update_metrics(sw, sh);
    display_print("[BOMATRIX V2] Desktop Matrix Layout Engine V2 (O(1) Spatial Map) Initialized.\n");
}

void bomatrix_update_metrics(uint32_t screen_w, uint32_t screen_h) {
    s_metrics.screen_w = screen_w;
    s_metrics.screen_h = screen_h;
    s_metrics.workspace_x = 0;
    s_metrics.workspace_y = 0;
    s_metrics.workspace_w = screen_w;
    s_metrics.workspace_h = (int32_t)screen_h - BOMATRIX_TASKBAR_HEIGHT;

    s_metrics.cell_w = BOMATRIX_CELL_WIDTH;
    s_metrics.cell_h = BOMATRIX_CELL_HEIGHT;
    s_metrics.cell_step_x = BOMATRIX_STEP_X;
    s_metrics.cell_step_y = BOMATRIX_STEP_Y;
    s_metrics.margin_x = BOMATRIX_MARGIN_X;
    s_metrics.margin_y = BOMATRIX_MARGIN_Y;

    s_metrics.max_rows = (s_metrics.workspace_h - s_metrics.margin_y) / s_metrics.cell_step_y;
    if (s_metrics.max_rows < 1) s_metrics.max_rows = 1;
    if (s_metrics.max_rows > BOMATRIX_MAX_ROWS) s_metrics.max_rows = BOMATRIX_MAX_ROWS;

    s_metrics.max_cols = (s_metrics.workspace_w - s_metrics.margin_x) / s_metrics.cell_step_x;
    if (s_metrics.max_cols < 1) s_metrics.max_cols = 1;
    if (s_metrics.max_cols > BOMATRIX_MAX_COLS) s_metrics.max_cols = BOMATRIX_MAX_COLS;
}

const BOMatrixMetrics* bomatrix_get_metrics(void) {
    return &s_metrics;
}

void bomatrix_grid_to_pixel(int32_t col, int32_t row, int32_t* out_x, int32_t* out_y) {
    if (out_x) *out_x = s_metrics.margin_x + col * s_metrics.cell_step_x;
    if (out_y) *out_y = s_metrics.margin_y + row * s_metrics.cell_step_y;
}

void bomatrix_pixel_to_grid(int32_t x, int32_t y, int32_t* out_col, int32_t* out_row) {
    int32_t col = (x - s_metrics.margin_x + s_metrics.cell_step_x / 2) / s_metrics.cell_step_x;
    int32_t row = (y - s_metrics.margin_y + s_metrics.cell_step_y / 2) / s_metrics.cell_step_y;

    if (col < 0) col = 0;
    if (col >= s_metrics.max_cols) col = s_metrics.max_cols - 1;

    if (row < 0) row = 0;
    if (row >= s_metrics.max_rows) row = s_metrics.max_rows - 1;

    if (out_col) *out_col = col;
    if (out_row) *out_row = row;
}

BOMatrixCell* bomatrix_find_cell_by_pos(int32_t col, int32_t row) {
    if (col < 0 || col >= s_metrics.max_cols || row < 0 || row >= s_metrics.max_rows) {
        return NULL;
    }
    int32_t idx = s_spatial_grid[col][row];
    if (idx >= 0 && idx < BOMATRIX_MAX_CELLS && s_cells[idx].occupied) {
        return &s_cells[idx];
    }
    return NULL;
}

BOMatrixCell* bomatrix_find_cell_by_window(uint32_t win_id) {
    if (win_id == 0) return NULL;
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].win_id == win_id) {
            return &s_cells[i];
        }
    }
    return NULL;
}

bool bomatrix_get_next_free_cell(int32_t* out_col, int32_t* out_row) {
    // Fast O(1) cell check
    for (int32_t c = 0; c < s_metrics.max_cols; c++) {
        for (int32_t r = 0; r < s_metrics.max_rows; r++) {
            if (s_spatial_grid[c][r] == -1) {
                if (out_col) *out_col = c;
                if (out_row) *out_row = r;
                return true;
            }
        }
    }
    return false;
}

bool bomatrix_register_icon(uint32_t win_id, uint32_t app_id, int32_t col, int32_t row) {
    if (col < 0 || row < 0) {
        if (!bomatrix_get_next_free_cell(&col, &row)) {
            col = 0;
            row = 0;
        }
    }

    BOMatrixCell* existing = bomatrix_find_cell_by_window(win_id);
    int32_t cell_idx = -1;

    if (!existing) {
        if (s_cell_count >= BOMATRIX_MAX_CELLS) return false;
        cell_idx = (int32_t)s_cell_count++;
        existing = &s_cells[cell_idx];
    } else {
        cell_idx = (int32_t)(existing - s_cells);
    }

    // Clear old spatial mapping
    if (existing->col >= 0 && existing->col < BOMATRIX_MAX_COLS &&
        existing->row >= 0 && existing->row < BOMATRIX_MAX_ROWS) {
        if (s_spatial_grid[existing->col][existing->row] == cell_idx) {
            s_spatial_grid[existing->col][existing->row] = -1;
        }
    }

    existing->win_id = win_id;
    existing->app_id = app_id;
    existing->col = col;
    existing->row = row;
    existing->occupied = true;
    bomatrix_grid_to_pixel(col, row, &existing->x, &existing->y);

    if (col >= 0 && col < BOMATRIX_MAX_COLS && row >= 0 && row < BOMATRIX_MAX_ROWS) {
        s_spatial_grid[col][row] = cell_idx;
    }

    return true;
}

void bomatrix_unregister_icon(uint32_t win_id) {
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].win_id == win_id) {
            if (s_cells[i].col >= 0 && s_cells[i].col < BOMATRIX_MAX_COLS &&
                s_cells[i].row >= 0 && s_cells[i].row < BOMATRIX_MAX_ROWS) {
                s_spatial_grid[s_cells[i].col][s_cells[i].row] = -1;
            }
            s_cells[i].occupied = false;
            s_cells[i].win_id = 0;
            break;
        }
    }
}

void bomatrix_snap_icon(uint32_t win_id, int32_t raw_x, int32_t raw_y, int32_t* out_snapped_x, int32_t* out_snapped_y) {
    BOMatrixCell* self_cell = bomatrix_find_cell_by_window(win_id);
    int32_t self_idx = self_cell ? (int32_t)(self_cell - s_cells) : -1;

    int32_t target_col = 0, target_row = 0;
    bomatrix_pixel_to_grid(raw_x, raw_y, &target_col, &target_row);

    BOMatrixCell* occupant = bomatrix_find_cell_by_pos(target_col, target_row);

    if (occupant && occupant->win_id != win_id) {
        // Swap Collision Resolution
        if (self_cell) {
            int32_t old_col = self_cell->col;
            int32_t old_row = self_cell->row;
            int32_t occupant_idx = (int32_t)(occupant - s_cells);

            occupant->col = old_col;
            occupant->row = old_row;
            bomatrix_grid_to_pixel(old_col, old_row, &occupant->x, &occupant->y);
            s_spatial_grid[old_col][old_row] = occupant_idx;
            BOS_SetBounds(occupant->win_id, occupant->x, occupant->y, s_metrics.cell_w, s_metrics.cell_h);

            self_cell->col = target_col;
            self_cell->row = target_row;
            bomatrix_grid_to_pixel(target_col, target_row, &self_cell->x, &self_cell->y);
            s_spatial_grid[target_col][target_row] = self_idx;
        }
    } else if (self_cell) {
        if (self_cell->col >= 0 && self_cell->col < BOMATRIX_MAX_COLS &&
            self_cell->row >= 0 && self_cell->row < BOMATRIX_MAX_ROWS) {
            s_spatial_grid[self_cell->col][self_cell->row] = -1;
        }
        self_cell->col = target_col;
        self_cell->row = target_row;
        bomatrix_grid_to_pixel(target_col, target_row, &self_cell->x, &self_cell->y);
        s_spatial_grid[target_col][target_row] = self_idx;
    }

    int32_t final_x = 0, final_y = 0;
    bomatrix_grid_to_pixel(target_col, target_row, &final_x, &final_y);

    if (out_snapped_x) *out_snapped_x = final_x;
    if (out_snapped_y) *out_snapped_y = final_y;
}

void bomatrix_rearrange_all(void) {
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].win_id != 0) {
            bomatrix_grid_to_pixel(s_cells[i].col, s_cells[i].row, &s_cells[i].x, &s_cells[i].y);
            BOS_SetBounds(s_cells[i].win_id, s_cells[i].x, s_cells[i].y, s_metrics.cell_w, s_metrics.cell_h);
        }
    }
}
