#ifndef SOUND_VISUALIZER_H
#define SOUND_VISUALIZER_H

#include <nds.h>
#include <stddef.h>
#include <stdbool.h>
#include "graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VISUALIZER_THEME_LIGHT = 0,
    VISUALIZER_THEME_DARK = 1
} VisualizerTheme;

#define VISUALIZER_MAX_BARS 20

typedef struct {
    GraphicsContext* ctx;
    bool mic_running;
    bool visible;
    bool force_redraw;
    bool layout_dirty;
    int num_bars;
    int spacing;
    int bar_width;
    int origin_x;
    int baseline;
    int max_height;
    int max_sample;
    int sample_rate;
    size_t mic_buffer_bytes;
    s16* mic_buffer;
    volatile bool frame_ready;
    volatile u16 target_heights[VISUALIZER_MAX_BARS];
    u16 pending_heights[VISUALIZER_MAX_BARS];
    int current_heights[VISUALIZER_MAX_BARS];
    u16 bar_colors[VISUALIZER_MAX_BARS];
    u16 highlight_colors[VISUALIZER_MAX_BARS];
    u16 background_color;
    u16 baseline_color;
    VisualizerTheme theme;
} SoundVisualizer;

void visualizer_init(SoundVisualizer* viz, GraphicsContext* ctx);
void visualizer_set_context(SoundVisualizer* viz, GraphicsContext* ctx);
void visualizer_set_theme(SoundVisualizer* viz, VisualizerTheme theme);
void visualizer_start(SoundVisualizer* viz);
void visualizer_stop(SoundVisualizer* viz);
void visualizer_force_redraw(SoundVisualizer* viz);
void visualizer_invalidate_layout(SoundVisualizer* viz);
void visualizer_update(SoundVisualizer* viz);

#ifdef __cplusplus
}
#endif

#endif // SOUND_VISUALIZER_H
