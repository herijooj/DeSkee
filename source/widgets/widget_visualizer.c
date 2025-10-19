#include "widgets/widget_visualizer.h"

#include <malloc.h>
#include <nds/arm9/sound.h>
#include <nds/interrupts.h>
#include <string.h>

#ifndef BIT
#define BIT(n) (1 << (n))
#endif

#define CLAMP(value, minv, maxv) (((value) < (minv)) ? (minv) : (((value) > (maxv)) ? (maxv) : (value)))

static SoundVisualizer* g_active_visualizer = NULL;

static inline u16 pack_color(int r, int g, int b) {
    return (u16)(RGB15(r, g, b) | BIT(15));
}

static u16 lerp_color(u16 start, u16 end, int index, int max_index) {
    int r0 = start & 0x1F;
    int g0 = (start >> 5) & 0x1F;
    int b0 = (start >> 10) & 0x1F;

    int r1 = end & 0x1F;
    int g1 = (end >> 5) & 0x1F;
    int b1 = (end >> 10) & 0x1F;

    int r = r0 + (r1 - r0) * index / max_index;
    int g = g0 + (g1 - g0) * index / max_index;
    int b = b0 + (b1 - b0) * index / max_index;

    return pack_color(r, g, b);
}

static size_t align32(size_t value) {
    return (value + 31) & ~((size_t)31);
}

static void visualizer_calculate_layout(SoundVisualizer* viz) {
    if (!viz || !viz->ctx) return;

    int width = viz->ctx->width;
    int height = viz->ctx->height;

    viz->spacing = 6;
    int total_spacing = viz->spacing * (viz->num_bars + 1);
    int available_width = width - total_spacing;
    if (available_width < viz->num_bars) {
        available_width = viz->num_bars;
    }

    viz->bar_width = available_width / viz->num_bars;
    if (viz->bar_width < 3) {
        viz->bar_width = 3;
    }

    int used_width = viz->num_bars * viz->bar_width + total_spacing;
    int leftover = width - used_width;
    viz->origin_x = viz->spacing + leftover / 2;

    viz->baseline = height - 1;
    viz->max_height = height - 6;
    if (viz->max_height < 10) {
        viz->max_height = height;
    }

    viz->layout_dirty = false;
    viz->force_redraw = true;
}

static void visualizer_build_palette(SoundVisualizer* viz) {
    if (!viz) return;

    if (viz->theme == VISUALIZER_THEME_DARK) {
        viz->background_color = pack_color(2, 5, 11);
        u16 start = RGB15(8, 10, 31);
        u16 end = RGB15(31, 10, 20);
        viz->baseline_color = pack_color(6, 8, 16);

        for (int i = 0; i < viz->num_bars; ++i) {
            viz->bar_colors[i] = lerp_color(start, end, i, viz->num_bars - 1);
            int highlight_shift = (i * 6) / (viz->num_bars - 1);
            int r = CLAMP(((viz->bar_colors[i] & 0x1F) + highlight_shift), 0, 31);
            int g = CLAMP((((viz->bar_colors[i] >> 5) & 0x1F) + highlight_shift / 2), 0, 31);
            int b = CLAMP((((viz->bar_colors[i] >> 10) & 0x1F) + highlight_shift / 3), 0, 31);
            viz->highlight_colors[i] = pack_color(r, g, b);
        }
    } else {
        viz->background_color = pack_color(24, 25, 30);
        u16 start = RGB15(6, 16, 31);
        u16 end = RGB15(20, 31, 17);
        viz->baseline_color = pack_color(18, 20, 27);

        for (int i = 0; i < viz->num_bars; ++i) {
            viz->bar_colors[i] = lerp_color(start, end, i, viz->num_bars - 1);
            int highlight_shift = 4 + (i * 4) / (viz->num_bars - 1);
            int r = CLAMP(((viz->bar_colors[i] & 0x1F) + highlight_shift), 0, 31);
            int g = CLAMP((((viz->bar_colors[i] >> 5) & 0x1F) + highlight_shift), 0, 31);
            int b = CLAMP((((viz->bar_colors[i] >> 10) & 0x1F) + highlight_shift / 2), 0, 31);
            viz->highlight_colors[i] = pack_color(r, g, b);
        }
    }

    viz->force_redraw = true;
}

static void visualizer_clear_frame(SoundVisualizer* viz) {
    if (!viz || !viz->ctx || !viz->ctx->framebuffer) return;
    gfx_clear(viz->ctx, viz->background_color);
    bgUpdate();
}

