#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

#include "widgets/widget.h"

#define GRID_COLS 5
#define GRID_ROWS 8
#define GRID_TOP_ROWS 4
#define GRID_MAX_ITEMS 16

typedef enum {
    GRID_SCREEN_TOP = 0,
    GRID_SCREEN_BOTTOM = 1
} GridScreen;

typedef struct {
    int x;
    int y;
    int width;
    int height;
} GridBounds;

typedef struct {
    Widget* widget;
    int grid_x;
    int grid_y;
    int grid_w;
    int grid_h;
    bool placeholder;
} GridItem;

typedef struct {
    GridItem items[GRID_MAX_ITEMS];
    int count;
    int cell_width;
    int cell_height;
    int offset_x;
} GridLayout;

void grid_init(GridLayout* layout, int screen_width, int screen_height);
int grid_add_widget(GridLayout* layout, Widget* widget, int grid_w, int grid_h,
                    int grid_x, int grid_y, bool placeholder);
GridItem* grid_get_item(GridLayout* layout, int index);
const GridItem* grid_get_item_const(const GridLayout* layout, int index);
int grid_find_index(const GridLayout* layout, const Widget* widget);
bool grid_move_widget(GridLayout* layout, int index, int new_x, int new_y);
GridBounds grid_item_bounds(const GridLayout* layout, const GridItem* item);
GridScreen grid_item_screen(const GridItem* item);
void grid_item_screen_rect(const GridLayout* layout, const GridItem* item,
                          int* out_x, int* out_y, int* out_width, int* out_height);
bool grid_can_place(const GridLayout* layout, int x, int y, int w, int h, int ignore_index);

#endif // GRID_H
