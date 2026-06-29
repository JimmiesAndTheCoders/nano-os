#include "vga_screen.hpp"
#include "vbe.h"
#include "graphics.h"
#include "font8x8.h"

extern "C" {
    #include "util.h"
    #include "ports.h"
}

VgaScreen screen; 

VgaScreen::VgaScreen() : cursor_x(0), cursor_y(0) {}

// Private helper to render a solid horizontal line at the bottom of the 16x16 text cell
void VgaScreen::draw_vbe_cursor(int cx, int cy, unsigned int color) {
    if (vbe_info->width == 0 || !cursor_visible) return; // Respect flag
    int px_x = cx * 16;
    int px_y = cy * 16;
    for (int j = 14; j < 16; j++) {
        for (int i = 0; i < 16; i++) {
            put_pixel(px_x + i, px_y + j, color);
        }
    }
}

void VgaScreen::clear() {
    if (vbe_info->width > 0) {
        fill_rect(0, 0, vbe_info->width, vbe_info->height, bg_color);
        cursor_x = 0;
        cursor_y = 0;
        draw_vbe_cursor(cursor_x, cursor_y, 0xFFFFFFFF);
    } else {
        unsigned char* vidmem = (unsigned char*)VIDEO_ADDRESS;
        for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
            vidmem[i * 2] = ' ';
            vidmem[i * 2 + 1] = 0x0F; 
        }
        cursor_x = 0;
        cursor_y = 0;
        set_cursor_offset(0);
    }
}

void VgaScreen::print(const char* message) {
    for (int i = 0; message[i] != '\0'; i++) {
        put_char(message[i]);
    }
}

// Private helper to draw an upscaled font glyph to the GUI
static void draw_glyph(int px_x, int px_y, char c, unsigned int color) {
    if (c < 32 || c > 126) return;
    const unsigned char* glyph = font8x8[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                // Scale x2 for better readability on high-res monitors
                put_pixel(px_x + col*2, px_y + row*2, color);
                put_pixel(px_x + col*2 + 1, px_y + row*2, color);
                put_pixel(px_x + col*2, px_y + row*2 + 1, color);
                put_pixel(px_x + col*2 + 1, px_y + row*2 + 1, color);
            }
        }
    }
}

void VgaScreen::put_char(char c) {
    if (vbe_info->width > 0) {
        int max_cols = vbe_info->width / 16;
        int max_rows = vbe_info->height / 16;

        draw_vbe_cursor(cursor_x, cursor_y, bg_color); // Use bg_color

        if (c == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else if (c == '\b') {
            if (cursor_x > 0) {
                cursor_x--;
                fill_rect(cursor_x * 16, cursor_y * 16, 16, 16, bg_color); // Use bg_color
            }
        } else {
            draw_glyph(cursor_x * 16, cursor_y * 16, c, 0xFFFFFFFF);
            cursor_x++;
            if (cursor_x >= max_cols) {
                cursor_x = 0;
                cursor_y++;
            }
        }

        if (cursor_y >= max_rows) {
            int byte_size = vbe_info->height * vbe_info->pitch;
            int row_size = 16 * vbe_info->pitch;
            memory_copy((const char*)(vbe_info->framebuffer + row_size), 
                        (char*)(vbe_info->framebuffer), 
                        byte_size - row_size);
            fill_rect(0, (max_rows - 1) * 16, vbe_info->width, 16, bg_color); // Use bg_color
            cursor_y--;
        }

        draw_vbe_cursor(cursor_x, cursor_y, 0xFFFFFFFF);
        return;
    }

    // --- VGA Text Mode Fallback ---
    unsigned char* vidmem = (unsigned char*)VIDEO_ADDRESS;
    int offset = get_cursor_offset();

    if (c == '\n') {
        int row = offset / (2 * MAX_COLS);
        offset = get_offset(0, row + 1);
    } else if (c == '\b') {
        if (offset > 0) {
            offset -= 2;
            vidmem[offset] = ' ';
            vidmem[offset + 1] = 0x0F;
        }
    } else {
        vidmem[offset] = c;
        vidmem[offset + 1] = 0x0F;
        offset += 2;
    }

    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        for (int i = 1; i < MAX_ROWS; i++) {
            memory_copy((const char*)(get_offset(0, i) + VIDEO_ADDRESS),
                        (char*)(get_offset(0, i - 1) + VIDEO_ADDRESS),
                        MAX_COLS * 2);
        }
        unsigned char* last_line = (unsigned char*)(get_offset(0, MAX_ROWS - 1) + VIDEO_ADDRESS);
        for (int i = 0; i < MAX_COLS; i++) {
            last_line[i * 2] = ' ';
            last_line[i * 2 + 1] = 0x0F; 
        }
        offset = get_offset(0, MAX_ROWS - 1);
    }
    set_cursor_offset(offset);
}

int VgaScreen::get_cursor_offset() {
    if (vbe_info->width > 0) return 0; // Hardware cursor doesn't exist in VBE
    port_byte_out(0x3d4, 14);
    int offset = port_byte_in(0x3d5) << 8;
    port_byte_out(0x3d4, 15);
    offset += port_byte_in(0x3d5);
    return offset * 2;
}

void VgaScreen::set_cursor_offset(int offset) {
    if (vbe_info->width > 0) return; // Hardware cursor doesn't exist in VBE
    offset /= 2;
    port_byte_out(0x3d4, 14);
    port_byte_out(0x3d5, (unsigned char)(offset >> 8));
    port_byte_out(0x3d4, 15);
    port_byte_out(0x3d5, (unsigned char)(offset & 0xff));
}

void VgaScreen::set_cursor(int col, int row) {
    if (vbe_info->width > 0) {
        draw_vbe_cursor(cursor_x, cursor_y, bg_color); // Use bg_color
        cursor_x = col;
        cursor_y = row;
        draw_vbe_cursor(cursor_x, cursor_y, 0xFFFFFFFF);
    } else {
        set_cursor_offset(get_offset(col, row));
    }
}

extern "C" {
    void print(const char* message) { screen.print(message); }
    void clear_screen() { screen.clear(); }
    void print_at(const char* message, int col, int row) {
        if (col >= 0 && row >= 0) screen.set_cursor(col, row);
        screen.print(message);
    }
}
