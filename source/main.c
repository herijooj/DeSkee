#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "graphics.h"
#include "grid.h"
#include "widgets/widget.h"
#include "widgets/widget_clock.h"
#include "widgets/widget_calendar.h"
#include "widgets/widget_visualizer.h"
#include "widgets/widget_battery.h"
#include "widgets/widget_draw.h"

typedef enum {
    THEME_LIGHT,
    THEME_DARK
} Theme;

typedef struct {
    Theme theme;
    RotationAngle rotation;
    GraphicsContext gfx_top;
    GraphicsContext gfx_bottom;
    GridLayout grid;
    int clock_slot;
    int calendar_slot;
    int visualizer_slot;
    int battery_slot;
    int draw_slot;
    Widget clock_widget;
    ClockWidgetState clock_state;
    Widget calendar_widget;
    CalendarWidgetState calendar_state;
    Widget visualizer_widget;
    VisualizerWidgetState visualizer_state;
    Widget battery_widget;
    BatteryWidgetState battery_state;
    Widget draw_widget;
    DrawWidgetState draw_state;
    int last_second;
} AppContext;

static int bottom_bg_id = -1;

static WidgetTheme app_widget_theme(const AppContext* app) {
    return (app->theme == THEME_LIGHT) ? WIDGET_THEME_LIGHT : WIDGET_THEME_DARK;
}

static void app_set_rotation(AppContext* app, RotationAngle rotation);
static void app_apply_theme(AppContext* app);
static void app_apply_full_layout(AppContext* app);

static void app_force_time_refresh(AppContext* app) {
    app->last_second = -1;
}

static void app_apply_theme(AppContext* app) {
    WidgetTheme widget_theme = app_widget_theme(app);
    ClockTheme* clock_theme = widget_clock_current_theme(&app->clock_state, widget_theme);

    if (clock_theme) {
        gfx_clear(&app->gfx_top, clock_theme->background);
        /* Also clear bottom framebuffer so areas outside widgets use the theme background
           (prevents leftover black from the bitmap background). */
        gfx_clear(&app->gfx_bottom, clock_theme->background);
        bgUpdate();
    }

    widget_set_theme(&app->clock_widget, widget_theme);
    widget_set_theme(&app->calendar_widget, widget_theme);
    widget_set_theme(&app->visualizer_widget, widget_theme);
    widget_set_theme(&app->battery_widget, widget_theme);
    widget_set_theme(&app->draw_widget, widget_theme);

    app_force_time_refresh(app);
}

static void app_set_rotation(AppContext* app, RotationAngle rotation) {
    if (app->rotation == rotation) return;

    app->rotation = rotation;
    widget_set_rotation(&app->clock_widget, rotation);
    widget_set_rotation(&app->calendar_widget, rotation);
    widget_set_rotation(&app->visualizer_widget, rotation);
    widget_set_rotation(&app->battery_widget, rotation);
    widget_set_rotation(&app->draw_widget, rotation);

    app_apply_full_layout(app);
    app_apply_theme(app);
}

static void app_apply_grid_item(AppContext* app, GridItem* item) {
    if (!app || !item || !item->widget) return;

    GridScreen screen = grid_item_screen(item);
    GraphicsContext* target = (screen == GRID_SCREEN_TOP) ? &app->gfx_top : &app->gfx_bottom;

    if (widget_context(item->widget) != target) {
        if (widget_context(item->widget)) {
            widget_detach(item->widget);
        }
        widget_attach(item->widget, target);
    }

    widget_set_rotation(item->widget, app->rotation);

    int x = 0, y = 0, w = 0, h = 0;
    grid_item_screen_rect(&app->grid, item, &x, &y, &w, &h);

    if (item->widget == &app->clock_widget) {
        int margin = (w > 40 && h > 40) ? 2 : 0;
        int bx = x + margin;
        int by = y + margin;
        int bw = w - margin * 2;
        int bh = h - margin * 2;
        if (bw <= 0 || bh <= 0) {
            bx = x;
            by = y;
            bw = w;
            bh = h;
        }
        widget_clock_set_bounds(&app->clock_state, bx, by, bw, bh);
    } else if (item->widget == &app->calendar_widget) {
        int margin = (w > 40 && h > 40) ? 2 : 0;
        int bx = x + margin;
        int by = y + margin;
        int bw = w - margin * 2;
        int bh = h - margin * 2;
        if (bw <= 0 || bh <= 0) {
            bx = x;
            by = y;
            bw = w;
            bh = h;
        }
        widget_calendar_set_bounds(&app->calendar_state, bx, by, bw, bh);
    } else if (item->widget == &app->visualizer_widget) {
        int margin = (w > 32 && h > 32) ? 2 : 0;
        int bx = x + margin;
        int by = y + margin;
        int bw = w - margin * 2;
        int bh = h - margin * 2;
        if (bw <= 0) {
            bx = x;
            bw = w;
        }
        if (bh <= 0) {
            by = y;
            bh = h;
        }
        widget_visualizer_set_bounds(&app->visualizer_state, bx, by, bw, bh);
    } else if (item->widget == &app->battery_widget) {
        int margin = (w > 40 && h > 40) ? 2 : 0;
        int bw = w - margin * 2;
        int bh = h - margin * 2;
        int bx = x + margin;
        int by = y + margin;
        if (bw <= 0 || bh <= 0) {
            bx = x;
            by = y;
            bw = w;
            bh = h;
        }
        widget_battery_set_bounds(&app->battery_state, bx, by, bw, bh);
    } else if (item->widget == &app->draw_widget) {
        widget_draw_set_bounds(&app->draw_state, x, y, w, h);
    }

    widget_set_split_mode(item->widget, screen == GRID_SCREEN_BOTTOM);
}

