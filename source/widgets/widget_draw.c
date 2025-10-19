#include "widgets/widget_draw.h"

#include <nds.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_LIGHT_BACKGROUND ARGB16(1, 27, 27, 29)
#define COLOR_LIGHT_CANVAS     ARGB16(1, 31, 31, 31)
#define COLOR_LIGHT_INFO_BG    ARGB16(1, 24, 24, 27)
#define COLOR_LIGHT_BORDER     ARGB16(1, 2, 2, 4)
#define COLOR_LIGHT_TEXT       ARGB16(1, 0, 0, 0)
#define COLOR_LIGHT_PEN        ARGB16(1, 0, 0, 0)

#define COLOR_DARK_BACKGROUND  ARGB16(1, 4, 5, 7)
#define COLOR_DARK_CANVAS      ARGB16(1, 2, 2, 4)
#define COLOR_DARK_INFO_BG     ARGB16(1, 7, 8, 11)
#define COLOR_DARK_BORDER      ARGB16(1, 18, 18, 21)
#define COLOR_DARK_TEXT        ARGB16(1, 28, 28, 29)
#define COLOR_DARK_PEN         ARGB16(1, 31, 31, 31)

#define COLOR_RED               ARGB16(1, 31, 0, 0)

#define BRUSH_SIZE 3




static inline int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void draw_apply_theme(DrawWidgetState* state, WidgetTheme theme) {
    if (!state) return;

    state->theme = theme;
    if (theme == WIDGET_THEME_DARK) {
        state->background_color = COLOR_DARK_BACKGROUND;
        state->canvas_background_color = COLOR_DARK_CANVAS;
        state->info_background_color = COLOR_DARK_INFO_BG;
        state->border_color = COLOR_DARK_BORDER;
        state->text_color = COLOR_DARK_TEXT;
        state->pen_color = COLOR_DARK_PEN;
    } else {
        state->background_color = COLOR_LIGHT_BACKGROUND;
        state->canvas_background_color = COLOR_LIGHT_CANVAS;
        state->info_background_color = COLOR_LIGHT_INFO_BG;
        state->border_color = COLOR_LIGHT_BORDER;
        state->text_color = COLOR_LIGHT_TEXT;
        state->pen_color = COLOR_LIGHT_PEN;
    }
    // Changing theme should not erase the user's canvas drawing.
    // Redraw only the UI elements (info bar, border, checkboxes) so the
    // canvas pixels are preserved. Mark instructions_dirty so the
    // info area / checkboxes get updated on the next frame.
    state->instructions_dirty = true;
}

static void draw_prepare_context(GraphicsContext* ctx, DrawWidgetState* state,
                                 RotationAngle* saved_rotation, int* saved_px, int* saved_py) {
    if (!ctx || !state) return;
    if (saved_rotation) {
        *saved_rotation = ctx->rotation;
    }
    if (saved_px) {
        *saved_px = ctx->pivot_x;
    }
    if (saved_py) {
        *saved_py = ctx->pivot_y;
    }

    gfx_set_transform(ctx, state->rotation, state->x + state->width / 2, state->y + state->height / 2);
}

static void draw_restore_context(GraphicsContext* ctx, RotationAngle rotation, int pivot_x, int pivot_y) {
    if (!ctx) return;
    gfx_set_transform(ctx, rotation, pivot_x, pivot_y);
}



static void draw_render_checkbox(GraphicsContext* ctx, DrawWidgetState* state) {
    if (!ctx || !state || !ctx->framebuffer) return;
    if (state->checkbox_size <= 0) return;

    RotationAngle saved_rotation;
    int saved_px, saved_py;
    draw_prepare_context(ctx, state, &saved_rotation, &saved_px, &saved_py);

    int x = state->checkbox_x;
    int y = state->checkbox_y;
    int size = state->checkbox_size;

    gfx_draw_filled_rect(ctx, x, y, size, size, state->info_background_color);
    gfx_draw_rect(ctx, x, y, size, size, 1, state->border_color);

    if (state->erasing) {
        int inset = (size >= 6) ? 2 : 1;
        int fill_w = size - inset * 2;
        if (fill_w < 1) fill_w = 1;
        gfx_draw_filled_rect(ctx, x + inset, y + inset, fill_w, fill_w, state->text_color);
    }

    draw_restore_context(ctx, saved_rotation, saved_px, saved_py);
}

