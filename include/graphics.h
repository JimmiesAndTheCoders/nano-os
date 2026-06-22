#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifdef __cplusplus
extern "C" {
#endif

void put_pixel(int x, int y, unsigned int color);
void draw_rect(int x, int y, int w, int h, unsigned int color);
void fill_rect(int x, int y, int w, int h, unsigned int color);
void draw_line(int x0, int y0, int x1, int y1, unsigned int color);

#ifdef __cplusplus
}
#endif

#endif