#ifndef WIDGET_CLOCK_H
#define WIDGET_CLOCK_H

#include <stdbool.h>
#include <time.h>

#include "widget.h"

typedef struct {
    int center_x;
    int center_y;
    int radius;
    bool show_numbers;
    bool show_markers;
    RotationAngle rotation;
} ClockConfig;

typedef struct {
    u16 background;
    u16 foreground;
    u16 border;
    u16 hour_hand;
    u16 minute_hand;
    u16 second_hand;
    u16 marker;
} ClockTheme;

typedef struct {
    ClockConfig config;
    ClockTheme light_theme;
    ClockTheme dark_theme;
    int last_second;
    int drawn_hour;
    int drawn_minute;
    int drawn_second;
    bool face_dirty;
} ClockWidgetState;

void widget_clock_init(Widget* widget, ClockWidgetState* state);
ClockTheme* widget_clock_current_theme(ClockWidgetState* state, WidgetTheme theme);

#endif // WIDGET_CLOCK_H