static void draw_render_clear_checkbox(GraphicsContext* ctx, DrawWidgetState* state) {
    if (!ctx || !state || !ctx->framebuffer) return;
    if (state->clear_checkbox_size <= 0) return;

    RotationAngle saved_rotation;
    int saved_px, saved_py;
    draw_prepare_context(ctx, state, &saved_rotation, &saved_px, &saved_py);

    int x = state->clear_checkbox_x;
    int y = state->clear_checkbox_y;
    int size = state->clear_checkbox_size;

    gfx_draw_filled_rect(ctx, x, y, size, size, state->info_background_color);
    gfx_draw_rect(ctx, x, y, size, size, 1, COLOR_RED);

    // Render an 'X' for the clear action (looks better than a blank box)
    int thickness = (size >= 8) ? 2 : 1;
    int inset = (size >= 6) ? 2 : 1;
    int x0 = x + inset;
    int y0 = y + inset;
    int x1 = x + size - inset - 1;
    int y1 = y + size - inset - 1;

    // Diagonal lines
    gfx_draw_thick_line(ctx, x0, y0, x1, y1, thickness, COLOR_RED);
    gfx_draw_thick_line(ctx, x0, y1, x1, y0, thickness, COLOR_RED);

    draw_restore_context(ctx, saved_rotation, saved_px, saved_py);
}

static void draw_clear_canvas(GraphicsContext* ctx, DrawWidgetState* state) {
    if (!ctx || !ctx->framebuffer || !state) return;
    RotationAngle saved_rotation;
    int saved_px, saved_py;
    draw_prepare_context(ctx, state, &saved_rotation, &saved_px, &saved_py);

    gfx_draw_filled_rect(ctx, state->x, state->y, state->width, state->height, state->background_color);

    if (state->canvas_width > 0 && state->canvas_height > 0) {
        gfx_draw_filled_rect(ctx, state->canvas_x, state->canvas_y,
                             state->canvas_width, state->canvas_height, state->canvas_background_color);
    }

    gfx_draw_filled_rect(ctx, state->x + 1, state->y + 1,
                         state->width - 2, state->info_height, state->info_background_color);

    gfx_draw_rect(ctx, state->x, state->y, state->width, state->height, 1, state->border_color);

    draw_restore_context(ctx, saved_rotation, saved_px, saved_py);
    state->last_point_valid = false;
    state->needs_full_clear = false;
    state->instructions_dirty = true;
}

static void draw_render_instructions(GraphicsContext* ctx, DrawWidgetState* state) {
    if (!ctx || !ctx->framebuffer || !state) return;
    // Redraw the info bar and border to reflect the new theme without
    // touching the canvas area itself (so drawings are preserved).
    // Info bar sits at (x+1, y+1) with height info_height inside the widget.
    if (state->info_height > 0) {
        gfx_draw_filled_rect(ctx, state->x + 1, state->y + 1,
                             state->width - 2, state->info_height,
                             state->info_background_color);
    }
    // Redraw widget border (outline). This only updates the 1px border
    // around the widget and won't erase the internal canvas contents.
    gfx_draw_rect(ctx, state->x, state->y, state->width, state->height, 1, state->border_color);

    // Draw checkboxes (erase / clear).
    draw_render_checkbox(ctx, state);
    draw_render_clear_checkbox(ctx, state);

    state->instructions_dirty = false;
}

static void draw_brush(GraphicsContext* ctx, DrawWidgetState* state, int x, int y, u16 color) {
    if (!ctx || !ctx->framebuffer || !state) return;
    int half = BRUSH_SIZE / 2;
    int min_x = clamp_int(x - half, state->canvas_x, state->canvas_x + state->canvas_width - 1);
    int max_x = clamp_int(x + half, state->canvas_x, state->canvas_x + state->canvas_width - 1);
    int min_y = clamp_int(y - half, state->canvas_y, state->canvas_y + state->canvas_height - 1);
    int max_y = clamp_int(y + half, state->canvas_y, state->canvas_y + state->canvas_height - 1);

    for (int py = min_y; py <= max_y; ++py) {
        for (int px = min_x; px <= max_x; ++px) {
            gfx_plot(ctx, px, py, color);
        }
    }
}

