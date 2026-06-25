#include "keyboard.h"
#include "ports.h"
#include "cpu.h"
#include "screen.h"
#include "util.h"
#include "shell.h"
#include "task.h"

static char key_buffer[256];

// Unshifted Normal Layout (Scan codes 0x00 to 0x39)
static const char sc_normal[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Shifted Layout (Scan codes 0x00 to 0x39)
static const char sc_shifted[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static int shift_pressed = 0;
static int ctrl_pressed = 0;

static unsigned int keyboard_callback(registers_t *regs) {
    unsigned char scancode = port_byte_in(0x60);
    
    // 1. INTERCEPT immediately if the text editor is active.
    // This passes raw shifts and controls straight to CNode.
    if (shell_editor_active()) {
        shell_editor_handle_key(scancode);
        return (unsigned int)regs;
    }

    // 2. Otherwise, process modifiers for the standard shell prompt
    if (scancode == 0x2A || scancode == 0x36) { // Left or Right Shift Pressed
        shift_pressed = 1;
        return (unsigned int)regs;
    }
    if (scancode == 0xAA || scancode == 0xB6) { // Left or Right Shift Released
        shift_pressed = 0;
        return (unsigned int)regs;
    }
    if (scancode == 0x1D) { // Control Pressed
        ctrl_pressed = 1;
        return (unsigned int)regs;
    }
    if (scancode == 0x9D) { // Control Released
        ctrl_pressed = 0;
        return (unsigned int)regs;
    }
    
    // Ignore all other Key Released actions
    if (scancode & 0x80) {
        return (unsigned int)regs;
    }
    
    // Intercept Ctrl+C (scancode for C is 0x2E)
    if (ctrl_pressed && scancode == 0x2E) {
        task_kill_foreground();
        print("^C\n");
        print_prompt();
        key_buffer[0] = '\0';
        return (unsigned int)regs;
    }

    // Process backspace
    if (scancode == 0x0E) {
        int len = strlen(key_buffer);
        if (len > 0) {
            backspace(key_buffer);
            print("\b");
        }
        return (unsigned int)regs;
    } 
    
    // Process enter
    if (scancode == 0x1C) { 
        print("\n");
        process_command(key_buffer);
        key_buffer[0] = '\0';
        return (unsigned int)regs;
    }

    // General alphanumeric key translation
    if (scancode < sizeof(sc_normal)) {
        char letter = shift_pressed ? sc_shifted[scancode] : sc_normal[scancode];
        if (letter != 0) {
            if (strlen(key_buffer) < 254) {
                append(key_buffer, letter);
                char str[2] = {letter, '\0'};
                print(str);
            }
        }
    }
    return (unsigned int)regs;
}

void init_keyboard() {
    key_buffer[0] = '\0';
    shift_pressed = 0;
    ctrl_pressed = 0;
    register_interrupt_handler(33, keyboard_callback);
}