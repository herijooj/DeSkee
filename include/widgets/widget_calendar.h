#ifndef WIDGET_CALENDAR_H
#define WIDGET_CALENDAR_H

#include <stdbool.h>
#include <time.h>

#include "widget.h"

typedef struct {
    int offset_x;
    int offset_y;
    int cell_width;
    int cell_height;
    bool show_month_year;
    RotationAngle rotation;
} CalendarConfig;

typedef struct {
    u16 background;
    u16 text;
    u16 border;
    u16 sunday;
    u16 saturday;
    u16 weekday;
    u16 today;
    u16 text_on_colored;
    u16 today_text;
} CalendarTheme;

typedef struct {
    CalendarConfig config;
    CalendarTheme light_theme;
    CalendarTheme dark_theme;
    int cached_day;
    int cached_month;
    int cached_year;
    bool dirty;
} CalendarWidgetState;

void widget_calendar_init(Widget* widget, CalendarWidgetState* state);

#endif // WIDGET_CALENDAR_H