static void visualizer_draw(SoundVisualizer* viz) {
    if (!viz || !viz->ctx || !viz->ctx->framebuffer) return;

    gfx_clear(viz->ctx, viz->background_color);

    gfx_draw_filled_rect(viz->ctx, 0, CLAMP(viz->baseline - 1, 0, viz->ctx->height - 1),
                         viz->ctx->width, 2, viz->baseline_color);

    for (int i = 0; i < viz->num_bars; ++i) {
        int height = viz->current_heights[i];
        if (height <= 0) continue;

        int x = viz->origin_x + i * (viz->bar_width + viz->spacing);
        int y = viz->baseline - height + 1;
        if (y < 0) {
            height += y;
            y = 0;
        }

        gfx_draw_filled_rect(viz->ctx, x, y, viz->bar_width, height, viz->bar_colors[i]);

        int highlight_height = height / 4;
        if (highlight_height > 0) {
            gfx_draw_filled_rect(viz->ctx, x, y, viz->bar_width, highlight_height, viz->highlight_colors[i]);
        }
    }

    bgUpdate();
}

static void visualizer_reset_levels(SoundVisualizer* viz) {
    memset(viz->current_heights, 0, sizeof(viz->current_heights));
    memset((void*)viz->target_heights, 0, sizeof(viz->target_heights));
    memset(viz->pending_heights, 0, sizeof(viz->pending_heights));
    viz->frame_ready = false;
}

static void visualizer_handle_samples(SoundVisualizer* viz, s16* samples, int sample_count) {
    if (!viz || viz->num_bars <= 0 || sample_count <= 0) return;

    int chunk = sample_count / viz->num_bars;
    if (chunk <= 0) chunk = 1;

    for (int i = 0; i < viz->num_bars; ++i) {
        int start = i * chunk;
        int end = (i == viz->num_bars - 1) ? sample_count : start + chunk;
        if (start >= sample_count) {
            start = sample_count - 1;
        }
        if (end > sample_count) {
            end = sample_count;
        }
        int count = end - start;
        if (count <= 0) count = 1;

        int accum = 0;
        int peak = 0;

        for (int s = start; s < end; ++s) {
            int value = samples[s];
            if (value < 0) value = -value;
            accum += value;
            if (value > peak) peak = value;
        }

        int average = accum / count;
        int amplitude = (average * 3 + peak) / 4;
        amplitude = CLAMP(amplitude, 0, viz->max_sample);
        int height = (amplitude * viz->max_height) / viz->max_sample;
        height = CLAMP(height, 0, viz->max_height);
        viz->target_heights[i] = (u16)height;
    }

    viz->frame_ready = true;
}

static void visualizer_mic_callback(void* data, int length) {
    if (!g_active_visualizer || !g_active_visualizer->mic_running) {
        return;
    }

    DC_InvalidateRange(data, length);
    int sample_count = length / sizeof(s16);
    if (sample_count <= 0) return;

    visualizer_handle_samples(g_active_visualizer, (s16*)data, sample_count);
}

static void visualizer_init(SoundVisualizer* viz, GraphicsContext* ctx) {
    if (!viz) return;

    memset(viz, 0, sizeof(*viz));
    viz->ctx = ctx;
    viz->num_bars = VISUALIZER_MAX_BARS;
    viz->max_sample = 2048;
    viz->sample_rate = 8192;
    viz->mic_buffer_bytes = align32((size_t)(viz->sample_rate * sizeof(s16) * 2 / 30));
    viz->mic_buffer = (s16*)memalign(32, viz->mic_buffer_bytes);
    viz->theme = VISUALIZER_THEME_DARK;

    visualizer_calculate_layout(viz);
    visualizer_build_palette(viz);
    visualizer_reset_levels(viz);
    visualizer_clear_frame(viz);
}

static void visualizer_set_context(SoundVisualizer* viz, GraphicsContext* ctx) {
    if (!viz) return;
    viz->ctx = ctx;
    viz->layout_dirty = true;
    viz->force_redraw = true;
}

static void visualizer_set_theme(SoundVisualizer* viz, VisualizerTheme theme) {
    if (!viz) return;
    if (viz->theme == theme) return;

    viz->theme = theme;
    visualizer_build_palette(viz);
    viz->force_redraw = true;
}

static void visualizer_start(SoundVisualizer* viz) {
    if (!viz || !viz->ctx || !viz->ctx->framebuffer) return;

    visualizer_reset_levels(viz);
    visualizer_build_palette(viz);
    visualizer_calculate_layout(viz);
    visualizer_clear_frame(viz);

    viz->visible = true;
    viz->force_redraw = true;

    if (viz->mic_buffer && !viz->mic_running) {
        if (soundMicRecord(viz->mic_buffer, viz->mic_buffer_bytes, MicFormat_12Bit, viz->sample_rate, visualizer_mic_callback)) {
            viz->mic_running = true;
            g_active_visualizer = viz;
        } else {
            g_active_visualizer = NULL;
        }
    }
}

