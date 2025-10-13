#ifndef CLOCK_H
#define CLOCK_H

#include "graphics.h"
#include <time.h>

// Clock configuration
typedef struct {
    int center_x;
    int center_y;
    int radius;
    bool show_numbers;    // Show 12, 3, 6, 9
    bool show_markers;    // Show hour markers
    RotationAngle rotation; // Clock's own rotation
} ClockConfig;

// Theme colors
typedef struct {
    u16 background;
    u16 foreground;
    u16 border;
    u16 hour_hand;
    u16 minute_hand;
    u16 second_hand;
    u16 marker;
} ClockTheme;

// Initialize default clock config
void clock_init_config(ClockConfig* config, int center_x, int center_y, int radius);

// Initialize light theme
void clock_init_light_theme(ClockTheme* theme);

// Initialize dark theme
void clock_init_dark_theme(ClockTheme* theme);

// Draw clock face (border, numbers, markers)
void clock_draw_face(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme);

// Draw clock hands for given time
void clock_draw_hands(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme, 
                      int hour, int minute, int second);

// Draw complete clock (face + hands)
void clock_draw(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme,
                int hour, int minute, int second);

#endif // CLOCK_H
