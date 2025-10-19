#include "widgets/widget_battery.h"

#include <nds.h>
#include <nds/system.h>

#define BATTERY_CHARGE_ANIM_STEP_DELAY 4
#define BATTERY_CHARGE_LIGHTEN_STEP    6

#define COLOR_LIGHT_BACKGROUND ARGB16(1, 31, 31, 31)
#define COLOR_LIGHT_BORDER     ARGB16(1, 0, 0, 0)
#define COLOR_LIGHT_CAP        ARGB16(1, 20, 21, 24)
#define COLOR_LIGHT_INNER_BG   ARGB16(1, 18, 19, 22)
#define COLOR_LIGHT_HIGH       ARGB16(1, 10, 29, 10)
#define COLOR_LIGHT_MEDIUM     ARGB16(1, 25, 28, 8)
#define COLOR_LIGHT_LOW        ARGB16(1, 31, 20, 4)
#define COLOR_LIGHT_CRITICAL   ARGB16(1, 31, 9, 6)
#define COLOR_LIGHT_CHARGING   ARGB16(1, 12, 20, 31)

#define COLOR_DARK_BACKGROUND  ARGB16(1, 5, 6, 8)
#define COLOR_DARK_BORDER      ARGB16(1, 12, 13, 16)
#define COLOR_DARK_CAP         ARGB16(1, 10, 11, 13)
#define COLOR_DARK_INNER_BG    ARGB16(1, 7, 8, 10)
#define COLOR_DARK_HIGH        ARGB16(1, 6, 18, 6)
#define COLOR_DARK_MEDIUM      ARGB16(1, 18, 20, 4)
#define COLOR_DARK_LOW         ARGB16(1, 23, 13, 3)
#define COLOR_DARK_CRITICAL    ARGB16(1, 26, 4, 4)
#define COLOR_DARK_CHARGING    ARGB16(1, 8, 14, 29)

static void battery_mark_dirty(BatteryWidgetState* state) {
    if (!state) return;
    state->dirty = true;
}

void widget_battery_set_bounds(BatteryWidgetState* state, int x, int y, int width, int height) {
    if (!state) return;

    state->x = x;
    state->y = y;
    state->width = width;
    state->height = height;
    battery_mark_dirty(state);
}

static void battery_apply_theme(BatteryWidgetState* state, WidgetTheme theme) {
    if (!state) return;

    if (theme == WIDGET_THEME_DARK) {
        state->background_color = COLOR_DARK_BACKGROUND;
        state->border_color = COLOR_DARK_BORDER;
        state->cap_color = COLOR_DARK_CAP;
        state->inner_background_color = COLOR_DARK_INNER_BG;
        state->fill_high_color = COLOR_DARK_HIGH;
        state->fill_medium_color = COLOR_DARK_MEDIUM;
        state->fill_low_color = COLOR_DARK_LOW;
        state->fill_critical_color = COLOR_DARK_CRITICAL;
        state->fill_charging_color = COLOR_DARK_CHARGING;
    } else {
        state->background_color = COLOR_LIGHT_BACKGROUND;
        state->border_color = COLOR_LIGHT_BORDER;
        state->cap_color = COLOR_LIGHT_CAP;
        state->inner_background_color = COLOR_LIGHT_INNER_BG;
        state->fill_high_color = COLOR_LIGHT_HIGH;
        state->fill_medium_color = COLOR_LIGHT_MEDIUM;
        state->fill_low_color = COLOR_LIGHT_LOW;
        state->fill_critical_color = COLOR_LIGHT_CRITICAL;
        state->fill_charging_color = COLOR_LIGHT_CHARGING;
    }

    battery_mark_dirty(state);
}

static void battery_refresh_state(BatteryWidgetState* state) {
    if (!state) return;

    unsigned raw = pmGetBatteryState();
    int level = (int)PM_BATT_LEVEL(raw);
    bool charging = (raw & PM_BATT_CHARGING) != 0;

    if (level < 0) level = 0;
    if (level > 15) level = 15;

    bool level_changed = state->level != level;
    bool charging_changed = state->charging != charging;

    if (level_changed || charging_changed) {
        state->level = level;
        state->charging = charging;
        state->charge_anim_position = 0;
        state->charge_anim_counter = 0;
        battery_mark_dirty(state);
    }
}

static inline int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static u16 battery_active_fill_color(const BatteryWidgetState* state) {
    if (!state) return 0;
    if (state->charging) return state->fill_charging_color;

    if (state->level >= 12) {
        return state->fill_high_color;
    } else if (state->level >= 8) {
        return state->fill_medium_color;
    } else if (state->level >= 4) {
        return state->fill_low_color;
    }
    return state->fill_critical_color;
}

