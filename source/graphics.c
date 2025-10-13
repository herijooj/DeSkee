#include "graphics.h"
#include <stdlib.h>

// Initialize graphics context
void gfx_init(GraphicsContext* ctx, u16* fb, int width, int height, RotationAngle rotation) {
    ctx->framebuffer = fb;
    ctx->rotation = rotation;
    ctx->width = width;
    ctx->height = height;
    ctx->pivot_x = width / 2;
    ctx->pivot_y = height / 2;
}

// Set rotation angle
void gfx_set_rotation(GraphicsContext* ctx, RotationAngle rotation) {
    ctx->rotation = rotation;
}

// Set rotation pivot
void gfx_set_pivot(GraphicsContext* ctx, int pivot_x, int pivot_y) {
    ctx->pivot_x = pivot_x;
    ctx->pivot_y = pivot_y;
}

// Convenience: update both rotation and pivot
void gfx_set_transform(GraphicsContext* ctx, RotationAngle rotation, int pivot_x, int pivot_y) {
    ctx->rotation = rotation;
    ctx->pivot_x = pivot_x;
    ctx->pivot_y = pivot_y;
}

// Core pixel plotting with rotation transform
void gfx_plot(GraphicsContext* ctx, int x, int y, u16 color) {
    if (!ctx->framebuffer) return;
    
    int final_x, final_y;
    
    // Rotation pivot
    int cx = ctx->pivot_x;
    int cy = ctx->pivot_y;
    
    // Translate to origin
    int rel_x = x - cx;
    int rel_y = y - cy;
    
    // Apply rotation around center
    switch (ctx->rotation) {
        case ROTATION_0:
            // No rotation
            final_x = x;
            final_y = y;
            break;
            
        case ROTATION_90:
            // 90° clockwise: (x, y) -> (-y, x)
            final_x = -rel_y + cx;
            final_y = rel_x + cy;
            break;
            
        case ROTATION_180:
            // 180°: (x, y) -> (-x, -y)
            final_x = -rel_x + cx;
            final_y = -rel_y + cy;
            break;
            
        case ROTATION_270:
            // 270° clockwise (90° counter-clockwise): (x, y) -> (y, -x)
            final_x = rel_y + cx;
            final_y = -rel_x + cy;
            break;
            
        default:
            final_x = x;
            final_y = y;
            break;
    }
    
    // Bounds check
    if (final_x < 0 || final_x >= 256 || final_y < 0 || final_y >= 192) {
        return;
    }
    
    ctx->framebuffer[final_y * 256 + final_x] = color;
}

// Bresenham line algorithm
void gfx_draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, u16 color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        gfx_plot(ctx, x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Thick line (draw multiple parallel lines)
void gfx_draw_thick_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, int thickness, u16 color) {
    for (int t = -thickness/2; t <= thickness/2; t++) {
        gfx_draw_line(ctx, x0 + t, y0, x1 + t, y1, color);
        gfx_draw_line(ctx, x0, y0 + t, x1, y1 + t, color);
    }
}

// Draw rectangle outline
void gfx_draw_rect(GraphicsContext* ctx, int x, int y, int w, int h, int thickness, u16 color) {
    // Top and bottom
    for (int t = 0; t < thickness; t++) {
        for (int dx = 0; dx < w; dx++) {
            gfx_plot(ctx, x + dx, y + t, color);
            gfx_plot(ctx, x + dx, y + h - 1 - t, color);
        }
    }
    // Left and right
    for (int t = 0; t < thickness; t++) {
        for (int dy = 0; dy < h; dy++) {
            gfx_plot(ctx, x + t, y + dy, color);
            gfx_plot(ctx, x + w - 1 - t, y + dy, color);
        }
    }
}

// Draw filled rectangle
void gfx_draw_filled_rect(GraphicsContext* ctx, int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            gfx_plot(ctx, x + dx, y + dy, color);
        }
    }
}

// Clear entire screen
void gfx_clear(GraphicsContext* ctx, u16 color) {
    RotationAngle saved_rotation = ctx->rotation;
    int saved_px = ctx->pivot_x;
    int saved_py = ctx->pivot_y;

    ctx->rotation = ROTATION_0;
    ctx->pivot_x = ctx->width / 2;
    ctx->pivot_y = ctx->height / 2;

    gfx_draw_filled_rect(ctx, 0, 0, ctx->width, ctx->height, color);

    ctx->rotation = saved_rotation;
    ctx->pivot_x = saved_px;
    ctx->pivot_y = saved_py;
}
