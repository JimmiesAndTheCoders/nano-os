#include "keyboard.h"
#include "ports.h"
#include "cpu.h"
#include "screen.h"
#include "util.h"
#include "shell.h"

/* Command line input accumulator */
static char key_buffer[256];

/* Keyboard scancode to ASCII layout mapping */
const char sc_name[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y', 
    'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g', 
    'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '};

static void keyboard_callback(registers_t *regs) {
    unsigned char scancode = port_byte_in(0x60);
    
    /* Ignore key releases (scancodes with bit 7 set, i.e., > 0x80) */
    if (scancode & 0x80) return;

    /* Handle Backspace (Scancode 14) */
    if (scancode == 14) {
        int len = strlen(key_buffer);
        if (len > 0) {
            backspace(key_buffer); /* Pop from memory string */
            print("\b");           /* Graphically remove from screen */
        }
    } 
    /* Handle Enter (Scancode 28) */
    else if (scancode == 28) {
        print("\n");
        process_command(key_buffer); /* Hand buffer to the shell parser */
        key_buffer[0] = '\0';        /* Clear memory buffer */
    } 
    /* Handle alphanumeric characters and general typing space limits */
    else if (scancode <= 57) {
        char letter = sc_name[scancode];
        if (letter != '?') {
            /* Keep our commands within standard shell length limits */
            if (strlen(key_buffer) < 254) {
                append(key_buffer, letter);
                char str[2] = {letter, '\0'};
                print(str);
            }
        }
    }
}

void init_keyboard() {
    key_buffer[0] = '\0'; /* Ensure buffer starts fully empty */
    register_interrupt_handler(33, keyboard_callback);
}