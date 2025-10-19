#include "grid.h"

#include <stddef.h>

static bool grid_position_valid(int x, int y, int w, int h) {
    if (x < 0 || y < 0) return false;
    if (w <= 0 || h <= 0) return false;
    if (x + w > GRID_COLS) return false;
    if (y + h > GRID_ROWS) return false;
    // Disallow spanning both screens at once
    if (y < GRID_TOP_ROWS && y + h > GRID_TOP_ROWS) return false;
    return true;
}

void grid_init(GridLayout* layout, int screen_width, int screen_height) {
    if (!layout) return;

    layout->count = 0;
    layout->cell_width = screen_width / GRID_COLS;
    if (layout->cell_width <= 0) {
        layout->cell_width = 1;
    }
    layout->offset_x = (screen_width - layout->cell_width * GRID_COLS) / 2;
    if (layout->offset_x < 0) {
        layout->offset_x = 0;
    }
    layout->cell_height = screen_height / GRID_TOP_ROWS;
    if (layout->cell_height <= 0) {
        layout->cell_height = 1;
    }
}

static bool grid_overlaps(const GridItem* a, const GridItem* b) {
    if (!a || !b) return false;
    if (a == b) return false;

    bool separate_x = (a->grid_x + a->grid_w) <= b->grid_x ||
                      (b->grid_x + b->grid_w) <= a->grid_x;
    bool separate_y = (a->grid_y + a->grid_h) <= b->grid_y ||
                      (b->grid_y + b->grid_h) <= a->grid_y;
    return !(separate_x || separate_y);
}

bool grid_can_place(const GridLayout* layout, int x, int y, int w, int h, int ignore_index) {
    if (!layout) return false;
    if (!grid_position_valid(x, y, w, h)) return false;

    for (int i = 0; i < layout->count; ++i) {
        if (i == ignore_index) continue;
        const GridItem* item = &layout->items[i];
        GridItem temp = {.grid_x = x, .grid_y = y, .grid_w = w, .grid_h = h};
        if (grid_overlaps(&temp, item)) {
            return false;
        }
    }

    return true;
}

int grid_add_widget(GridLayout* layout, Widget* widget, int grid_w, int grid_h,
                    int grid_x, int grid_y, bool placeholder) {
    if (!layout || !widget) return -1;
    if (layout->count >= GRID_MAX_ITEMS) return -1;
    if (!grid_can_place(layout, grid_x, grid_y, grid_w, grid_h, -1)) return -1;

    GridItem* item = &layout->items[layout->count];
    item->widget = widget;
    item->grid_w = grid_w;
    item->grid_h = grid_h;
    item->grid_x = grid_x;
    item->grid_y = grid_y;
    item->placeholder = placeholder;

    layout->count++;
    return layout->count - 1;
}

GridItem* grid_get_item(GridLayout* layout, int index) {
    if (!layout || index < 0 || index >= layout->count) return NULL;
    return &layout->items[index];
}

const GridItem* grid_get_item_const(const GridLayout* layout, int index) {
    if (!layout || index < 0 || index >= layout->count) return NULL;
    return &layout->items[index];
}

int grid_find_index(const GridLayout* layout, const Widget* widget) {
    if (!layout || !widget) return -1;
    for (int i = 0; i < layout->count; ++i) {
        if (layout->items[i].widget == widget) {
            return i;
        }
    }
    return -1;
}

bool grid_move_widget(GridLayout* layout, int index, int new_x, int new_y) {
    GridItem* item = grid_get_item(layout, index);
    if (!item) return false;
    if (!grid_can_place(layout, new_x, new_y, item->grid_w, item->grid_h, index)) {
        return false;
    }

    item->grid_x = new_x;
    item->grid_y = new_y;
    return true;
}

GridBounds grid_item_bounds(const GridLayout* layout, const GridItem* item) {
    GridBounds bounds = {0, 0, 0, 0};
    if (!layout || !item) return bounds;

    bounds.x = layout->offset_x + item->grid_x * layout->cell_width;
    bounds.y = item->grid_y * layout->cell_height;
    bounds.width = item->grid_w * layout->cell_width;
    bounds.height = item->grid_h * layout->cell_height;
    return bounds;
}

GridScreen grid_item_screen(const GridItem* item) {
    if (!item) return GRID_SCREEN_TOP;
    return (item->grid_y >= GRID_TOP_ROWS) ? GRID_SCREEN_BOTTOM : GRID_SCREEN_TOP;
}

void grid_item_screen_rect(const GridLayout* layout, const GridItem* item,
                          int* out_x, int* out_y, int* out_width, int* out_height) {
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_width) *out_width = 0;
    if (out_height) *out_height = 0;

    if (!layout || !item) return;

    GridBounds bounds = grid_item_bounds(layout, item);
    if (out_x) *out_x = bounds.x;
    if (out_width) *out_width = bounds.width;

    if (grid_item_screen(item) == GRID_SCREEN_TOP) {
        if (out_y) *out_y = bounds.y;
        if (out_height) *out_height = bounds.height;
    } else {
        int top_height = GRID_TOP_ROWS * layout->cell_height;
        if (out_y) *out_y = bounds.y - top_height;
        if (out_height) *out_height = bounds.height;
    }
}
