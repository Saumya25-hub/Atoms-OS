#ifndef BOS_UI_LAYOUT_H
#define BOS_UI_LAYOUT_H

#include "bos_ui_core.h"

/* Grid Definitions */
typedef enum {
    BOS_GRID_UNIT_AUTO,
    BOS_GRID_UNIT_PIXEL,
    BOS_GRID_UNIT_STAR
} BOS_GridUnitType;

typedef struct {
    float            value;
    BOS_GridUnitType unit;
    int32_t          computed_size;
} BOS_RowDefinition, BOS_ColumnDefinition;

typedef struct {
    BOS_UIElement        base;
    BOS_RowDefinition*   rows;
    uint32_t             row_count;
    BOS_ColumnDefinition* columns;
    uint32_t             column_count;
} BOS_Grid;

/* StackPanel Definitions */
typedef enum {
    BOS_ORIENTATION_VERTICAL = 0,
    BOS_ORIENTATION_HORIZONTAL
} BOS_Orientation;

typedef struct {
    BOS_UIElement   base;
    BOS_Orientation orientation;
    int32_t         spacing;
} BOS_StackPanel;

/* DockPanel Definitions */
typedef enum {
    BOS_DOCK_LEFT = 0,
    BOS_DOCK_TOP,
    BOS_DOCK_RIGHT,
    BOS_DOCK_BOTTOM,
    BOS_DOCK_FILL
} BOS_Dock;

typedef struct {
    BOS_UIElement base;
    bool          last_child_fill;
} BOS_DockPanel;

/* Canvas Definitions */
typedef struct {
    BOS_UIElement base;
} BOS_Canvas;

/* Factory Functions */
BOS_Grid*       BOS_Grid_Create(void);
void            BOS_Grid_AddRow(BOS_Grid* grid, float value, BOS_GridUnitType unit);
void            BOS_Grid_AddColumn(BOS_Grid* grid, float value, BOS_GridUnitType unit);
void            BOS_Grid_SetCell(BOS_Grid* grid, BOS_UIElement* elem, uint32_t row, uint32_t col);

BOS_StackPanel* BOS_StackPanel_Create(BOS_Orientation orientation);
BOS_DockPanel*  BOS_DockPanel_Create(bool last_child_fill);
BOS_Canvas*     BOS_Canvas_Create(void);

void BOS_Layout_MeasurePass(BOS_UIElement* elem, BOS_Size available_size);
void BOS_Layout_ArrangePass(BOS_UIElement* elem, BOS_Rect final_rect);

#endif /* BOS_UI_LAYOUT_H */
