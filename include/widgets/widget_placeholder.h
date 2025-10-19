#ifndef WIDGET_PLACEHOLDER_H
#define WIDGET_PLACEHOLDER_H

#include "widget.h"

typedef struct {
    int x;
    int y;
    int width;
    int height;
    u16 light_color;
    u16 dark_color;
    u16 border_color;
    u16 active_fill;
    bool dirty;
} PlaceholderWidgetState;

void widget_placeholder_init(Widget* widget, PlaceholderWidgetState* state);
void widget_placeholder_set_bounds(PlaceholderWidgetState* state, int x, int y, int width, int height);

#endif // WIDGET_PLACEHOLDER_H
