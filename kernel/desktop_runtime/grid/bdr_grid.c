#include "../include/bdr_api.h"

int32_t BDR_SnapToGrid(int32_t raw_x, int32_t raw_y, int32_t* out_grid_x, int32_t* out_grid_y) {
    if (!out_grid_x || !out_grid_y) return -1;

    int32_t gx = raw_x / BDR_GRID_CELL_WIDTH;
    int32_t gy = raw_y / BDR_GRID_CELL_HEIGHT;

    if (gx < 0) gx = 0;
    if (gy < 0) gy = 0;

    *out_grid_x = gx;
    *out_grid_y = gy;
    return 0;
}
