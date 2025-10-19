#ifndef WIDGET_VISUALIZER_H
#define WIDGET_VISUALIZER_H

#include <stdbool.h>

#include "widget.h"

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

typedef struct {
    SoundVisualizer visualizer;
    bool initialized;
    bool running;
} VisualizerWidgetState;

void widget_visualizer_init(Widget* widget, VisualizerWidgetState* state);

#endif // WIDGET_VISUALIZER_H
