#ifndef VBE_H
#define VBE_H

/* Struct representing the VBE Mode Info block exactly as returned by BIOS */
typedef struct __attribute__((packed)) {
    unsigned short attributes;
    unsigned char  window_a;
    unsigned char  window_b;
    unsigned short granularity;
    unsigned short window_size;
    unsigned short segment_a;
    unsigned short segment_b;
    unsigned int   win_func_ptr;
    unsigned short pitch;          // Bytes per horizontal line
    unsigned short width;          // Width in pixels
    unsigned short height;         // Height in pixels
    unsigned char  w_char;
    unsigned char  y_char;
    unsigned char  planes;
    unsigned char  bpp;            // Bits per pixel
    unsigned char  banks;
    unsigned char  memory_model;
    unsigned char  bank_size;
    unsigned char  image_pages;
    unsigned char  reserved0;
    unsigned char  red_mask;
    unsigned char  red_position;
    unsigned char  green_mask;
    unsigned char  green_position;
    unsigned char  blue_mask;
    unsigned char  blue_position;
    unsigned char  reserved_mask;
    unsigned char  reserved_position;
    unsigned char  direct_color_attributes;
    unsigned int   framebuffer;    // Physical Address of the Linear Framebuffer
    unsigned int   off_screen_mem_off;
    unsigned short off_screen_mem_size;
    unsigned char  reserved1[206];
} vbe_mode_info_t;

/* Tell the compiler that vbe_info exists, but it's defined elsewhere */
extern vbe_mode_info_t* vbe_info;

#endif