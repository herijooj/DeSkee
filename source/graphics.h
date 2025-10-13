#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <nds.h>
#include <stdbool.h>

// Rotation angles
typedef enum {
    ROTATION_0,     // No rotation (normal)
    ROTATION_90,    // 90° clockwise
    ROTATION_180,   // 180° (upside down)
    ROTATION_270    // 270° clockwise (90° counter-clockwise)
} RotationAngle;

// Graphics context for rotation-aware drawing
typedef struct {
    u16* framebuffer;
    RotationAngle rotation;  // Current rotation angle
    int width;         // Logical width (before rotation)
    int height;        // Logical height (before rotation)
    int pivot_x;       // Rotation pivot X
    int pivot_y;       // Rotation pivot Y
} GraphicsContext;

// Initialize graphics context
void gfx_init(GraphicsContext* ctx, u16* fb, int width, int height, RotationAngle rotation);

// Set rotation angle
void gfx_set_rotation(GraphicsContext* ctx, RotationAngle rotation);

// Set rotation pivot
void gfx_set_pivot(GraphicsContext* ctx, int pivot_x, int pivot_y);

// Convenience: set rotation and pivot together
void gfx_set_transform(GraphicsContext* ctx, RotationAngle rotation, int pivot_x, int pivot_y);

// Basic pixel plotting
void gfx_plot(GraphicsContext* ctx, int x, int y, u16 color);

// Drawing primitives
void gfx_draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, u16 color);
void gfx_draw_thick_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, int thickness, u16 color);
void gfx_draw_rect(GraphicsContext* ctx, int x, int y, int w, int h, int thickness, u16 color);
void gfx_draw_filled_rect(GraphicsContext* ctx, int x, int y, int w, int h, u16 color);
void gfx_clear(GraphicsContext* ctx, u16 color);

#endif // GRAPHICS_H