static void visualizer_stop(SoundVisualizer* viz) {
    if (!viz) return;

    if (viz->mic_running) {
        soundMicOff();
        viz->mic_running = false;
    }

    viz->visible = false;
    viz->frame_ready = false;
    if (g_active_visualizer == viz) {
        g_active_visualizer = NULL;
    }

    visualizer_reset_levels(viz);
}

static void visualizer_force_redraw(SoundVisualizer* viz) {
    if (!viz) return;
    viz->force_redraw = true;
}

static void visualizer_update(SoundVisualizer* viz) {
    if (!viz || !viz->visible || !viz->ctx || !viz->ctx->framebuffer) return;

    if (viz->layout_dirty) {
        visualizer_calculate_layout(viz);
    }

    if (viz->frame_ready) {
        u32 ime = enterCriticalSection();
        for (int i = 0; i < viz->num_bars; ++i) {
            viz->pending_heights[i] = viz->target_heights[i];
        }
        viz->frame_ready = false;
        leaveCriticalSection(ime);
    }

    bool updated = viz->force_redraw;

    for (int i = 0; i < viz->num_bars; ++i) {
        int target = viz->pending_heights[i];
        int current = viz->current_heights[i];
        if (target > current) {
            int diff = target - current;
            current += (diff + 2) / 3;
        } else {
            int diff = current - target;
            current -= (diff + 3) / 4;
        }
        current = CLAMP(current, 0, viz->max_height);
        if (current != viz->current_heights[i]) {
            viz->current_heights[i] = current;
            updated = true;
        }
    }

    if (!updated) {
        return;
    }

    viz->force_redraw = false;
    visualizer_draw(viz);
}

static void visualizer_widget_update_running(Widget* widget) {
    VisualizerWidgetState* state = widget_state(widget);
    GraphicsContext* ctx = widget_context(widget);

    if (!state || !state->initialized || !ctx || !ctx->framebuffer) {
        return;
    }

    if (widget->split_mode) {
        if (state->running) {
            visualizer_stop(&state->visualizer);
            state->running = false;
        }
    } else {
        if (!state->running) {
            visualizer_start(&state->visualizer);
            state->running = true;
        }
    }
}

static void visualizer_widget_attach(Widget* widget, GraphicsContext* context) {
    VisualizerWidgetState* state = widget_state(widget);
    if (!state || !context) return;

    if (!state->initialized) {
        visualizer_init(&state->visualizer, context);
        state->initialized = true;
    } else {
        visualizer_set_context(&state->visualizer, context);
    }

    visualizer_set_theme(&state->visualizer,
                         widget->theme == WIDGET_THEME_LIGHT ? VISUALIZER_THEME_LIGHT : VISUALIZER_THEME_DARK);
    visualizer_force_redraw(&state->visualizer);
    visualizer_widget_update_running(widget);
}

static void visualizer_widget_detach(Widget* widget) {
    VisualizerWidgetState* state = widget_state(widget);
    if (!state || !state->initialized) return;

    if (state->running) {
        visualizer_stop(&state->visualizer);
        state->running = false;
    }

    state->visualizer.ctx = NULL;
}

static void visualizer_widget_theme_changed(Widget* widget, WidgetTheme theme) {
    VisualizerWidgetState* state = widget_state(widget);
    if (!state || !state->initialized) return;

    visualizer_set_theme(&state->visualizer,
                         theme == WIDGET_THEME_LIGHT ? VISUALIZER_THEME_LIGHT : VISUALIZER_THEME_DARK);
    visualizer_force_redraw(&state->visualizer);
}

static void visualizer_widget_rotation_changed(Widget* widget, RotationAngle rotation) {
    (void)widget;
    (void)rotation;
}

static void visualizer_widget_layout_changed(Widget* widget, bool split_mode) {
    (void)split_mode;
    visualizer_widget_update_running(widget);
}

static void visualizer_widget_time_tick(Widget* widget, const struct tm* timeinfo) {
    (void)widget;
    (void)timeinfo;
}

static void visualizer_widget_update(Widget* widget) {
    VisualizerWidgetState* state = widget_state(widget);
    if (!state || !state->initialized || !state->running) return;

    visualizer_update(&state->visualizer);
}

static const WidgetOps VISUALIZER_WIDGET_OPS = {
    .on_attach = visualizer_widget_attach,
    .on_detach = visualizer_widget_detach,
    .on_theme_changed = visualizer_widget_theme_changed,
    .on_rotation_changed = visualizer_widget_rotation_changed,
    .on_layout_changed = visualizer_widget_layout_changed,
    .on_time_tick = visualizer_widget_time_tick,
    .on_update = visualizer_widget_update,
};

void widget_visualizer_init(Widget* widget, VisualizerWidgetState* state) {
    if (!widget || !state) return;

    state->initialized = false;
    state->running = false;
    widget_init(widget, "Visualizer", state, &VISUALIZER_WIDGET_OPS);
}
