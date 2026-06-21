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

static unsigned int keyboard_callback(registers_t *regs) {
    unsigned char scancode = port_byte_in(0x60);
    
    if (scancode & 0x80) return (unsigned int)regs; // Return current stack

    if (scancode == 14) {
        int len = strlen(key_buffer);
        if (len > 0) {
            backspace(key_buffer);
            print("\b");
        }
    } 
    else if (scancode == 28) {
        print("\n");
        process_command(key_buffer);
        key_buffer[0] = '\0';
    } 
    else if (scancode <= 57) {
        char letter = sc_name[scancode];
        if (letter != '?') {
            if (strlen(key_buffer) < 254) {
                append(key_buffer, letter);
                char str[2] = {letter, '\0'};
                print(str);
            }
        }
    }
    return (unsigned int)regs; // MUST return the stack pointer
}

void init_keyboard() {
    key_buffer[0] = '\0'; /* Ensure buffer starts fully empty */
    register_interrupt_handler(33, keyboard_callback);
}