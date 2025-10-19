#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "graphics.h"
#include "widgets/widget.h"
#include "widgets/widget_clock.h"
#include "widgets/widget_calendar.h"
#include "widgets/widget_visualizer.h"

typedef enum {
    THEME_LIGHT,
    THEME_DARK
} Theme;

typedef struct {
    Theme theme;
    RotationAngle rotation;
    bool split_mode;
    GraphicsContext gfx_top;
    GraphicsContext gfx_bottom;
    Widget clock_widget;
    ClockWidgetState clock_state;
    Widget calendar_widget;
    CalendarWidgetState calendar_state;
    Widget visualizer_widget;
    VisualizerWidgetState visualizer_state;
    int last_second;
} AppContext;

static int bottom_bg_id = -1;

static WidgetTheme app_widget_theme(const AppContext* app) {
    return (app->theme == THEME_LIGHT) ? WIDGET_THEME_LIGHT : WIDGET_THEME_DARK;
}

static void app_set_rotation(AppContext* app, RotationAngle rotation);
static void app_apply_theme(AppContext* app);
static void app_update_layout(AppContext* app);

static void app_force_time_refresh(AppContext* app) {
    app->last_second = -1;
}

static void app_apply_theme(AppContext* app) {
    WidgetTheme widget_theme = app_widget_theme(app);
    ClockTheme* clock_theme = widget_clock_current_theme(&app->clock_state, widget_theme);

    if (clock_theme) {
        gfx_clear(&app->gfx_top, clock_theme->background);
        if (app->split_mode && app->gfx_bottom.framebuffer) {
            gfx_clear(&app->gfx_bottom, clock_theme->background);
            bgUpdate();
        }
    }

    widget_set_theme(&app->clock_widget, widget_theme);
    widget_set_theme(&app->calendar_widget, widget_theme);
    widget_set_theme(&app->visualizer_widget, widget_theme);

    app_force_time_refresh(app);
}

static void app_set_rotation(AppContext* app, RotationAngle rotation) {
    if (app->rotation == rotation) return;

    app->rotation = rotation;
    widget_set_rotation(&app->clock_widget, rotation);
    widget_set_rotation(&app->calendar_widget, rotation);
    widget_set_rotation(&app->visualizer_widget, rotation);

    app_force_time_refresh(app);
}

static void app_update_layout(AppContext* app) {
    if (app->split_mode) {
        if (widget_context(&app->visualizer_widget)) {
            widget_detach(&app->visualizer_widget);
        }
        if (widget_context(&app->calendar_widget) != &app->gfx_bottom) {
            if (widget_context(&app->calendar_widget)) {
                widget_detach(&app->calendar_widget);
            }
            widget_attach(&app->calendar_widget, &app->gfx_bottom);
        }
    } else {
        if (!widget_context(&app->visualizer_widget)) {
            widget_attach(&app->visualizer_widget, &app->gfx_bottom);
        }
        if (widget_context(&app->calendar_widget) != &app->gfx_top) {
            if (widget_context(&app->calendar_widget)) {
                widget_detach(&app->calendar_widget);
            }
            widget_attach(&app->calendar_widget, &app->gfx_top);
        }
    }

    widget_set_split_mode(&app->clock_widget, app->split_mode);
    widget_set_split_mode(&app->calendar_widget, app->split_mode);
    widget_set_split_mode(&app->visualizer_widget, app->split_mode);

    app_force_time_refresh(app);
}

static void app_toggle_theme(AppContext* app) {
    app->theme = (app->theme == THEME_LIGHT) ? THEME_DARK : THEME_LIGHT;
    app_apply_theme(app);
}

static void app_toggle_split_mode(AppContext* app) {
    app->split_mode = !app->split_mode;
    app_update_layout(app);
    app_apply_theme(app);
}

static void app_handle_time_tick(AppContext* app, struct tm* timeinfo) {
    if (!timeinfo) return;

    widget_time_tick(&app->clock_widget, timeinfo);
    widget_time_tick(&app->calendar_widget, timeinfo);

    app->last_second = timeinfo->tm_sec;
}

static void app_update_widgets(AppContext* app) {
    widget_update(&app->clock_widget);
    widget_update(&app->calendar_widget);
    widget_update(&app->visualizer_widget);
}

static void app_init_widgets(AppContext* app) {
    widget_clock_init(&app->clock_widget, &app->clock_state);
    widget_calendar_init(&app->calendar_widget, &app->calendar_state);
    widget_visualizer_init(&app->visualizer_widget, &app->visualizer_state);

    widget_attach(&app->clock_widget, &app->gfx_top);
    widget_attach(&app->calendar_widget, &app->gfx_top);
    widget_attach(&app->visualizer_widget, &app->gfx_bottom);

    widget_set_rotation(&app->clock_widget, app->rotation);
    widget_set_rotation(&app->calendar_widget, app->rotation);
    widget_set_rotation(&app->visualizer_widget, app->rotation);

    app_update_layout(app);
    app_apply_theme(app);
}

int main(void) {
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* framebuffer = (u16*)VRAM_A;

    vramSetBankC(VRAM_C_SUB_BG);
    videoSetModeSub(MODE_5_2D);
    bottom_bg_id = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(bottom_bg_id, 0);
    bgShow(bottom_bg_id);
    u16* bottom_framebuffer = bgGetGfxPtr(bottom_bg_id);

    AppContext app = {
        .theme = THEME_LIGHT,
        .rotation = ROTATION_0,
        .split_mode = false,
        .last_second = -1,
    };

    gfx_init(&app.gfx_top, framebuffer, 256, 192, ROTATION_0);
    gfx_init(&app.gfx_bottom, bottom_framebuffer, 256, 192, ROTATION_0);

    app_init_widgets(&app);

    while (1) {
        swiWaitForVBlank();
        scanKeys();

        u32 keys = keysDown();

        if (keys & KEY_START) break;

        if (keys & KEY_A) {
            app_toggle_theme(&app);
        }

        if (keys & KEY_B) {
            RotationAngle next = (RotationAngle)((app.rotation + 1) % 4);
            app_set_rotation(&app, next);
        }

        if (keys & KEY_X) {
            RotationAngle prev = (RotationAngle)((app.rotation + 3) % 4);
            app_set_rotation(&app, prev);
        }

        if (keys & KEY_Y) {
            app_set_rotation(&app, ROTATION_0);
        }

        if (keys & KEY_L) {
            app_toggle_split_mode(&app);
        }

        time_t current = time(NULL);
        struct tm* timeinfo = localtime(&current);

        if (timeinfo && timeinfo->tm_sec != app.last_second) {
            app_handle_time_tick(&app, timeinfo);
        }

        app_update_widgets(&app);
    }

    widget_detach(&app.visualizer_widget);
    widget_detach(&app.calendar_widget);
    widget_detach(&app.clock_widget);

    return 0;
}