static void battery_draw(BatteryWidgetState* state, GraphicsContext* ctx, bool is_bottom_screen) {
    if (!state || !ctx || !ctx->framebuffer) return;
    if (state->width <= 0 || state->height <= 0) return;

    gfx_draw_filled_rect(ctx, state->x, state->y, state->width, state->height, state->background_color);
    gfx_draw_rect(ctx, state->x, state->y, state->width, state->height, 1, state->border_color);

    int margin_x = clamp_int(state->width / 10, 2, 6);
    int margin_y = clamp_int(state->height / 10, 2, 6);

    int core_width = state->width - margin_x * 2;
    int core_height = state->height - margin_y * 2;
    if (core_width <= 6 || core_height <= 6) {
        margin_x = clamp_int(state->width / 12, 1, 4);
        margin_y = clamp_int(state->height / 12, 1, 4);
        core_width = state->width - margin_x * 2;
        core_height = state->height - margin_y * 2;
    }
    if (core_width <= 4 || core_height <= 4) {
        core_width = clamp_int(core_width, 4, state->width);
        core_height = clamp_int(core_height, 4, state->height);
    }

    int body_x = state->x + margin_x;
    int body_y = state->y + margin_y;

    // For vertical orientation: cap is on top, body fills vertically
    int cap_spacing = (core_height > 16) ? 2 : 1;
    int cap_height = clamp_int(core_height / 6, 2, 8);
    if (core_height - cap_height - cap_spacing < 8) {
        cap_height = 0;
        cap_spacing = 0;
    }

    int body_height = core_height - cap_height - cap_spacing;
    if (body_height <= 0) {
        body_height = core_height;
        cap_height = 0;
        cap_spacing = 0;
    }

    gfx_draw_filled_rect(ctx, body_x, body_y + cap_height + cap_spacing, core_width, body_height, state->inner_background_color);
    gfx_draw_rect(ctx, body_x, body_y + cap_height + cap_spacing, core_width, body_height, 1, state->border_color);

    if (cap_height > 0) {
        int cap_width = clamp_int(core_width / 2, 3, core_width - 2);
        int cap_x = body_x + (core_width - cap_width) / 2;
        int cap_y = body_y;
        gfx_draw_filled_rect(ctx, cap_x, cap_y, cap_width, cap_height, state->cap_color);
        gfx_draw_rect(ctx, cap_x, cap_y, cap_width, cap_height, 1, state->border_color);
    }

    int fill_margin = clamp_int(core_width / 12, 1, 3);
    int fill_width_margin = clamp_int(body_height / 12, 1, 3);
    int inner_width = core_width - fill_margin * 2;
    int inner_height = body_height - fill_width_margin * 2;
    if (inner_width < 2) {
        inner_width = core_width - 2;
        fill_margin = clamp_int((core_width - inner_width) / 2, 0, 2);
    }
    if (inner_height < 2) {
        inner_height = body_height - 2;
        fill_width_margin = clamp_int((body_height - inner_height) / 2, 0, 2);
    }

    int fill_x = body_x + fill_margin;
    int fill_y = body_y + cap_height + cap_spacing + fill_width_margin;

    int clamped_level = clamp_int(state->level, 0, 15);
    int fill_height = (inner_height * clamped_level + 7) / 15;
    if (clamped_level > 0 && fill_height <= 0) {
        fill_height = 1;
    }
    fill_height = clamp_int(fill_height, 0, inner_height);

    // Fill grows from bottom up
    int actual_fill_y = fill_y + inner_height - fill_height;

    // Clear the interior before drawing fill to avoid artifacts
    gfx_draw_filled_rect(ctx, fill_x, fill_y, inner_width, inner_height, state->inner_background_color);

    if (fill_height > 0) {
        gfx_draw_filled_rect(ctx, fill_x, actual_fill_y, inner_width, fill_height, battery_active_fill_color(state));
    }

    gfx_draw_rect(ctx, fill_x, fill_y, inner_width, inner_height, 1, state->border_color);

    // Charging animation disabled

    if (is_bottom_screen) {
        bgUpdate();
    }

    state->dirty = false;
}

static void battery_update_animation(BatteryWidgetState* state) {
    if (!state) return;

    // Animation disabled - keep counters at zero
    state->charge_anim_counter = 0;
    state->charge_anim_position = 0;
}

static void battery_on_attach(Widget* widget, GraphicsContext* context) {
    (void)context;
    BatteryWidgetState* state = widget_state(widget);
    battery_refresh_state(state);
    battery_mark_dirty(state);
}

static void battery_on_detach(Widget* widget) {
    BatteryWidgetState* state = widget_state(widget);
    battery_mark_dirty(state);
}

static void battery_on_theme_changed(Widget* widget, WidgetTheme theme) {
    BatteryWidgetState* state = widget_state(widget);
    battery_apply_theme(state, theme);
}

static void battery_on_rotation_changed(Widget* widget, RotationAngle rotation) {
    (void)rotation;
    BatteryWidgetState* state = widget_state(widget);
    battery_mark_dirty(state);
}

static void battery_on_layout_changed(Widget* widget, bool split_mode) {
    (void)split_mode;
    BatteryWidgetState* state = widget_state(widget);
    battery_mark_dirty(state);
}

static void battery_on_time_tick(Widget* widget, const struct tm* timeinfo) {
    (void)timeinfo;
    BatteryWidgetState* state = widget_state(widget);
    battery_refresh_state(state);
}

static void battery_on_update(Widget* widget) {
    BatteryWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);
    if (!state || !ctx) return;

    battery_update_animation(state);

    if (!state->dirty) return;

    battery_draw(state, ctx, widget->split_mode);
}

static const WidgetOps BATTERY_WIDGET_OPS = {
    .on_attach = battery_on_attach,
    .on_detach = battery_on_detach,
    .on_theme_changed = battery_on_theme_changed,
    .on_rotation_changed = battery_on_rotation_changed,
    .on_layout_changed = battery_on_layout_changed,
    .on_time_tick = battery_on_time_tick,
    .on_update = battery_on_update,
};

void widget_battery_init(Widget* widget, BatteryWidgetState* state) {
    if (!widget || !state) return;

    state->x = 0;
    state->y = 0;
    state->width = 0;
    state->height = 0;
    state->level = -1;
    state->charging = false;
    state->dirty = true;
    state->charge_anim_position = 0;
    state->charge_anim_counter = 0;

    battery_apply_theme(state, WIDGET_THEME_LIGHT);
    battery_refresh_state(state);

    widget_init(widget, "Battery", state, &BATTERY_WIDGET_OPS);
}
