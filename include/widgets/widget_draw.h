#ifndef WIDGET_DRAW_H
#define WIDGET_DRAW_H

#include <nds.h>

#include "widget.h"

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int canvas_x;
    int canvas_y;
    int canvas_width;
    int canvas_height;
    int info_height;
    int checkbox_x;
    int checkbox_y;
    int checkbox_size;
    int clear_checkbox_x;
    int clear_checkbox_y;
    int clear_checkbox_size;
    bool erasing;
    bool needs_full_clear;
    bool instructions_dirty;
    bool last_point_valid;
    int last_x;
    int last_y;
    WidgetTheme theme;
    RotationAngle rotation;
    u16 background_color;
    u16 border_color;
    u16 canvas_background_color;
    u16 info_background_color;
    u16 text_color;
    u16 pen_color;
} DrawWidgetState;

void widget_draw_init(Widget* widget, DrawWidgetState* state);
void widget_draw_set_bounds(DrawWidgetState* state, int x, int y, int width, int height);

#endif // WIDGET_DRAW_H
