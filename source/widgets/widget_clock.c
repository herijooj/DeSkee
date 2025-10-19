#include "widgets/widget_clock.h"

#include <math.h>
#include <nds.h>

#define PI 3.14159265358979323846

#define COLOR_BLACK ARGB16(1, 0, 0, 0)
#define COLOR_WHITE ARGB16(1, 31, 31, 31)
#define COLOR_GRAY ARGB16(1, 20, 20, 20)
#define COLOR_LIGHT_GRAY ARGB16(1, 25, 25, 25)
#define COLOR_RED ARGB16(1, 31, 10, 15)
#define COLOR_CYAN ARGB16(1, 10, 25, 31)

typedef struct {
    RotationAngle rotation;
    int pivot_x;
    int pivot_y;
} TransformState;

static void clock_init_config(ClockConfig* config, int center_x, int center_y, int radius) {
    config->center_x = center_x;
    config->center_y = center_y;
    config->radius = radius;
    config->show_numbers = true;
    config->show_markers = true;
    config->rotation = ROTATION_0;
}

static void clock_init_light_theme(ClockTheme* theme) {
    theme->background = COLOR_WHITE;
    theme->foreground = COLOR_BLACK;
    theme->border = COLOR_BLACK;
    theme->hour_hand = COLOR_GRAY;
    theme->minute_hand = COLOR_RED;
    theme->second_hand = COLOR_CYAN;
    theme->marker = COLOR_GRAY;
}

static void clock_init_dark_theme(ClockTheme* theme) {
    theme->background = COLOR_BLACK;
    theme->foreground = COLOR_WHITE;
    theme->border = COLOR_WHITE;
    theme->hour_hand = COLOR_LIGHT_GRAY;
    theme->minute_hand = COLOR_RED;
    theme->second_hand = COLOR_CYAN;
    theme->marker = COLOR_LIGHT_GRAY;
}

static void push_clock_transform(GraphicsContext* gfx, const ClockConfig* config, TransformState* state) {
    state->rotation = gfx->rotation;
    state->pivot_x = gfx->pivot_x;
    state->pivot_y = gfx->pivot_y;
    gfx->rotation = config->rotation;
    gfx->pivot_x = config->center_x;
    gfx->pivot_y = config->center_y;
}

static void pop_clock_transform(GraphicsContext* gfx, const TransformState* state) {
    gfx->rotation = state->rotation;
    gfx->pivot_x = state->pivot_x;
    gfx->pivot_y = state->pivot_y;
}

static void draw_number(GraphicsContext* gfx, int x, int y, int num, u16 color) {
    const u8 numbers[10][5] = {
        {0x7, 0x5, 0x5, 0x5, 0x7},
        {0x2, 0x6, 0x2, 0x2, 0x7},
        {0x7, 0x1, 0x7, 0x4, 0x7},
        {0x7, 0x1, 0x7, 0x1, 0x7},
        {0x5, 0x5, 0x7, 0x1, 0x1},
        {0x7, 0x4, 0x7, 0x1, 0x7},
        {0x7, 0x4, 0x7, 0x5, 0x7},
        {0x7, 0x1, 0x1, 0x1, 0x1},
        {0x7, 0x5, 0x7, 0x5, 0x7},
        {0x7, 0x5, 0x7, 0x1, 0x7},
    };

    if (num < 0 || num > 9) return;

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (numbers[num][row] & (1 << (2 - col))) {
                gfx_plot(gfx, x + col * 2, y + row * 2, color);
                gfx_plot(gfx, x + col * 2 + 1, y + row * 2, color);
                gfx_plot(gfx, x + col * 2, y + row * 2 + 1, color);
                gfx_plot(gfx, x + col * 2 + 1, y + row * 2 + 1, color);
            }
        }
    }
}

static void draw_numbers(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme) {
    if (!config->show_numbers) return;

    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;

    draw_number(gfx, cx - 3, cy - r + 8, 1, theme->foreground);
    draw_number(gfx, cx + 4, cy - r + 8, 2, theme->foreground);
    draw_number(gfx, cx + r - 14, cy - 5, 3, theme->foreground);
    draw_number(gfx, cx - 3, cy + r - 18, 6, theme->foreground);
    draw_number(gfx, cx - r + 8, cy - 5, 9, theme->foreground);
}

