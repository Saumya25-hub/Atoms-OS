#include "framework/include/bos_ui_layout.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

BOS_Grid* BOS_Grid_Create(void) {
    BOS_Grid* grid = (BOS_Grid*)kmalloc(sizeof(BOS_Grid));
    if (!grid) return NULL;

    memset(grid, 0, sizeof(BOS_Grid));
    BOS_UIElement_Init(&grid->base, "Grid");
    return grid;
}

void BOS_Grid_AddRow(BOS_Grid* grid, float value, BOS_GridUnitType unit) {
    if (!grid) return;
    uint32_t new_count = grid->row_count + 1;
    BOS_RowDefinition* new_rows = (BOS_RowDefinition*)kmalloc(sizeof(BOS_RowDefinition) * new_count);
    if (!new_rows) return;

    if (grid->rows && grid->row_count > 0) {
        memcpy(new_rows, grid->rows, sizeof(BOS_RowDefinition) * grid->row_count);
        kfree(grid->rows);
    }

    new_rows[grid->row_count].value = value;
    new_rows[grid->row_count].unit = unit;
    new_rows[grid->row_count].computed_size = 0;

    grid->rows = new_rows;
    grid->row_count = new_count;
    BOS_UIElement_InvalidateLayout(&grid->base);
}

void BOS_Grid_AddColumn(BOS_Grid* grid, float value, BOS_GridUnitType unit) {
    if (!grid) return;
    uint32_t new_count = grid->column_count + 1;
    BOS_ColumnDefinition* new_cols = (BOS_ColumnDefinition*)kmalloc(sizeof(BOS_ColumnDefinition) * new_count);
    if (!new_cols) return;

    if (grid->columns && grid->column_count > 0) {
        memcpy(new_cols, grid->columns, sizeof(BOS_ColumnDefinition) * grid->column_count);
        kfree(grid->columns);
    }

    new_cols[grid->column_count].value = value;
    new_cols[grid->column_count].unit = unit;
    new_cols[grid->column_count].computed_size = 0;

    grid->columns = new_cols;
    grid->column_count = new_count;
    BOS_UIElement_InvalidateLayout(&grid->base);
}

void BOS_Grid_SetCell(BOS_Grid* grid, BOS_UIElement* elem, uint32_t row, uint32_t col) {
    if (!grid || !elem) return;
    (void)row; (void)col;
    BOS_UIElement_AddChild(&grid->base, elem);
}