static void draw_stroke(GraphicsContext* ctx, DrawWidgetState* state,
                        int x0, int y0, int x1, int y1, u16 color) {
    if (!ctx || !ctx->framebuffer || !state) return;

    int min_x = state->canvas_x;
    int max_x = state->canvas_x + state->canvas_width - 1;
    int min_y = state->canvas_y;
    int max_y = state->canvas_y + state->canvas_height - 1;

    x0 = clamp_int(x0, min_x, max_x);
    y0 = clamp_int(y0, min_y, max_y);
    x1 = clamp_int(x1, min_x, max_x);
    y1 = clamp_int(y1, min_y, max_y);

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int steps = dx > dy ? dx : dy;
    if (steps == 0) {
        draw_brush(ctx, state, x0, y0, color);
        return;
    }

    for (int i = 0; i <= steps; ++i) {
        int x = x0 + (x1 - x0) * i / steps;
        int y = y0 + (y1 - y0) * i / steps;
        draw_brush(ctx, state, x, y, color);
    }
}

static bool draw_handle_touch(GraphicsContext* ctx, Widget* widget) {
    if (!ctx || !widget) return false;
    DrawWidgetState* state = widget_state(widget);
    if (!state || state->canvas_width <= 0 || state->canvas_height <= 0) return false;

    u32 held = keysHeld();
    bool touching = (held & KEY_TOUCH) != 0;
    u32 down = keysDown();
    if (down & KEY_TOUCH) {
        touching = true;
    }
    if (!touching) {
        state->last_point_valid = false;
        return false;
    }

    touchPosition touch;
    touchRead(&touch);

    int tx = touch.px;
    int ty = touch.py;

    if (down & KEY_TOUCH) {
        // Check erase checkbox
        if (state->checkbox_size > 0) {
            bool in_checkbox = (tx >= state->checkbox_x) && (tx < state->checkbox_x + state->checkbox_size) &&
                               (ty >= state->checkbox_y) && (ty < state->checkbox_y + state->checkbox_size);
            if (in_checkbox) {
                state->erasing = !state->erasing;
                state->instructions_dirty = true;
                state->last_point_valid = false;
                return true;
            }
        }
        // Check clear checkbox
        if (state->clear_checkbox_size > 0) {
            bool in_clear_checkbox = (tx >= state->clear_checkbox_x) && (tx < state->clear_checkbox_x + state->clear_checkbox_size) &&
                                     (ty >= state->clear_checkbox_y) && (ty < state->clear_checkbox_y + state->clear_checkbox_size);
            if (in_clear_checkbox) {
                draw_clear_canvas(ctx, state);
                state->last_point_valid = false;
                return true;
            }
        }
    }

    if (tx < state->canvas_x || tx >= state->canvas_x + state->canvas_width ||
        ty < state->canvas_y || ty >= state->canvas_y + state->canvas_height) {
        state->last_point_valid = false;
        return false;
    }

    u16 stroke_color = state->erasing ? state->canvas_background_color : state->pen_color;

    if (!state->last_point_valid) {
        draw_brush(ctx, state, tx, ty, stroke_color);
        state->last_point_valid = true;
        state->last_x = tx;
        state->last_y = ty;
    } else {
        draw_stroke(ctx, state, state->last_x, state->last_y, tx, ty, stroke_color);
        state->last_x = tx;
        state->last_y = ty;
    }

    return true;
}

static void draw_on_attach(Widget* widget, GraphicsContext* context) {
    (void)context;
    DrawWidgetState* state = widget_state(widget);
    if (!state) return;
    state->rotation = widget->rotation;
    state->needs_full_clear = true;
    state->instructions_dirty = true;
}

static void draw_on_detach(Widget* widget) {
    DrawWidgetState* state = widget_state(widget);
    if (!state) return;
    state->last_point_valid = false;
}

static void draw_on_theme_changed(Widget* widget, WidgetTheme theme) {
    DrawWidgetState* state = widget_state(widget);
    if (!state) return;
    draw_apply_theme(state, theme);
}

static void draw_on_rotation_changed(Widget* widget, RotationAngle rotation) {
    DrawWidgetState* state = widget_state(widget);
    if (!state) return;
    state->rotation = rotation;
    state->needs_full_clear = true;
}

static void draw_on_layout_changed(Widget* widget, bool split_mode) {
    (void)split_mode;
    DrawWidgetState* state = widget_state(widget);
    if (!state) return;
    state->needs_full_clear = true;
}

static void draw_on_time_tick(Widget* widget, const struct tm* timeinfo) {
    (void)widget;
    (void)timeinfo;
}

static void draw_on_update(Widget* widget) {
    DrawWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);
    if (!state || !ctx || !ctx->framebuffer) return;

    bool updated = false;

    if (state->needs_full_clear) {
        draw_clear_canvas(ctx, state);
        updated = true;
    }

    bool input_updated = false;
    if (widget->split_mode) {
        input_updated = draw_handle_touch(ctx, widget);
    }

    if (state->instructions_dirty) {
        draw_render_instructions(ctx, state);
        updated = true;
    }

    if ((updated || input_updated) && widget->split_mode) {
        bgUpdate();
    }
}

