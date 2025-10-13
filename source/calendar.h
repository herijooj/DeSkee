#ifndef CALENDAR_H
#define CALENDAR_H

#include "graphics.h"
#include "clock.h"
#include <time.h>

// Calendar configuration
typedef struct {
    int offset_x;         // Top-left X position
    int offset_y;         // Top-left Y position
    int cell_width;       // Width of each cell
    int cell_height;      // Height of each cell
    bool show_month_year; // Show "MM/YYYY" header
    RotationAngle rotation; // Calendar's own rotation
} CalendarConfig;

// Calendar theme colors
typedef struct {
    u16 background;
    u16 text;
    u16 border;
    u16 sunday;           // Sunday header/cells
    u16 saturday;         // Saturday header/cells
    u16 weekday;          // Regular weekday header
    u16 today;            // Today's date background
    u16 text_on_colored;  // Text color on colored backgrounds
    u16 today_text;       // Text color for today's cell
} CalendarTheme;

// Initialize default calendar config
void calendar_init_config(CalendarConfig* config, int x, int y, int cell_size);

// Initialize light theme
void calendar_init_light_theme(CalendarTheme* theme, const ClockTheme* clock_theme);

// Initialize dark theme
void calendar_init_dark_theme(CalendarTheme* theme, const ClockTheme* clock_theme);

// Draw calendar for given date
void calendar_draw(GraphicsContext* gfx, const CalendarConfig* config, 
                   const CalendarTheme* theme, struct tm* timeinfo);

#endif // CALENDAR_H
