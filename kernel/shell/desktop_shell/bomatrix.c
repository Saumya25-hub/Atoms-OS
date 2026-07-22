#include "bomatrix.h"
#include "kernel/core/lib/include/string.h"

extern uint32_t g_kernel_screen_width;
extern uint32_t g_kernel_screen_height;
extern void display_print(const char* s);

static BOMatrixMetrics s_metrics;
static BOMatrixCell    s_cells[BOMATRIX_MAX_CELLS];
static uint32_t        s_cell_count = 0;

void bomatrix_init(void) {
    memset(&s_metrics, 0, sizeof(s_metrics));
    memset(s_cells, 0, sizeof(s_cells));
    s_cell_count = 0;

    uint32_t sw = g_kernel_screen_width > 0 ? g_kernel_screen_width : 1280;
    uint32_t sh = g_kernel_screen_height > 0 ? g_kernel_screen_height : 720;
    
    bomatrix_update_metrics(sw, sh);
    display_print("[BOMATRIX] Desktop Matrix Layout Engine Initialized.\n");
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

    s_metrics.max_cols = (s_metrics.workspace_w - s_metrics.margin_x) / s_metrics.cell_step_x;
    if (s_metrics.max_cols < 1) s_metrics.max_cols = 1;
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
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].col == col && s_cells[i].row == row) {
            return &s_cells[i];
        }
    }
    return NULL;
}

BOMatrixCell* bomatrix_find_cell_by_window(uint32_t win_id) {
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].win_id == win_id) {
            return &s_cells[i];
        }
    }
    return NULL;
}

bool bomatrix_get_next_free_cell(int32_t* out_col, int32_t* out_row) {
    // Column-major order: fill column 0 vertically, then column 1, etc.
    for (int32_t c = 0; c < s_metrics.max_cols; c++) {
        for (int32_t r = 0; r < s_metrics.max_rows; r++) {
            if (!bomatrix_find_cell_by_pos(c, r)) {
                if (out_col) *out_col = c;
                if (out_row) *out_row = r;
                return true;
            }
        }
    }
    return false;
}

bool bomatrix_register_icon(uint32_t win_id, uint32_t app_id, int32_t col, int32_t row) {
    BOMatrixCell* existing = bomatrix_find_cell_by_window(win_id);
    if (!existing) {
        if (s_cell_count >= BOMATRIX_MAX_CELLS) return false;
        existing = &s_cells[s_cell_count++];
    }

    existing->win_id = win_id;
    existing->app_id = app_id;
    existing->col = col;
    existing->row = row;
    existing->occupied = true;
    bomatrix_grid_to_pixel(col, row, &existing->x, &existing->y);

    return true;
}

void bomatrix_unregister_icon(uint32_t win_id) {
    for (uint32_t i = 0; i < s_cell_count; i++) {
        if (s_cells[i].occupied && s_cells[i].win_id == win_id) {
            s_cells[i].occupied = false;
            s_cells[i].win_id = 0;
            break;
        }
    }
}

void bomatrix_snap_icon(uint32_t win_id, int32_t raw_x, int32_t raw_y, int32_t* out_snapped_x, int32_t* out_snapped_y) {
    BOMatrixCell* self_cell = bomatrix_find_cell_by_window(win_id);

    int32_t target_col = 0, target_row = 0;
    bomatrix_pixel_to_grid(raw_x, raw_y, &target_col, &target_row);

    BOMatrixCell* occupant = bomatrix_find_cell_by_pos(target_col, target_row);

    if (occupant && occupant->win_id != win_id) {
        // Deterministic Swap Collision Resolution
        if (self_cell) {
            int32_t old_col = self_cell->col;
            int32_t old_row = self_cell->row;

            occupant->col = old_col;
            occupant->row = old_row;
            bomatrix_grid_to_pixel(old_col, old_row, &occupant->x, &occupant->y);
            BOS_SetBounds(occupant->win_id, occupant->x, occupant->y, s_metrics.cell_w, s_metrics.cell_h);

            self_cell->col = target_col;
            self_cell->row = target_row;
            bomatrix_grid_to_pixel(target_col, target_row, &self_cell->x, &self_cell->y);
        } else {
            // Find next free cell for self if self had no previous record
            int32_t free_col = 0, free_row = 0;
            if (bomatrix_get_next_free_cell(&free_col, &free_row)) {
                target_col = free_col;
                target_row = free_row;
            }
        }
    } else if (self_cell) {
        self_cell->col = target_col;
        self_cell->row = target_row;
        bomatrix_grid_to_pixel(target_col, target_row, &self_cell->x, &self_cell->y);
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
