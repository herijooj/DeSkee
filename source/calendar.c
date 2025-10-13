#include "calendar.h"

// Helper to draw small numbers (2x5 pixel bitmap)
static void draw_small_number(GraphicsContext* gfx, int x, int y, int num, u16 color) {
    const int small_nums[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
        {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        {0b111, 0b001, 0b001, 0b001, 0b001}, // 7
        {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}  // 9
    };

    if (num < 0 || num > 9) return;

    for (int py = 0; py < 5; py++) {
        for (int px = 0; px < 3; px++) {
            if (small_nums[num][py] & (1 << (2 - px))) {
                gfx_plot(gfx, x + px, y + py, color);
            }
        }
    }
}

static void draw_small_letter(GraphicsContext* gfx, int x, int y, char letter, u16 color) {
    static const int glyph_S[5] = {0b111, 0b100, 0b111, 0b001, 0b111};
    static const int glyph_M[5] = {0b101, 0b111, 0b101, 0b101, 0b101};
    static const int glyph_T[5] = {0b111, 0b010, 0b010, 0b010, 0b010};
    static const int glyph_W[5] = {0b101, 0b101, 0b101, 0b111, 0b101};
    static const int glyph_F[5] = {0b111, 0b100, 0b110, 0b100, 0b100};

    const int* glyph = NULL;
    switch (letter) {
        case 'S': glyph = glyph_S; break;
        case 'M': glyph = glyph_M; break;
        case 'T': glyph = glyph_T; break;
        case 'W': glyph = glyph_W; break;
        case 'F': glyph = glyph_F; break;
        default: return;
    }

    for (int py = 0; py < 5; py++) {
        for (int px = 0; px < 3; px++) {
            if (glyph[py] & (1 << (2 - px))) {
                gfx_plot(gfx, x + px, y + py, color);
            }
        }
    }
}

// Get days in month
static int get_days_in_month(int month, int year) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        return 29; // Leap year
    }
    return days[month];
}

// Get first day of month (0 = Sunday)
static int get_first_day_of_month(int month, int year) {
    // Zeller's congruence
    if (month < 2) {
        month += 12;
        year--;
    }
    int q = 1;
    int m = month;
    int k = year % 100;
    int j = year / 100;
    int h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
    return (h + 6) % 7; // Convert to 0=Sunday format
}

void calendar_init_config(CalendarConfig* config, int x, int y, int cell_size) {
    config->offset_x = x;
    config->offset_y = y;
    config->cell_width = cell_size;
    config->cell_height = cell_size;
    config->show_month_year = true;
    config->rotation = ROTATION_0;
}

void calendar_init_light_theme(CalendarTheme* theme, const ClockTheme* clock_theme) {
    theme->background = clock_theme->background;
    theme->text = clock_theme->foreground;
    theme->border = clock_theme->border;
    theme->sunday = clock_theme->minute_hand;
    theme->saturday = clock_theme->second_hand;
    theme->weekday = clock_theme->hour_hand;
    theme->today = clock_theme->foreground;
    theme->text_on_colored = clock_theme->foreground;
    theme->today_text = clock_theme->background;
}

void calendar_init_dark_theme(CalendarTheme* theme, const ClockTheme* clock_theme) {
    theme->background = clock_theme->background;
    theme->text = clock_theme->foreground;
    theme->border = clock_theme->border;
    theme->sunday = clock_theme->minute_hand;
    theme->saturday = clock_theme->second_hand;
    theme->weekday = clock_theme->hour_hand;
    theme->today = clock_theme->foreground;
    theme->text_on_colored = clock_theme->foreground;
    theme->today_text = clock_theme->background;
}

