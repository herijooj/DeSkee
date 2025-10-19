#include "widgets/widget_calendar.h"

#include <nds.h>
#include <time.h>

#define COLOR_BLACK ARGB16(1, 0, 0, 0)
#define COLOR_WHITE ARGB16(1, 31, 31, 31)
#define COLOR_GRAY ARGB16(1, 20, 20, 20)
#define COLOR_LIGHT_GRAY ARGB16(1, 25, 25, 25)
#define COLOR_RED ARGB16(1, 31, 10, 15)
#define COLOR_CYAN ARGB16(1, 10, 25, 31)

#define COLOR_DARK_BG ARGB16(1, 0, 0, 0)
#define COLOR_DARK_TEXT ARGB16(1, 31, 31, 31)
#define COLOR_DARK_BORDER ARGB16(1, 31, 31, 31)
#define COLOR_DARK_HOUR ARGB16(1, 25, 25, 25)

static void calendar_init_config(CalendarConfig* config, int x, int y, int cell_width, int cell_height) {
    config->offset_x = x;
    config->offset_y = y;
    config->cell_width = cell_width;
    config->cell_height = cell_height;
    config->show_month_year = true;
    config->rotation = ROTATION_0;
}

static void calendar_init_light_theme(CalendarTheme* theme) {
    theme->background = COLOR_WHITE;
    theme->text = COLOR_BLACK;
    theme->border = COLOR_BLACK;
    theme->sunday = COLOR_RED;
    theme->saturday = COLOR_CYAN;
    theme->weekday = COLOR_GRAY;
    theme->today = COLOR_BLACK;
    theme->text_on_colored = COLOR_BLACK;
    theme->today_text = COLOR_WHITE;
}

static void calendar_init_dark_theme(CalendarTheme* theme) {
    theme->background = COLOR_DARK_BG;
    theme->text = COLOR_DARK_TEXT;
    theme->border = COLOR_DARK_BORDER;
    theme->sunday = COLOR_RED;
    theme->saturday = COLOR_CYAN;
    theme->weekday = COLOR_DARK_HOUR;
    theme->today = COLOR_DARK_TEXT;
    theme->text_on_colored = COLOR_DARK_TEXT;
    theme->today_text = COLOR_DARK_BG;
}

static void calendar_widget_reset(CalendarWidgetState* state) {
    if (!state) return;

    state->cached_day = -1;
    state->cached_month = -1;
    state->cached_year = -1;
    state->dirty = true;
}

void widget_calendar_set_bounds(CalendarWidgetState* state, int x, int y, int width, int height) {
    if (!state) return;

    state->bounds_x = x;
    state->bounds_y = y;
    state->bounds_width = width;
    state->bounds_height = height;

    int available_width = width;
    int available_height = height;
    if (available_width < 14) available_width = 14;
    if (available_height < 30) available_height = 30;

    int cell_w = (available_width - 8) / 7;
    int cell_h = (available_height - 22) / 7;
    if (cell_w < 8) cell_w = 8;
    if (cell_h < 8) cell_h = 8;

    int total_width = 7 * cell_w + 8;
    int total_height = 22 + 7 * cell_h;

    int offset_x = x + (width - total_width) / 2;
    int offset_y = y + (height - total_height) / 2;
    if (offset_x < x) offset_x = x;
    if (offset_y < y) offset_y = y;

    RotationAngle rotation = state->config.rotation;
    calendar_init_config(&state->config, offset_x, offset_y, cell_w, cell_h);
    state->config.rotation = rotation;
    state->dirty = true;
}

static void calendar_draw_bounds(GraphicsContext* gfx, const CalendarWidgetState* state,
                                 u16 fill_color, u16 border_color, bool draw_fill, bool draw_border) {
    if (!gfx || !state) return;
    if (!draw_fill && !draw_border) return;
    if (state->bounds_width <= 0 || state->bounds_height <= 0) return;

    RotationAngle saved_rotation = gfx->rotation;
    int saved_px = gfx->pivot_x;
    int saved_py = gfx->pivot_y;

    gfx->rotation = state->config.rotation;
    gfx->pivot_x = state->bounds_x + state->bounds_width / 2;
    gfx->pivot_y = state->bounds_y + state->bounds_height / 2;

    if (draw_fill) {
        gfx_draw_filled_rect(gfx, state->bounds_x, state->bounds_y,
                             state->bounds_width, state->bounds_height, fill_color);
    }

    if (draw_border) {
        gfx_draw_rect(gfx, state->bounds_x, state->bounds_y,
                      state->bounds_width, state->bounds_height, 1, border_color);
    }

    gfx->rotation = saved_rotation;
    gfx->pivot_x = saved_px;
    gfx->pivot_y = saved_py;
}