static void draw_markers(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme) {
    if (!config->show_markers) return;

    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;

    int marker_hours[] = {1, 2, 4, 5, 7, 8, 10, 11};
    for (int i = 0; i < 8; i++) {
        float angle = (marker_hours[i] * 30 - 90) * PI / 180.0f;
        int mx = cx + (int)(cosf(angle) * (r - 12));
        int my = cy + (int)(sinf(angle) * (r - 12));
        gfx_draw_filled_rect(gfx, mx - 2, my - 2, 4, 4, theme->marker);
    }
}

static void clock_draw_face(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme) {
    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;

    TransformState state;
    push_clock_transform(gfx, config, &state);

    gfx_draw_filled_rect(gfx, cx - r - 2, cy - r - 2, r * 2 + 4, r * 2 + 4, theme->background);
    draw_numbers(gfx, config, theme);
    draw_markers(gfx, config, theme);

    pop_clock_transform(gfx, &state);
}

static void clock_draw_face_overlay(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme) {
    TransformState state;
    push_clock_transform(gfx, config, &state);

    draw_numbers(gfx, config, theme);
    draw_markers(gfx, config, theme);

    pop_clock_transform(gfx, &state);
}

static void clock_draw_hands(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme,
                             int hour, int minute, int second) {
    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;

    TransformState state;
    push_clock_transform(gfx, config, &state);

    float second_angle = (second * 6 - 90) * PI / 180.0f;
    float minute_angle = (minute * 6 + second * 0.1f - 90) * PI / 180.0f;
    float hour_angle = ((hour % 12) * 30 + minute * 0.5f - 90) * PI / 180.0f;

    int hour_length = (int)(r * 0.45f);
    int hour_x = cx + (int)(cosf(hour_angle) * hour_length);
    int hour_y = cy + (int)(sinf(hour_angle) * hour_length);
    gfx_draw_thick_line(gfx, cx, cy, hour_x, hour_y, 6, theme->hour_hand);

    int minute_length = (int)(r * 0.65f);
    int minute_x = cx + (int)(cosf(minute_angle) * minute_length);
    int minute_y = cy + (int)(sinf(minute_angle) * minute_length);
    gfx_draw_thick_line(gfx, cx, cy, minute_x, minute_y, 5, theme->minute_hand);

    int second_length = (int)(r * 0.75f);
    int second_x = cx + (int)(cosf(second_angle) * second_length);
    int second_y = cy + (int)(sinf(second_angle) * second_length);
    gfx_draw_line(gfx, cx, cy, second_x, second_y, theme->second_hand);

    gfx_draw_filled_rect(gfx, cx - 2, cy - 2, 5, 5, theme->foreground);

    pop_clock_transform(gfx, &state);
}

static void clock_erase_hands(GraphicsContext* gfx, const ClockConfig* config, const ClockTheme* theme,
                              int hour, int minute, int second) {
    if (hour < 0 || minute < 0 || second < 0) {
        return;
    }

    int cx = config->center_x;
    int cy = config->center_y;
    int r = config->radius;

    TransformState state;
    push_clock_transform(gfx, config, &state);

    float second_angle = (second * 6 - 90) * PI / 180.0f;
    float minute_angle = (minute * 6 + second * 0.1f - 90) * PI / 180.0f;
    float hour_angle = ((hour % 12) * 30 + minute * 0.5f - 90) * PI / 180.0f;

    int hour_length = (int)(r * 0.45f);
    int hour_x = cx + (int)(cosf(hour_angle) * hour_length);
    int hour_y = cy + (int)(sinf(hour_angle) * hour_length);
    gfx_draw_thick_line(gfx, cx, cy, hour_x, hour_y, 6, theme->background);

    int minute_length = (int)(r * 0.65f);
    int minute_x = cx + (int)(cosf(minute_angle) * minute_length);
    int minute_y = cy + (int)(sinf(minute_angle) * minute_length);
    gfx_draw_thick_line(gfx, cx, cy, minute_x, minute_y, 5, theme->background);

    int second_length = (int)(r * 0.75f);
    int second_x = cx + (int)(cosf(second_angle) * second_length);
    int second_y = cy + (int)(sinf(second_angle) * second_length);
    gfx_draw_line(gfx, cx, cy, second_x, second_y, theme->background);

    gfx_draw_filled_rect(gfx, cx - 2, cy - 2, 5, 5, theme->background);

    pop_clock_transform(gfx, &state);
}

