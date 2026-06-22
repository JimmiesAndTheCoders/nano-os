#include "graphics.h"
#include "vbe.h"

// The bootloader stores the VBE Info struct at 0x5000
vbe_mode_info_t* vbe_info = (vbe_mode_info_t*)0x5000;

void put_pixel(int x, int y, unsigned int color) {
    if (x < 0 || x >= vbe_info->width || y < 0 || y >= vbe_info->height) return;

    unsigned int offset = (y * vbe_info->pitch) + (x * (vbe_info->bpp / 8));
    unsigned char* vram = (unsigned char*)vbe_info->framebuffer;

    if (vbe_info->bpp == 32) {
        *((unsigned int*)(vram + offset)) = color;
    } else if (vbe_info->bpp == 24) {
        vram[offset] = color & 0xFF;             // Blue
        vram[offset + 1] = (color >> 8) & 0xFF;  // Green
        vram[offset + 2] = (color >> 16) & 0xFF; // Red
    }
}

void fill_rect(int x, int y, int w, int h, unsigned int color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            put_pixel(i, j, color);
        }
    }
}

void draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int i = x; i < x + w; i++) put_pixel(i, y, color);
    for (int i = x; i < x + w; i++) put_pixel(i, y + h - 1, color);
    for (int j = y; j < y + h; j++) put_pixel(x, j, color);
    for (int j = y; j < y + h; j++) put_pixel(x + w - 1, j, color);
}

void draw_line(int x0, int y0, int x1, int y1, unsigned int color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}