#include "keyboard.h"
#include "ports.h"
#include "cpu.h"
#include "screen.h"
#include "util.h"
#include "shell.h"
#include "task.h"

static char key_buffer[256];

const char sc_name[] = { '?', '?', '1', '2', '3', '4', '5', '6',     
    '7', '8', '9', '0', '-', '=', '?', '?', 'q', 'w', 'e', 'r', 't', 'y', 
    'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g', 
    'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '};

static int ctrl_pressed = 0;

static unsigned int keyboard_callback(registers_t *regs) {
    unsigned char scancode = port_byte_in(0x60);
    
    if (scancode == 0x1D) { ctrl_pressed = 1; return (unsigned int)regs; }
    if (scancode == 0x9D) { ctrl_pressed = 0; return (unsigned int)regs; }
    
    if (scancode & 0x80) return (unsigned int)regs;

    // --- INTERCEPT keystrokes if the cnode terminal editor is running ---
    if (shell_editor_active()) {
        shell_editor_handle_key(scancode);
        return (unsigned int)regs;
    }
    
    if (ctrl_pressed && scancode == 46) {
        task_kill_foreground();
        print("^C\n");
        print_prompt();
        key_buffer[0] = '\0';
        return (unsigned int)regs;
    }

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
    return (unsigned int)regs;
}

void init_keyboard() {
    key_buffer[0] = '\0';
    register_interrupt_handler(33, keyboard_callback);
}