void calendar_draw(GraphicsContext* gfx, const CalendarConfig* config,
                   const CalendarTheme* theme, struct tm* timeinfo) {
    // Save current rotation and set calendar's rotation
    RotationAngle saved_rotation = gfx->rotation;
    int saved_px = gfx->pivot_x;
    int saved_py = gfx->pivot_y;
    gfx->rotation = config->rotation;
    
    int start_x = config->offset_x + 6;
    int start_y = config->offset_y + 22;
    int cell_w = config->cell_width;
    int cell_h = config->cell_height;
    int total_width = 7 * cell_w + 8;
    int total_height = 22 + 7 * cell_h;
    gfx->pivot_x = config->offset_x + total_width / 2;
    gfx->pivot_y = config->offset_y + total_height / 2;
    
    // Clear calendar area - tighter bounds to avoid overlap
    // Width: 7 cells + small margins = 7 * 13 + 8 = 99
    // Height: header (22) + 7 rows (1 header + 6 weeks max) = 22 + 7*13 = 113
    gfx_draw_filled_rect(gfx, config->offset_x, config->offset_y, 
                         total_width, total_height, theme->background);
    
    // Draw month/year header if enabled
    if (config->show_month_year) {
        int month = timeinfo->tm_mon + 1;
        int year = timeinfo->tm_year + 1900;
        
        int header_x = config->offset_x + 10;
        int header_y = config->offset_y + 5;
        
        // Draw MM/YYYY
        draw_small_number(gfx, header_x, header_y, month / 10, theme->text);
        draw_small_number(gfx, header_x + 4, header_y, month % 10, theme->text);
        
        // Draw slash
        gfx_plot(gfx, header_x + 9, header_y + 2, theme->text);
        
        // Draw year
        draw_small_number(gfx, header_x + 12, header_y, (year / 1000) % 10, theme->text);
        draw_small_number(gfx, header_x + 16, header_y, (year / 100) % 10, theme->text);
        draw_small_number(gfx, header_x + 20, header_y, (year / 10) % 10, theme->text);
        draw_small_number(gfx, header_x + 24, header_y, year % 10, theme->text);
    }
    
    // Draw day headers (Sun-Sat) with colored backgrounds
    u16 header_colors[] = {
        theme->sunday, theme->weekday, theme->weekday, theme->weekday,
        theme->weekday, theme->weekday, theme->saturday
    };
    const char* header_labels = "SMTWTFS";
    
    for (int day = 0; day < 7; day++) {
        int x = start_x + day * cell_w;
        int y = start_y;
        
        // Draw header cell background
        for (int py = 0; py < cell_h - 1; py++) {
            for (int px = 0; px < cell_w - 1; px++) {
                gfx_plot(gfx, x + px, y + py, header_colors[day]);
            }
        }
        
        // Draw border
        for (int i = 0; i < cell_w - 1; i++) {
            gfx_plot(gfx, x + i, y, theme->border);
            gfx_plot(gfx, x + i, y + cell_h - 2, theme->border);
        }
        for (int i = 0; i < cell_h - 1; i++) {
            gfx_plot(gfx, x, y + i, theme->border);
            gfx_plot(gfx, x + cell_w - 2, y + i, theme->border);
        }

        // Draw header letter centered
        int letter_x = x + (cell_w - 3) / 2;
        int letter_y = y + (cell_h - 1 - 5) / 2;
    char letter = header_labels[day];
    u16 letter_color = (day == 0 || day == 6) ? theme->text_on_colored : theme->text;
    draw_small_letter(gfx, letter_x, letter_y, letter, letter_color);
    }
    
    // Get calendar info
    int days_in_month = get_days_in_month(timeinfo->tm_mon, timeinfo->tm_year + 1900);
    int first_day = get_first_day_of_month(timeinfo->tm_mon, timeinfo->tm_year + 1900);
    int today = timeinfo->tm_mday;
    
    // Draw calendar days
    int day_num = 1;
    for (int week = 0; week < 6 && day_num <= days_in_month; week++) {
        for (int day = 0; day < 7; day++) {
            if (week == 0 && day < first_day) {
                continue; // Empty cell before month starts
            }
            if (day_num > days_in_month) {
                break;
            }
            
            int x = start_x + day * cell_w;
            int y = start_y + (week + 1) * cell_h;
            
            // Determine cell background color
            u16 cell_bg;
            if (day_num == today) {
                cell_bg = theme->today; // Today's date
            } else if (day == 0) {
                cell_bg = theme->sunday; // Sunday
            } else if (day == 6) {
                cell_bg = theme->saturday; // Saturday
            } else {
                cell_bg = theme->background; // Regular day
            }
            
            // Draw cell background
            for (int py = 0; py < cell_h - 1; py++) {
                for (int px = 0; px < cell_w - 1; px++) {
                    gfx_plot(gfx, x + px, y + py, cell_bg);
                }
            }
            
            // Draw border
            for (int i = 0; i < cell_w - 1; i++) {
                gfx_plot(gfx, x + i, y, theme->border);
                gfx_plot(gfx, x + i, y + cell_h - 2, theme->border);
            }
            for (int i = 0; i < cell_h - 1; i++) {
                gfx_plot(gfx, x, y + i, theme->border);
                gfx_plot(gfx, x + cell_w - 2, y + i, theme->border);
            }
            
            // Draw day number
            int num_x = x + 3;
            int num_y = y + 4;
            u16 num_color = theme->text;
            if (day_num == today) {
                num_color = theme->today_text;
            } else if (day == 0 || day == 6) {
                num_color = theme->text_on_colored;
            }
            
            if (day_num >= 10) {
                draw_small_number(gfx, num_x, num_y, day_num / 10, num_color);
                draw_small_number(gfx, num_x + 4, num_y, day_num % 10, num_color);
            } else {
                draw_small_number(gfx, num_x + 2, num_y, day_num, num_color);
            }
            
            day_num++;
        }
    }
    
    // Restore previous rotation
    gfx->rotation = saved_rotation;
    gfx->pivot_x = saved_px;
    gfx->pivot_y = saved_py;
}