static void clock_widget_reset(ClockWidgetState* state) {
    if (!state) return;

    state->last_second = -1;
    state->drawn_hour = -1;
    state->drawn_minute = -1;
    state->drawn_second = -1;
    state->face_dirty = true;
}

static void clock_widget_configure_layout(ClockWidgetState* state, bool split_mode) {
    if (!state) return;

    if (split_mode) {
        clock_init_config(&state->config, 128, 96, 70);
    } else {
        clock_init_config(&state->config, 64, 96, 55);
    }
}

ClockTheme* widget_clock_current_theme(ClockWidgetState* state, WidgetTheme theme) {
    if (!state) return NULL;
    return (theme == WIDGET_THEME_LIGHT) ? &state->light_theme : &state->dark_theme;
}

static void clock_widget_attach(Widget* widget, GraphicsContext* context) {
    (void)context;
    ClockWidgetState* state = widget_state(widget);
    clock_widget_reset(state);
    state->config.rotation = widget->rotation;
}

static void clock_widget_detach(Widget* widget) {
    ClockWidgetState* state = widget_state(widget);
    clock_widget_reset(state);
}

static void clock_widget_theme_changed(Widget* widget, WidgetTheme theme) {
    (void)theme;
    ClockWidgetState* state = widget_state(widget);
    clock_widget_reset(state);
}

static void clock_widget_rotation_changed(Widget* widget, RotationAngle rotation) {
    ClockWidgetState* state = widget_state(widget);
    state->config.rotation = rotation;
    clock_widget_reset(state);
}

static void clock_widget_layout_changed(Widget* widget, bool split_mode) {
    ClockWidgetState* state = widget_state(widget);
    clock_widget_configure_layout(state, split_mode);
    state->config.rotation = widget->rotation;
    clock_widget_reset(state);
}

static void clock_widget_time_tick(Widget* widget, const struct tm* timeinfo) {
    if (!widget || !timeinfo) return;

    ClockWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);
    if (!state || !ctx || !ctx->framebuffer) return;

    if (state->last_second == timeinfo->tm_sec && !state->face_dirty) {
        return;
    }

    ClockTheme* theme = widget_clock_current_theme(state, widget->theme);
    if (!theme) return;

    if (state->face_dirty) {
        clock_draw_face(ctx, &state->config, theme);
        state->face_dirty = false;
    } else if (state->drawn_second != -1) {
        clock_erase_hands(ctx, &state->config, theme,
                          state->drawn_hour, state->drawn_minute, state->drawn_second);
        clock_draw_face_overlay(ctx, &state->config, theme);
    }

    clock_draw_hands(ctx, &state->config, theme,
                     timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    state->drawn_hour = timeinfo->tm_hour;
    state->drawn_minute = timeinfo->tm_min;
    state->drawn_second = timeinfo->tm_sec;
    state->last_second = timeinfo->tm_sec;
}

static const WidgetOps CLOCK_WIDGET_OPS = {
    .on_attach = clock_widget_attach,
    .on_detach = clock_widget_detach,
    .on_theme_changed = clock_widget_theme_changed,
    .on_rotation_changed = clock_widget_rotation_changed,
    .on_layout_changed = clock_widget_layout_changed,
    .on_time_tick = clock_widget_time_tick,
    .on_update = NULL,
};

void widget_clock_init(Widget* widget, ClockWidgetState* state) {
    if (!widget || !state) return;

    clock_widget_configure_layout(state, false);
    clock_init_light_theme(&state->light_theme);
    clock_init_dark_theme(&state->dark_theme);
    clock_widget_reset(state);

    widget_init(widget, "Clock", state, &CLOCK_WIDGET_OPS);
}
