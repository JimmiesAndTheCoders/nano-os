#include "shell.h"
#include "vfs.h"
#include "screen.h"
#include "util.h"
#include "kmalloc.h"
#include "graphics.h"

static int editor_active = 0;
static char editing_filename[64];
static char edit_buffer[2048];
static int edit_len = 0;
static int editor_shift_pressed = 0;

static const char editor_sc_name_normal[] = { 
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '?', '?', 
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '?', '?', 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '
};

static const char editor_sc_name_shifted[] = { 
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '?', '?', 
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '?', '?', 
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', '?', '|', 
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '?', '?', '?', ' '
};

static void draw_editor_interface() {
    clear_screen();
    print("--------------------------------------------------------------------------------\n");
    print("  CNODE TERMINAL EDITOR v1.0   |   Editing File: ");
    print(editing_filename);
    print("\n");
    print("  [ESC]: Save & Exit           |   [F2]: Discard & Quit\n");
    print("--------------------------------------------------------------------------------\n\n");
    
    print(edit_buffer);
}

int shell_editor_active() {
    return editor_active;
}

void shell_editor_handle_key(unsigned char scancode) {
    if (scancode == 0x2A || scancode == 0x36) { // Shift pressed
        editor_shift_pressed = 1;
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) { // Shift released
        editor_shift_pressed = 0;
        return;
    }

    // Ignore other key release scancodes
    if (scancode & 0x80) {
        return;
    }

    if (scancode == 1) { // ESC: Save & Exit
        vfs_node_t* file = vfs_resolve_path(editing_filename);
        if (file) {
            vfs_write(file, 0, edit_len, (const unsigned char*)edit_buffer);
            vfs_close(file);
        }
        editor_active = 0;
        editor_shift_pressed = 0;
        clear_screen();
        print("File '");
        print(editing_filename);
        print("' saved successfully via VFS.\n\n");
        print_prompt();
    } 
    else if (scancode == 60) { // F2: Discard changes & Quit
        editor_active = 0;
        editor_shift_pressed = 0;
        clear_screen();
        print("Editor closed. Changes discarded.\n\n");
        print_prompt();
    } 
    else if (scancode == 14) { // Backspace
        if (edit_len > 0) {
            edit_buffer[--edit_len] = '\0';
            draw_editor_interface(); // Redraws screen with updated cursor position
        }
    } 
    else if (scancode == 28) { // Enter
        if (edit_len < 2040) {
            edit_buffer[edit_len++] = '\n';
            edit_buffer[edit_len] = '\0';
            draw_editor_interface();
        }
    } 
    else if (scancode <= 57) { // Printable characters
        char letter = editor_shift_pressed ? editor_sc_name_shifted[scancode] : editor_sc_name_normal[scancode];
        if (letter != '?') {
            if (edit_len < 2040) {
                edit_buffer[edit_len++] = letter;
                edit_buffer[edit_len] = '\0';
                draw_editor_interface(); // Redraws screen with updated cursor position
            }
        }
    }
}

void cnode_launch(const char* filename, const char* cur_dir) {
    char target_path[128];
    build_full_path(cur_dir, filename, target_path);
    
    vfs_node_t* file = vfs_resolve_path(target_path);
    if (!file) {
        char parent_path[128];
        get_parent_directory(target_path, parent_path);
        
        const char* just_filename = filename;
        for (int k = strlen(filename) - 1; k >= 0; k--) {
            if (filename[k] == '/') {
                just_filename = filename + k + 1;
                break;
            }
        }
        
        vfs_node_t* parent = vfs_resolve_path(parent_path);
        if (parent) {
            if (parent->create && parent->create(parent, just_filename, VFS_FILE)) {
                file = vfs_resolve_path(target_path);
            }
            vfs_close(parent);
        }
    }
    
    if (file) {
        if (file->length > 2040) file->length = 2040;
        
        unsigned int read_bytes = vfs_read(file, 0, file->length, (unsigned char*)edit_buffer);
        edit_buffer[read_bytes] = '\0';
        edit_len = read_bytes;
        
        memory_copy(target_path, editing_filename, strlen(target_path) + 1);
        editor_active = 1;
        draw_editor_interface();
        vfs_close(file);
    } else {
        print("Error: Could not open file for editing.\n");
    }
}