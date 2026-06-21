#ifndef SCREEN_H
#define SCREEN_H

// These are the "C" functions that bridge to the C++ VgaScreen object
void clear_screen();
void print(const char *message);
void print_at(const char *message, int col, int row);

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

#endif