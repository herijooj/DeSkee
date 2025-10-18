#include "clock.h"
#include <math.h>

#define PI 3.14159265358979323846

// Color definitions
#define COLOR_BLACK ARGB16(1, 0, 0, 0)
#define COLOR_WHITE ARGB16(1, 31, 31, 31)
#define COLOR_GRAY ARGB16(1, 20, 20, 20)
#define COLOR_LIGHT_GRAY ARGB16(1, 25, 25, 25)
#define COLOR_RED ARGB16(1, 31, 10, 15)
#define COLOR_CYAN ARGB16(1, 10, 25, 31)

// Helper to draw a number at position
static void draw_number(GraphicsContext* gfx, int x, int y, int num, u16 color) {
    // Simple 7-segment style numbers (3x5 pixels each, 2x scaled = 6x10)
    const u8 numbers[10][5] = {
        {0x7, 0x5, 0x5, 0x5, 0x7}, // 0
        {0x2, 0x6, 0x2, 0x2, 0x7}, // 1
        {0x7, 0x1, 0x7, 0x4, 0x7}, // 2
        {0x7, 0x1, 0x7, 0x1, 0x7}, // 3
        {0x5, 0x5, 0x7, 0x1, 0x1}, // 4
        {0x7, 0x4, 0x7, 0x1, 0x7}, // 5
        {0x7, 0x4, 0x7, 0x5, 0x7}, // 6
        {0x7, 0x1, 0x1, 0x1, 0x1}, // 7
        {0x7, 0x5, 0x7, 0x5, 0x7}, // 8
        {0x7, 0x5, 0x7, 0x1, 0x7}, // 9
    };

    if (num < 0 || num > 9) return;

    // Draw 2x scaled for better visibility
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (numbers[num][row] & (1 << (2 - col))) {
                // Draw a 2x2 block for each pixel
                gfx_plot(gfx, x + col * 2, y + row * 2, color);
                gfx_plot(gfx, x + col * 2 + 1, y + row * 2, color);
                gfx_plot(gfx, x + col * 2, y + row * 2 + 1, color);
                gfx_plot(gfx, x + col * 2 + 1, y + row * 2 + 1, color);
            }
        }
    }
}

void clock_init_config(ClockConfig* config, int center_x, int center_y, int radius) {
    config->center_x = center_x;
    config->center_y = center_y;
    config->radius = radius;
    config->show_numbers = true;
    config->show_markers = true;
    config->rotation = ROTATION_0;
}

void clock_init_light_theme(ClockTheme* theme) {
    theme->background = COLOR_WHITE;
    theme->foreground = COLOR_BLACK;
    theme->border = COLOR_BLACK;
    theme->hour_hand = COLOR_GRAY;
    theme->minute_hand = COLOR_RED;
    theme->second_hand = COLOR_CYAN;
    theme->marker = COLOR_GRAY;
}

void clock_init_dark_theme(ClockTheme* theme) {
    theme->background = COLOR_BLACK;
    theme->foreground = COLOR_WHITE;
    theme->border = COLOR_WHITE;
    theme->hour_hand = COLOR_LIGHT_GRAY;
    theme->minute_hand = COLOR_RED;
    theme->second_hand = COLOR_CYAN;
    theme->marker = COLOR_LIGHT_GRAY;
}

void clock_draw_face(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme) {
    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;
    
    // Save current rotation and set clock's rotation
    RotationAngle saved_rotation = gfx->rotation;
    int saved_px = gfx->pivot_x;
    int saved_py = gfx->pivot_y;
    gfx->rotation = config->rotation;
    gfx->pivot_x = config->center_x;
    gfx->pivot_y = config->center_y;
    
    // Clear clock area
    gfx_draw_filled_rect(gfx, cx - r - 2, cy - r - 2, r * 2 + 4, r * 2 + 4, theme->background);
    
    // Draw square border
    gfx_draw_rect(gfx, cx - r, cy - r, r * 2, r * 2, 2, theme->border);
    
    // Draw numbers at 12, 3, 6, 9 if enabled
    if (config->show_numbers) {
        // 12 at top
        draw_number(gfx, cx - 3, cy - r + 8, 1, theme->foreground);
        draw_number(gfx, cx + 4, cy - r + 8, 2, theme->foreground);
        
        // 3 at right
        draw_number(gfx, cx + r - 14, cy - 5, 3, theme->foreground);
        
        // 6 at bottom
        draw_number(gfx, cx - 3, cy + r - 18, 6, theme->foreground);
        
        // 9 at left
        draw_number(gfx, cx - r + 8, cy - 5, 9, theme->foreground);
    }
    
    // Draw hour markers if enabled
    if (config->show_markers) {
        int marker_hours[] = {1, 2, 4, 5, 7, 8, 10, 11};
        for (int i = 0; i < 8; i++) {
            float angle = (marker_hours[i] * 30 - 90) * PI / 180.0;
            int mx = cx + (int)(cos(angle) * (r - 12));
            int my = cy + (int)(sin(angle) * (r - 12));
            gfx_draw_filled_rect(gfx, mx - 2, my - 2, 4, 4, theme->marker);
        }
    }
    
    // Restore previous rotation
    gfx->rotation = saved_rotation;
    gfx->pivot_x = saved_px;
    gfx->pivot_y = saved_py;
}

void clock_draw_hands(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme,
                      int hour, int minute, int second) {
    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;
    
    // Save current rotation and set clock's rotation
    RotationAngle saved_rotation = gfx->rotation;
    int saved_px = gfx->pivot_x;
    int saved_py = gfx->pivot_y;
    gfx->rotation = config->rotation;
    gfx->pivot_x = config->center_x;
    gfx->pivot_y = config->center_y;
    
    // Convert time to angles (12 o'clock is -90 degrees)
    float second_angle = (second * 6 - 90) * PI / 180.0;
    float minute_angle = (minute * 6 + second * 0.1 - 90) * PI / 180.0;
    float hour_angle = ((hour % 12) * 30 + minute * 0.5 - 90) * PI / 180.0;
    
    // Draw hour hand (short, thick)
    int hour_length = r * 0.45;
    int hour_x = cx + (int)(cos(hour_angle) * hour_length);
    int hour_y = cy + (int)(sin(hour_angle) * hour_length);
    gfx_draw_thick_line(gfx, cx, cy, hour_x, hour_y, 6, theme->hour_hand);
    
    // Draw minute hand (medium, thick)
    int minute_length = r * 0.65;
    int minute_x = cx + (int)(cos(minute_angle) * minute_length);
    int minute_y = cy + (int)(sin(minute_angle) * minute_length);
    gfx_draw_thick_line(gfx, cx, cy, minute_x, minute_y, 5, theme->minute_hand);
    
    // Draw second hand (long, thin)
    int second_length = r * 0.75;
    int second_x = cx + (int)(cos(second_angle) * second_length);
    int second_y = cy + (int)(sin(second_angle) * second_length);
    gfx_draw_line(gfx, cx, cy, second_x, second_y, theme->second_hand);
    
    // Draw center pivot
    gfx_draw_filled_rect(gfx, cx - 2, cy - 2, 5, 5, theme->foreground);
    
    // Restore previous rotation
    gfx->rotation = saved_rotation;
    gfx->pivot_x = saved_px;
    gfx->pivot_y = saved_py;
}

void clock_draw(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme,
                int hour, int minute, int second) {
    clock_draw_face(gfx, config, theme);
    clock_draw_hands(gfx, config, theme, hour, minute, second);
}
