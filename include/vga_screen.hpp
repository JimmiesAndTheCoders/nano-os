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
    
    // Add this line:
    void set_cursor(int col, int row);

private:
    void set_cursor_offset(int offset);
    int get_cursor_offset();
    int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }

    int cursor_x;
    int cursor_y;
};

extern VgaScreen screen;

#endif
#endif