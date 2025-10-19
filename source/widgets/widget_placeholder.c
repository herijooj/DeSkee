#include "widgets/widget_placeholder.h"

#include <nds.h>

#define COLOR_LIGHT_FILL ARGB16(1, 20, 20, 25)
#define COLOR_DARK_FILL ARGB16(1, 6, 6, 8)
#define COLOR_BORDER ARGB16(1, 15, 15, 15)

static void placeholder_mark_dirty(PlaceholderWidgetState* state) {
    if (!state) return;
    state->dirty = true;
}

void widget_placeholder_set_bounds(PlaceholderWidgetState* state, int x, int y, int width, int height) {
    if (!state) return;
    state->x = x;
    state->y = y;
    state->width = width;
    state->height = height;
    placeholder_mark_dirty(state);
}

static void placeholder_draw(PlaceholderWidgetState* state, GraphicsContext* ctx) {
    if (!state || !ctx || !ctx->framebuffer) return;
    if (state->width <= 0 || state->height <= 0) return;

    gfx_draw_filled_rect(ctx, state->x, state->y, state->width, state->height, state->active_fill);
    gfx_draw_rect(ctx, state->x, state->y, state->width, state->height, 1, state->border_color);
    state->dirty = false;
}

static void placeholder_on_attach(Widget* widget, GraphicsContext* context) {
    (void)context;
    PlaceholderWidgetState* state = widget_state(widget);
    placeholder_mark_dirty(state);
}

static void placeholder_on_detach(Widget* widget) {
    PlaceholderWidgetState* state = widget_state(widget);
    placeholder_mark_dirty(state);
}

static void placeholder_on_theme_changed(Widget* widget, WidgetTheme theme) {
    PlaceholderWidgetState* state = widget_state(widget);
    if (!state) return;

    if (theme == WIDGET_THEME_LIGHT) {
        state->active_fill = state->light_color;
    } else {
        state->active_fill = state->dark_color;
    }
    placeholder_mark_dirty(state);
}

static void placeholder_on_layout_changed(Widget* widget, bool is_bottom) {
    (void)widget;
    (void)is_bottom;
}

static void placeholder_on_update(Widget* widget) {
    PlaceholderWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);
    if (!state || !ctx) return;
    if (!state->dirty) return;

    placeholder_draw(state, ctx);
}

static const WidgetOps PLACEHOLDER_WIDGET_OPS = {
    .on_attach = placeholder_on_attach,
    .on_detach = placeholder_on_detach,
    .on_theme_changed = placeholder_on_theme_changed,
    .on_rotation_changed = NULL,
    .on_layout_changed = placeholder_on_layout_changed,
    .on_time_tick = NULL,
    .on_update = placeholder_on_update,
};

void widget_placeholder_init(Widget* widget, PlaceholderWidgetState* state) {
    if (!widget || !state) return;

    state->x = 0;
    state->y = 0;
    state->width = 0;
    state->height = 0;
    state->light_color = COLOR_LIGHT_FILL;
    state->dark_color = COLOR_DARK_FILL;
    state->border_color = COLOR_BORDER;
    state->active_fill = state->light_color;
    state->dirty = true;

    widget_init(widget, "Placeholder", state, &PLACEHOLDER_WIDGET_OPS);
}