static int get_days_in_month(int month, int year) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) {
        return 29;
    }
    return days[month];
}

static int get_first_day_of_month(int month, int year) {
    if (month < 2) {
        month += 12;
        year--;
    }
    int q = 1;
    int m = month;
    int k = year % 100;
    int j = year / 100;
    int h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
    return (h + 6) % 7;
}

static void draw_small_number(GraphicsContext* gfx, int x, int y, int num, u16 color) {
    const int small_nums[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111},
        {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111},
        {0b111, 0b001, 0b111, 0b001, 0b111},
        {0b101, 0b101, 0b111, 0b001, 0b001},
        {0b111, 0b100, 0b111, 0b001, 0b111},
        {0b111, 0b100, 0b111, 0b101, 0b111},
        {0b111, 0b001, 0b001, 0b001, 0b001},
        {0b111, 0b101, 0b111, 0b101, 0b111},
        {0b111, 0b101, 0b111, 0b001, 0b111},
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

static void calendar_draw(GraphicsContext* gfx, const CalendarConfig* config,
                          const CalendarTheme* theme, struct tm* timeinfo) {
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

    gfx_draw_filled_rect(gfx, config->offset_x, config->offset_y,
                         total_width, total_height, theme->background);

    if (config->show_month_year) {
        int month = timeinfo->tm_mon + 1;
        int year = timeinfo->tm_year + 1900;

        int header_x = config->offset_x + 10;
        int header_y = config->offset_y + 5;

        draw_small_number(gfx, header_x, header_y, month / 10, theme->text);
        draw_small_number(gfx, header_x + 4, header_y, month % 10, theme->text);

        gfx_plot(gfx, header_x + 9, header_y + 2, theme->text);

        draw_small_number(gfx, header_x + 12, header_y, (year / 1000) % 10, theme->text);
        draw_small_number(gfx, header_x + 16, header_y, (year / 100) % 10, theme->text);
        draw_small_number(gfx, header_x + 20, header_y, (year / 10) % 10, theme->text);
        draw_small_number(gfx, header_x + 24, header_y, year % 10, theme->text);
    }

    u16 header_colors[] = {
        theme->sunday, theme->weekday, theme->weekday, theme->weekday,
        theme->weekday, theme->weekday, theme->saturday
    };
    const char* header_labels = "SMTWTFS";

    for (int day = 0; day < 7; day++) {
        int x = start_x + day * cell_w;
        int y = start_y;

        for (int py = 0; py < cell_h - 1; py++) {
            for (int px = 0; px < cell_w - 1; px++) {
                gfx_plot(gfx, x + px, y + py, header_colors[day]);
            }
        }

        for (int i = 0; i < cell_w - 1; i++) {
            gfx_plot(gfx, x + i, y, theme->border);
            gfx_plot(gfx, x + i, y + cell_h - 2, theme->border);
        }
        for (int i = 0; i < cell_h - 1; i++) {
            gfx_plot(gfx, x, y + i, theme->border);
            gfx_plot(gfx, x + cell_w - 2, y + i, theme->border);
        }

        int letter_x = x + (cell_w - 3) / 2;
        int letter_y = y + (cell_h - 1 - 5) / 2;
        char letter = header_labels[day];
        u16 letter_color = (day == 0 || day == 6) ? theme->text_on_colored : theme->text;
        draw_small_letter(gfx, letter_x, letter_y, letter, letter_color);
    }

    int days_in_month = get_days_in_month(timeinfo->tm_mon, timeinfo->tm_year + 1900);
    int first_day = get_first_day_of_month(timeinfo->tm_mon, timeinfo->tm_year + 1900);
    int today = timeinfo->tm_mday;

    int day_num = 1;
    for (int week = 0; week < 6 && day_num <= days_in_month; week++) {
        for (int day = 0; day < 7; day++) {
            if (week == 0 && day < first_day) {
                continue;
            }
            if (day_num > days_in_month) {
                break;
            }

            int x = start_x + day * cell_w;
            int y = start_y + (week + 1) * cell_h;

            u16 cell_bg;
            if (day_num == today) {
                cell_bg = theme->today;
            } else if (day == 0) {
                cell_bg = theme->sunday;
            } else if (day == 6) {
                cell_bg = theme->saturday;
            } else {
                cell_bg = theme->background;
            }

            for (int py = 0; py < cell_h - 1; py++) {
                for (int px = 0; px < cell_w - 1; px++) {
                    gfx_plot(gfx, x + px, y + py, cell_bg);
                }
            }

            for (int i = 0; i < cell_w - 1; i++) {
                gfx_plot(gfx, x + i, y, theme->border);
                gfx_plot(gfx, x + i, y + cell_h - 2, theme->border);
            }
            for (int i = 0; i < cell_h - 1; i++) {
                gfx_plot(gfx, x, y + i, theme->border);
                gfx_plot(gfx, x + cell_w - 2, y + i, theme->border);
            }

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

    gfx->rotation = saved_rotation;
    gfx->pivot_x = saved_px;
    gfx->pivot_y = saved_py;
}

static void calendar_widget_attach(Widget* widget, GraphicsContext* context) {
    (void)context;
    CalendarWidgetState* state = widget_state(widget);
    calendar_widget_reset(state);
    state->config.rotation = widget->rotation;
}

static void calendar_widget_detach(Widget* widget) {
    CalendarWidgetState* state = widget_state(widget);
    calendar_widget_reset(state);
}

static void calendar_widget_theme_changed(Widget* widget, WidgetTheme theme) {
    CalendarWidgetState* state = widget_state(widget);
    state->dirty = true;
    (void)theme;
}

static void calendar_widget_rotation_changed(Widget* widget, RotationAngle rotation) {
    CalendarWidgetState* state = widget_state(widget);
    state->config.rotation = rotation;
    state->dirty = true;
}

static void calendar_widget_layout_changed(Widget* widget, bool split_mode) {
    CalendarWidgetState* state = widget_state(widget);
    (void)split_mode;
    state->config.rotation = widget->rotation;
    state->dirty = true;
}

static void calendar_widget_time_tick(Widget* widget, const struct tm* timeinfo) {
    if (!widget || !timeinfo) return;

    CalendarWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);

    if (!state || !ctx || !ctx->framebuffer) return;

    bool date_changed = (timeinfo->tm_mday != state->cached_day) ||
                        (timeinfo->tm_mon != state->cached_month) ||
                        (timeinfo->tm_year != state->cached_year);

    if (!state->dirty && !date_changed) {
        return;
    }

    CalendarTheme* theme = (widget->theme == WIDGET_THEME_LIGHT)
                                ? &state->light_theme
                                : &state->dark_theme;

    calendar_draw_bounds(ctx, state, theme->background, theme->border, true, false);

    struct tm time_copy = *timeinfo;
    calendar_draw(ctx, &state->config, theme, &time_copy);

    calendar_draw_bounds(ctx, state, theme->background, theme->border, false, true);

    state->cached_day = timeinfo->tm_mday;
    state->cached_month = timeinfo->tm_mon;
    state->cached_year = timeinfo->tm_year;
    state->dirty = false;

    if (widget->split_mode) {
        bgUpdate();
    }
}

static const WidgetOps CALENDAR_WIDGET_OPS = {
    .on_attach = calendar_widget_attach,
    .on_detach = calendar_widget_detach,
    .on_theme_changed = calendar_widget_theme_changed,
    .on_rotation_changed = calendar_widget_rotation_changed,
    .on_layout_changed = calendar_widget_layout_changed,
    .on_time_tick = calendar_widget_time_tick,
    .on_update = NULL,
};

void widget_calendar_init(Widget* widget, CalendarWidgetState* state) {
    if (!widget || !state) return;

    calendar_init_light_theme(&state->light_theme);
    calendar_init_dark_theme(&state->dark_theme);

    calendar_init_config(&state->config, 0, 0, 13, 13);
    state->config.rotation = ROTATION_0;
    state->bounds_x = 0;
    state->bounds_y = 0;
    state->bounds_width = 0;
    state->bounds_height = 0;
    calendar_widget_reset(state);

    widget_init(widget, "Calendar", state, &CALENDAR_WIDGET_OPS);
}
