#include "vga_screen.hpp"

VgaScreen screen; 

VgaScreen::VgaScreen() : cursor_x(0), cursor_y(0) {}

void VgaScreen::clear() {
    unsigned char* vidmem = (unsigned char*)VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
        vidmem[i * 2] = ' ';
        vidmem[i * 2 + 1] = 0x0F; 
    }
    set_cursor_offset(0);
}

void VgaScreen::print(const char* message) {
    for (int i = 0; message[i] != '\0'; i++) {
        put_char(message[i]);
    }
}

void VgaScreen::put_char(char c) {
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

    // Improved Scrolling
    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        for (int i = 1; i < MAX_ROWS; i++) {
            memory_copy((const char*)(get_offset(0, i) + VIDEO_ADDRESS),
                        (char*)(get_offset(0, i - 1) + VIDEO_ADDRESS),
                        MAX_COLS * 2);
        }
        // Wipe the new bottom line
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
    port_byte_out(0x3d4, 14);
    int offset = port_byte_in(0x3d5) << 8;
    port_byte_out(0x3d4, 15);
    offset += port_byte_in(0x3d5);
    return offset * 2;
}

void VgaScreen::set_cursor_offset(int offset) {
    offset /= 2;
    port_byte_out(0x3d4, 14);
    port_byte_out(0x3d5, (unsigned char)(offset >> 8));
    port_byte_out(0x3d4, 15);
    port_byte_out(0x3d5, (unsigned char)(offset & 0xff));
}

void VgaScreen::set_cursor(int col, int row) {
    set_cursor_offset(get_offset(col, row));
}

extern "C" {
    void print(const char* message) { screen.print(message); }
    void clear_screen() { screen.clear(); }
    void print_at(const char* message, int col, int row) {
        if (col >= 0 && row >= 0) screen.set_cursor(col, row);
        screen.print(message);
    }
}