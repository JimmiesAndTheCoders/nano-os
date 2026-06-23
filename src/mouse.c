#include "mouse.h"
#include "ports.h"
#include "cpu.h"
#include "screen.h"
#include "vbe.h"

static unsigned char mouse_cycle = 0;
static unsigned char mouse_byte[3];
static int mouse_x = 0;
static int mouse_y = 0;

static void mouse_wait(unsigned char a_type) {
    unsigned int time_out = 100000;
    if (a_type == 0) {
        // Wait for data to be readable
        while (time_out--) {
            if ((port_byte_in(0x64) & 1) == 1) return;
        }
    } else {
        // Wait for buffer to be writable
        while (time_out--) {
            if ((port_byte_in(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(unsigned char a_write) {
    mouse_wait(1);
    port_byte_out(0x64, 0xD4);
    mouse_wait(1);
    port_byte_out(0x60, a_write);
}

static unsigned char mouse_read() {
    mouse_wait(0);
    return port_byte_in(0x60);
}

static unsigned int mouse_callback(registers_t *regs) {
    unsigned char status = port_byte_in(0x64);
    // Ensure the interrupt was triggered by the mouse (bit 5)
    if (!(status & 0x20)) {
        return (unsigned int)regs;
    }

    switch (mouse_cycle) {
        case 0:
            mouse_byte[0] = port_byte_in(0x60);
            // Bit 3 of byte 0 must be 1 (alignment bit)
            if (mouse_byte[0] & 0x08) {
                mouse_cycle++;
            }
            break;
        case 1:
            mouse_byte[1] = port_byte_in(0x60);
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = port_byte_in(0x60);
            
            // Calculate 9-bit sign-extended relative coordinates
            int rel_x = mouse_byte[1] - ((mouse_byte[0] << 4) & 0x100);
            int rel_y = mouse_byte[2] - ((mouse_byte[0] << 3) & 0x100);

            mouse_x += rel_x;
            mouse_y -= rel_y; // Subtract Y, because screen Y grows downwards

            // Clamp coordinates to screen boundaries
            vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;
            int max_x = (vbe->width > 0) ? vbe->width - 1 : 79;
            int max_y = (vbe->height > 0) ? vbe->height - 1 : 24;
            
            if (mouse_x < 0) mouse_x = 0;
            if (mouse_x > max_x) mouse_x = max_x;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_y > max_y) mouse_y = max_y;

            mouse_cycle = 0;
            break;
    }
    return (unsigned int)regs; // Return unchanged ESP
}

void init_mouse() {
    unsigned char status;
    vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;

    // Center mouse coordinates based on active graphical mode
    mouse_x = (vbe->width > 0) ? vbe->width / 2 : 40;
    mouse_y = (vbe->height > 0) ? vbe->height / 2 : 12;

    // Enable Auxiliary Device
    mouse_wait(1);
    port_byte_out(0x64, 0xA8);

    // Enable IRQ12 inside the Compaq status byte
    mouse_wait(1);
    port_byte_out(0x64, 0x20);
    mouse_wait(0);
    status = (port_byte_in(0x60) | 2);
    mouse_wait(1);
    port_byte_out(0x64, 0x60);
    mouse_wait(1);
    port_byte_out(0x60, status);

    // Tell the mouse to use default settings
    mouse_write(0xF6);
    mouse_read(); // Ack

    // Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // Ack

    // Hook IRQ 12
    register_interrupt_handler(44, mouse_callback);
}

int get_mouse_x() { return mouse_x; }
int get_mouse_y() { return mouse_y; }