static const WidgetOps DRAW_WIDGET_OPS = {
    .on_attach = draw_on_attach,
    .on_detach = draw_on_detach,
    .on_theme_changed = draw_on_theme_changed,
    .on_rotation_changed = draw_on_rotation_changed,
    .on_layout_changed = draw_on_layout_changed,
    .on_time_tick = draw_on_time_tick,
    .on_update = draw_on_update,
};

void widget_draw_init(Widget* widget, DrawWidgetState* state) {
    if (!widget || !state) return;

    memset(state, 0, sizeof(*state));
    state->info_height = 14;
    state->theme = WIDGET_THEME_LIGHT;
    state->rotation = ROTATION_0;
    draw_apply_theme(state, WIDGET_THEME_LIGHT);
    state->needs_full_clear = true;
    state->instructions_dirty = true;

    widget_init(widget, "Canvas", state, &DRAW_WIDGET_OPS);
}

void widget_draw_set_bounds(DrawWidgetState* state, int x, int y, int width, int height) {
    if (!state) return;

    // Mirror other widgets: add a small margin so the widget is visually centered
    // inside the grid cell but still occupies the full bottom screen area.
    int margin = (width > 40 && height > 40) ? 2 : 0;
    int bx = x + margin;
    int by = y + margin;
    int bw = width - margin * 2;
    int bh = height - margin * 2;
    if (bw <= 0 || bh <= 0) {
        bx = x;
        by = y;
        bw = width;
        bh = height;
    }

    state->x = bx;
    state->y = by;
    state->width = bw;
    state->height = bh;

    if (state->info_height + 2 >= height) {
        state->info_height = (height > 4) ? (height - 2) / 2 : 0;
    }

    state->canvas_x = state->x + 1;
    state->canvas_width = state->width - 2;
    if (state->canvas_width < 1) {
        state->canvas_width = 1;
    }

    state->canvas_y = state->y + 1 + state->info_height;
    state->canvas_height = state->height - 2 - state->info_height;
    if (state->canvas_height < 1) {
        state->canvas_height = 1;
        state->canvas_y = state->y + state->height - 2;
    }
    if (state->canvas_y + state->canvas_height > state->y + state->height - 1) {
        state->canvas_height = (state->y + state->height - 1) - state->canvas_y;
        if (state->canvas_height < 1) {
            state->canvas_height = 1;
            state->canvas_y = state->y + state->height - 2;
        }
    }

    if (state->info_height >= 4) {
        int max_size = state->info_height - 2;
        if (max_size < 3) {
            max_size = 3;
        }
        int desired = state->info_height - 4;
        if (desired < 4) {
            desired = state->info_height - 2;
        }
        if (desired < 3) {
            desired = 3;
        }
        if (desired > max_size) {
            desired = max_size;
        }
        state->checkbox_size = desired;
        int info_left = state->x + 1;
        int info_width = state->width - 2;
        int effective_left = info_left + 2;
        int effective_width = info_width - 4;
        int center_x = effective_left + effective_width / 2;
        int total_boxes_width = state->checkbox_size * 2 + 4;
        int left_box_x = center_x - total_boxes_width / 2;
        state->checkbox_x = left_box_x;
        int info_top = state->y + 2;
        int centered_y = info_top + (state->info_height - state->checkbox_size) / 2;
        int info_bottom = state->y + 1 + state->info_height;
        if (centered_y + state->checkbox_size > info_bottom) {
            centered_y = info_bottom - state->checkbox_size;
        }
        if (centered_y < state->y + 1) {
            centered_y = state->y + 1;
        }
        state->checkbox_y = centered_y;

        // Clear checkbox next to erase checkbox
        state->clear_checkbox_size = state->checkbox_size;
        state->clear_checkbox_x = state->checkbox_x + state->checkbox_size + 4;
        state->clear_checkbox_y = state->checkbox_y;
    } else {
        state->checkbox_size = 0;
        state->checkbox_x = state->x + 1;
        state->checkbox_y = state->y + 1;
        state->clear_checkbox_size = 0;
        state->clear_checkbox_x = state->x + 1;
        state->clear_checkbox_y = state->y + 1;
    }

    state->needs_full_clear = true;
    state->instructions_dirty = true;
    state->last_point_valid = false;
}
