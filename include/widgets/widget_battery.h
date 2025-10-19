#ifndef WIDGET_BATTERY_H
#define WIDGET_BATTERY_H

#include "widget.h"

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int level;            // Battery level in the 0-15 range exposed by pmGetBatteryState
    bool charging;        // True when PM_BATT_CHARGING is set
    bool dirty;           // Indicates the widget needs to redraw
    int charge_anim_position;   // Accumulates charging animation offset in pixels
    int charge_anim_counter;    // Frame counter to throttle animation speed
    u16 background_color; // Full widget background color
    u16 border_color;     // Border color for the widget and battery body
    u16 cap_color;        // Color for the battery cap block
    u16 inner_background_color; // Background color for the battery body interior
    u16 fill_high_color;
    u16 fill_medium_color;
    u16 fill_low_color;
    u16 fill_critical_color;
    u16 fill_charging_color;
} BatteryWidgetState;

void widget_battery_init(Widget* widget, BatteryWidgetState* state);
void widget_battery_set_bounds(BatteryWidgetState* state, int x, int y, int width, int height);

#endif // WIDGET_BATTERY_H
