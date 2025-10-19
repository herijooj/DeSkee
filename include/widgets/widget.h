#ifndef WIDGET_H
#define WIDGET_H

#include <stdbool.h>
#include <time.h>

#include "graphics.h"

typedef enum {
    WIDGET_THEME_LIGHT,
    WIDGET_THEME_DARK
} WidgetTheme;

typedef struct Widget Widget;

typedef struct {
    void (*on_attach)(Widget* widget, GraphicsContext* context);
    void (*on_detach)(Widget* widget);
    void (*on_theme_changed)(Widget* widget, WidgetTheme theme);
    void (*on_rotation_changed)(Widget* widget, RotationAngle rotation);
    void (*on_layout_changed)(Widget* widget, bool split_mode);
    void (*on_time_tick)(Widget* widget, const struct tm* timeinfo);
    void (*on_update)(Widget* widget);
} WidgetOps;

struct Widget {
    const char* name;
    GraphicsContext* context;
    WidgetTheme theme;
    RotationAngle rotation;
    bool split_mode;
    void* state;
    const WidgetOps* ops;
};

static inline void widget_init(Widget* widget, const char* name, void* state, const WidgetOps* ops) {
    if (!widget) {
        return;
    }

    widget->name = name;
    widget->state = state;
    widget->ops = ops;
    widget->context = NULL;
    widget->theme = WIDGET_THEME_LIGHT;
    widget->rotation = ROTATION_0;
    widget->split_mode = false;
}

static inline void widget_attach(Widget* widget, GraphicsContext* context) {
    if (!widget) {
        return;
    }

    widget->context = context;
    if (widget->ops && widget->ops->on_attach) {
        widget->ops->on_attach(widget, context);
    }
}

static inline void widget_detach(Widget* widget) {
    if (!widget) {
        return;
    }

    if (widget->ops && widget->ops->on_detach) {
        widget->ops->on_detach(widget);
    }

    widget->context = NULL;
}

static inline void widget_set_theme(Widget* widget, WidgetTheme theme) {
    if (!widget) {
        return;
    }

    if (widget->theme == theme) {
        return;
    }

    widget->theme = theme;
    if (widget->ops && widget->ops->on_theme_changed) {
        widget->ops->on_theme_changed(widget, theme);
    }
}

static inline void widget_set_rotation(Widget* widget, RotationAngle rotation) {
    if (!widget) {
        return;
    }

    if (widget->rotation == rotation) {
        return;
    }

    widget->rotation = rotation;
    if (widget->ops && widget->ops->on_rotation_changed) {
        widget->ops->on_rotation_changed(widget, rotation);
    }
}

static inline void widget_set_split_mode(Widget* widget, bool split_mode) {
    if (!widget) {
        return;
    }

    if (widget->split_mode == split_mode) {
        return;
    }

    widget->split_mode = split_mode;
    if (widget->ops && widget->ops->on_layout_changed) {
        widget->ops->on_layout_changed(widget, split_mode);
    }
}

static inline void widget_time_tick(Widget* widget, const struct tm* timeinfo) {
    if (!widget || !widget->ops || !widget->ops->on_time_tick) {
        return;
    }

    widget->ops->on_time_tick(widget, timeinfo);
}

static inline void widget_update(Widget* widget) {
    if (!widget || !widget->ops || !widget->ops->on_update) {
        return;
    }

    widget->ops->on_update(widget);
}

static inline void* widget_state(Widget* widget) {
    return widget ? widget->state : NULL;
}

static inline GraphicsContext* widget_context(Widget* widget) {
    return widget ? widget->context : NULL;
}

#endif // WIDGET_H
