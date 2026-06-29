#ifndef VGA_SCREEN_HPP
#define VGA_SCREEN_HPP

#ifdef __cplusplus
extern "C" {
    #include "ports.h"
    #include "util.h"
}

class VgaScreen {
public:
    static const int MAX_ROWS = 25;
    static const int MAX_COLS = 80;
    static const unsigned int VIDEO_ADDRESS = 0xb8000;

    VgaScreen();
    void clear();
    void print(const char* message);
    void put_char(char c);
    void set_cursor(int col, int row);
    
    // New Panic-related controls
    void set_background_color(unsigned int color) { bg_color = color; }
    void show_cursor(bool visible) { cursor_visible = visible; }

private:
    void set_cursor_offset(int offset);
    int get_cursor_offset();
    int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }
    
    // Member function for GUI cursor
    void draw_vbe_cursor(int cx, int cy, unsigned int color);

    int cursor_x;
    int cursor_y;
    bool cursor_visible = true;         // Track if cursor should be drawn
    unsigned int bg_color = 0xFF000000; // Track current background color
};

extern VgaScreen screen;

#endif
#endif