static void app_apply_full_layout(AppContext* app) {
    if (!app) return;
    for (int i = 0; i < app->grid.count; ++i) {
        app_apply_grid_item(app, &app->grid.items[i]);
    }
    app_force_time_refresh(app);
}

static void app_toggle_theme(AppContext* app) {
    app->theme = (app->theme == THEME_LIGHT) ? THEME_DARK : THEME_LIGHT;
    app_apply_theme(app);
}

static void app_handle_time_tick(AppContext* app, struct tm* timeinfo) {
    if (!timeinfo) return;

    widget_time_tick(&app->clock_widget, timeinfo);
    widget_time_tick(&app->calendar_widget, timeinfo);
    widget_time_tick(&app->battery_widget, timeinfo);

    app->last_second = timeinfo->tm_sec;
}

static void app_update_widgets(AppContext* app) {
    widget_update(&app->clock_widget);
    widget_update(&app->calendar_widget);
    widget_update(&app->visualizer_widget);
    widget_update(&app->battery_widget);
    widget_update(&app->draw_widget);
}

static void app_init_widgets(AppContext* app) {
    if (!app) return;

    widget_clock_init(&app->clock_widget, &app->clock_state);
    widget_calendar_init(&app->calendar_widget, &app->calendar_state);
    widget_visualizer_init(&app->visualizer_widget, &app->visualizer_state);
    widget_battery_init(&app->battery_widget, &app->battery_state);
    widget_draw_init(&app->draw_widget, &app->draw_state);

    grid_init(&app->grid, app->gfx_top.width, app->gfx_top.height);

    app->clock_slot = grid_add_widget(&app->grid, &app->clock_widget, 2, 2, 0, 0, false);
    app->calendar_slot = grid_add_widget(&app->grid, &app->calendar_widget, 2, 2, 2, 0, false);
    app->battery_slot = grid_add_widget(&app->grid, &app->battery_widget, 1, 2, 4, 0, false);
    app->visualizer_slot = grid_add_widget(&app->grid, &app->visualizer_widget, 5, 2, 0, 2, false);
    app->draw_slot = grid_add_widget(&app->grid, &app->draw_widget, 5, GRID_ROWS - GRID_TOP_ROWS, 0, GRID_TOP_ROWS, false);

    widget_set_rotation(&app->clock_widget, app->rotation);
    widget_set_rotation(&app->calendar_widget, app->rotation);
    widget_set_rotation(&app->visualizer_widget, app->rotation);
    widget_set_rotation(&app->battery_widget, app->rotation);
    widget_set_rotation(&app->draw_widget, app->rotation);

    app_apply_full_layout(app);
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
        .clock_slot = -1,
        .calendar_slot = -1,
        .visualizer_slot = -1,
        .battery_slot = -1,
        .draw_slot = -1,
        .last_second = -1,
    };

    gfx_init(&app.gfx_top, framebuffer, 256, 192, ROTATION_0);
    gfx_init(&app.gfx_bottom, bottom_framebuffer, 256, 192, ROTATION_0);

    app_init_widgets(&app);

    while (1) {
        swiWaitForVBlank();
        scanKeys();

        u32 keys_down = keysDown();

        if (keys_down & KEY_START) break;

        if (keys_down & KEY_A) {
            app_toggle_theme(&app);
        }

        if (keys_down & KEY_B) {
            RotationAngle next = (RotationAngle)((app.rotation + 1) % 4);
            app_set_rotation(&app, next);
        }

        if (keys_down & KEY_X) {
            RotationAngle prev = (RotationAngle)((app.rotation + 3) % 4);
            app_set_rotation(&app, prev);
        }

        if (keys_down & KEY_Y) {
            app_set_rotation(&app, ROTATION_0);
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
    widget_detach(&app.battery_widget);
    widget_detach(&app.draw_widget);

    return 0;
}
