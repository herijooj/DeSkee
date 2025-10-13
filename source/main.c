#include <nds.h>
#include <stdio.h>
#include <time.h>
#include "graphics.h"
#include "clock.h"
#include "calendar.h"

static PrintConsole bottom_console;
static bool console_active = true;
static int split_bg_id = -1;

static void print_instructions(void) {
    consoleSelect(&bottom_console);
    consoleClear();
    iprintf("\n\n  Clock & Calendar\n");
    iprintf("  ---------------\n\n");
    iprintf("  A: Toggle Theme\n");
    iprintf("  B: Rotate Clockwise\n");
    iprintf("  X: Rotate Counter-CW\n");
    iprintf("  Y: Reset Rotation\n");
    iprintf("  L: Toggle Split Mode\n");
    iprintf("  START: Exit\n");
}

static void enable_bottom_console(void) {
    vramSetBankC(VRAM_C_SUB_BG);
    videoSetModeSub(MODE_0_2D);
    consoleInit(&bottom_console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&bottom_console);
    print_instructions();
    console_active = true;
}

static void disable_bottom_console(void) {
    consoleClear();
    console_active = false;
    // Switch to the default console (top) to avoid writes to sub engine
    consoleSelect(NULL);
}

static void configure_layout(bool split_mode, RotationAngle rotation,
                             ClockConfig* clock_config, CalendarConfig* calendar_config) {
    if (split_mode) {
        clock_init_config(clock_config, 128, 96, 70);
        calendar_init_config(calendar_config, 78, 20, 13);
    } else {
        clock_init_config(clock_config, 64, 96, 55);
        calendar_init_config(calendar_config, 135, 41, 13);
    }

    clock_config->rotation = rotation;
    calendar_config->rotation = rotation;
}

typedef enum {
    THEME_LIGHT,
    THEME_DARK
} Theme;

int main(void) {
    // Initialize video
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    u16* framebuffer = (u16*)VRAM_A;
    
    // Set up bottom screen for controls (combined mode default)
    enable_bottom_console();
    
    // Application state
    Theme theme = THEME_LIGHT;
    RotationAngle rotation = ROTATION_0;
    bool split_mode = false;
    int last_second = -1;
    
    // Initialize graphics context
    GraphicsContext gfx_top;
    GraphicsContext gfx_bottom;
    gfx_init(&gfx_top, framebuffer, 256, 192, ROTATION_0);
    gfx_init(&gfx_bottom, NULL, 256, 192, ROTATION_0);
    
    // Initialize clock config and themes
    ClockConfig clock_config;
    ClockTheme clock_light_theme, clock_dark_theme;
    clock_init_light_theme(&clock_light_theme);
    clock_init_dark_theme(&clock_dark_theme);
    
    // Initialize calendar config and themes
    CalendarConfig calendar_config;
    CalendarTheme calendar_light_theme, calendar_dark_theme;
    calendar_init_light_theme(&calendar_light_theme, &clock_light_theme);
    calendar_init_dark_theme(&calendar_dark_theme, &clock_dark_theme);
    
    // Set positions - these stay constant, rotation handles orientation
    configure_layout(split_mode, rotation, &clock_config, &calendar_config);
    
    // Clear screen initially
    gfx_clear(&gfx_top, clock_light_theme.background);
    
    while (1) {
        swiWaitForVBlank();
        scanKeys();
        
        u32 keys = keysDown();
        
        // Exit
        if (keys & KEY_START) break;
        
        // Toggle theme
        if (keys & KEY_A) {
            theme = (theme == THEME_LIGHT) ? THEME_DARK : THEME_LIGHT;
            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            gfx_clear(&gfx_top, clock_theme->background);
            if (split_mode && gfx_bottom.framebuffer) {
                gfx_clear(&gfx_bottom, clock_theme->background);
                bgUpdate();
            }
            last_second = -1; // Force redraw
        }
        
        // Rotate clockwise - rotate both modules independently
        if (keys & KEY_B) {
            rotation = (RotationAngle)((rotation + 1) % 4);
            clock_config.rotation = rotation;
            calendar_config.rotation = rotation;
            
            // Clear screen
            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            gfx_clear(&gfx_top, clock_theme->background);
            if (split_mode && gfx_bottom.framebuffer) {
                gfx_clear(&gfx_bottom, clock_theme->background);
                bgUpdate();
            }
            
            last_second = -1; // Force redraw
        }
        
        // Rotate counter-clockwise - rotate both modules independently
        if (keys & KEY_X) {
            rotation = (RotationAngle)((rotation + 3) % 4);
            clock_config.rotation = rotation;
            calendar_config.rotation = rotation;
            
            // Clear screen
            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            gfx_clear(&gfx_top, clock_theme->background);
            if (split_mode && gfx_bottom.framebuffer) {
                gfx_clear(&gfx_bottom, clock_theme->background);
                bgUpdate();
            }
            
            last_second = -1; // Force redraw
        }
        
        // Reset rotation to 0°
        if (keys & KEY_Y) {
            rotation = ROTATION_0;
            clock_config.rotation = ROTATION_0;
            calendar_config.rotation = ROTATION_0;
            
            // Clear screen
            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            gfx_clear(&gfx_top, clock_theme->background);
            if (split_mode && gfx_bottom.framebuffer) {
                gfx_clear(&gfx_bottom, clock_theme->background);
                bgUpdate();
            }
            
            last_second = -1; // Force redraw
        }

        // Toggle split mode (clock and calendar on separate screens)
        if (keys & KEY_L) {
            split_mode = !split_mode;
            if (split_mode) {
                if (console_active) {
                    disable_bottom_console();
                }

                vramSetBankC(VRAM_C_SUB_BG);
                videoSetModeSub(MODE_5_2D);

                split_bg_id = bgInitSub(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
                bgSetPriority(split_bg_id, 0);
                bgShow(split_bg_id);

                u16* bottom_fb = bgGetGfxPtr(split_bg_id);
                gfx_init(&gfx_bottom, bottom_fb, 256, 192, ROTATION_0);
            } else {
                if (split_bg_id >= 0) {
                    bgHide(split_bg_id);
                    split_bg_id = -1;
                }

                gfx_init(&gfx_bottom, NULL, 256, 192, ROTATION_0);
                videoSetModeSub(MODE_0_2D);
                enable_bottom_console();
            }

            configure_layout(split_mode, rotation, &clock_config, &calendar_config);

            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            gfx_clear(&gfx_top, clock_theme->background);
            if (split_mode && gfx_bottom.framebuffer) {
                gfx_clear(&gfx_bottom, clock_theme->background);
                bgUpdate();
            }

            last_second = -1;
        }
        
        // Update clock and calendar every second
        time_t current = time(NULL);
        struct tm* timeinfo = localtime(&current);
        
        if (timeinfo != NULL && timeinfo->tm_sec != last_second) {
            last_second = timeinfo->tm_sec;
            
            ClockTheme* clock_theme = (theme == THEME_LIGHT) ? &clock_light_theme : &clock_dark_theme;
            CalendarTheme* calendar_theme = (theme == THEME_LIGHT) ? &calendar_light_theme : &calendar_dark_theme;
            
            GraphicsContext* calendar_ctx = split_mode ? &gfx_bottom : &gfx_top;

            clock_draw(&gfx_top, &clock_config, clock_theme, 
                      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
            calendar_draw(calendar_ctx, &calendar_config, calendar_theme, timeinfo);

            if (split_mode && gfx_bottom.framebuffer) {
                bgUpdate();
            }
        }
    }
    
    return 0;
}
