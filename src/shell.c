#include "shell.h"
#include "screen.h"
#include "util.h"
#include "initrd.h"
#include "mouse.h"
#include "pci.h"

static char current_dir[64] = "/";

/* Terminal editor status parameters */
static int editor_active = 0;
static char editing_filename[64];
static char edit_buffer[2048];
static int edit_len = 0;

static const char editor_sc_name[] = { 
    '?', '?', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '?', '?', 
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '?', '?', 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' '
};

static int strncmp_local(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

static const char* strstr_local(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack, *n = needle;
            while (*h && *n && *h == *n) {
                h++;
                n++;
            }
            if (!*n) return haystack;
        }
    }
    return 0;
}

static void build_full_path(const char* current, const char* relative, char* dest) {
    int cur_len = strlen(current);
    int rel_len = strlen(relative);
    
    if (relative[0] == '/') {
        memory_copy(relative, dest, rel_len + 1);
        return;
    }
    
    memory_copy(current, dest, cur_len);
    if (current[cur_len - 1] != '/') {
        dest[cur_len] = '/';
        memory_copy(relative, dest + cur_len + 1, rel_len + 1);
    } else {
        memory_copy(relative, dest + cur_len, rel_len + 1);
    }
}

static void get_parent_directory(const char* path, char* dest) {
    int len = strlen(path);
    if (strcmp(path, "/") == 0) {
        memory_copy("/", dest, 2);
        return;
    }
    
    memory_copy(path, dest, len + 1);
    if (len > 1 && dest[len - 1] == '/') {
        dest[len - 1] = '\0';
        len--;
    }
    
    for (int i = len - 1; i >= 0; i--) {
        if (dest[i] == '/') {
            if (i == 0) {
                dest[1] = '\0';
            } else {
                dest[i] = '\0';
            }
            return;
        }
    }
}

static void hex_to_string(unsigned int val, char* dest, int width) {
    static const char hex_chars[] = "0123456789ABCDEF";
    dest[0] = '0';
    dest[1] = 'x';
    for (int i = 0; i < width; i++) {
        dest[width + 1 - i] = hex_chars[(val >> (i * 4)) & 0x0F];
    }
    dest[width + 2] = '\0';
}

/* Redraws the terminal editor screen cleanly */
static void draw_editor_interface() {
    clear_screen();
    print("--------------------------------------------------------------------------------\n");
    print("  CNODE TERMINAL EDITOR v1.0   |   Editing File: ");
    print("\n");
    print(editing_filename);
    print("\n");
    print("  [ESC]: Save & Exit           |   [F2]: Discard & Quit\n");
    print("--------------------------------------------------------------------------------\n\n");
    
    if (edit_len > 0) {
        print(edit_buffer);
    }
}

int shell_editor_active() {
    return editor_active;
}

void shell_editor_handle_key(unsigned char scancode) {
    if (scancode == 1) { // ESC: Save & Exit
        write_file_content(editing_filename, edit_buffer, edit_len);
        editor_active = 0;
        clear_screen();
        print("File '");
        print(editing_filename);
        print("' saved successfully to RAM disk.\n\n");
        print_prompt();
    } 
    else if (scancode == 60) { // F2: Discard changes & Quit
        editor_active = 0;
        clear_screen();
        print("Editor closed. Changes discarded.\n\n");
        print_prompt();
    } 
    else if (scancode == 14) { // Backspace
        if (edit_len > 0) {
            edit_buffer[--edit_len] = '\0';
            draw_editor_interface(); // Redraw layout to keep text cursor stable
        }
    } 
    else if (scancode == 28) { // Enter key
        if (edit_len < 2046) {
            edit_buffer[edit_len++] = '\n';
            edit_buffer[edit_len] = '\0';
            draw_editor_interface();
        }
    } 
    else if (scancode <= 57) { // Char input keys
        char letter = editor_sc_name[scancode];
        if (letter != '?') {
            if (edit_len < 2046) {
                edit_buffer[edit_len++] = letter;
                edit_buffer[edit_len] = '\0';
                
                // Fast path print for single character execution efficiency
                char str[2] = {letter, '\0'};
                print(str);
            }
        }
    }
}

void print_prompt() {
    print("nano:");
    print(current_dir);
    print("> ");
}

void process_command(char *input) {
    if (strlen(input) == 0) {
        print_prompt();
        return;
    }

    if (strcmp(input, "help") == 0) {
        print("Available Commands:\n");
        print("  help         - Display this reference panel.\n");
        print("  clear        - Clear the display terminal.\n");
        print("  echo [text]  - Print a line of text.\n");
        print("  pwd          - Print the current working directory.\n");
        print("  cd [dir]     - Change directory (validated path check).\n");
        print("  ls           - List available files in this directory.\n");
        print("  cat [file]   - Display the contents of a file.\n");
        print("  grep [pat] [f]- Find lines matching a pattern in a file.\n");
        print("  touch [file] - Create an empty file on RAM disk.\n");
        print("  mkdir [dir]  - Create a directory on RAM disk.\n");
        print("  cnode [file] - Run the terminal text/code editor.\n");
        print("  pci          - List all detected PCI bus devices.\n");
        print("  pci msi-enable [index] [vector]  - Enable MSI on specified device.\n");
        print("  pci msi-disable [index]          - Disable MSI on specified device.\n");
        print("  pci msix-enable [index] [vector] - Enable MSI-X on specified device.\n");
        print("  pci msix-disable [index]         - Disable MSI-X on specified device.\n");
        print("  status       - Inspect system operational metrics.\n");
        print("  nano --status- Execute system mascot configuration check.\n");
        print("  halt         - Safely power down the hardware processor.\n");
    } 
    else if (strcmp(input, "clear") == 0) {
        clear_screen();
    } 
    else if (strcmp(input, "pwd") == 0) {
        print(current_dir);
        print("\n");
    }
    else if (strcmp(input, "echo") == 0) {
        print("\n");
    }
    else if (strncmp_local(input, "echo ", 5) == 0) {
        print(input + 5);
        print("\n");
    }
    else if (strcmp(input, "cd") == 0) {
        memory_copy("/", current_dir, 2);
    }
    else if (strncmp_local(input, "cd ", 3) == 0) {
        char *dir = input + 3;
        char target_path[64];
        
        if (strcmp(dir, "..") == 0) {
            get_parent_directory(current_dir, target_path);
        } else if (strcmp(dir, ".") == 0) {
            target_path[0] = '\0';
        } else {
            build_full_path(current_dir, dir, target_path);
        }
        
        if (target_path[0] != '\0') {
            if (directory_exists(target_path)) {
                int path_len = strlen(target_path);
                if (path_len < 64) {
                    memory_copy(target_path, current_dir, path_len + 1);
                }
            } else {
                print("Error: Directory '");
                print(dir);
                print("' not found.\n");
            }
        }
    }
    else if (strcmp(input, "ls") == 0) {
        list_files(current_dir);
    } 
    else if (strncmp_local(input, "cat ", 4) == 0) {
        if (strlen(input) > 4) {
            char* filename = input + 4; 
            char target_path[64];
            build_full_path(current_dir, filename, target_path);
            
            char* content = read_file(target_path);
            if (content) {
                print(content);
                print("\n");
            } else {
                print("Error: File '");
                print(filename);
                print("' not found.\n");
            }
        } else {
            print("Usage: cat [filename]\n");
        }
    }
    else if (strncmp_local(input, "grep ", 5) == 0) {
        char* args = input + 5;
        char* pattern = args;
        char* filename = 0;
        
        int i = 0;
        while (args[i] != '\0') {
            if (args[i] == ' ') {
                args[i] = '\0';
                filename = args + i + 1;
                break;
            }
            i++;
        }
        
        if (!filename || strlen(filename) == 0) {
            print("Usage: grep [pattern] [filename]\n");
        } else {
            char target_path[64];
            build_full_path(current_dir, filename, target_path);
            
            char* content = read_file(target_path);
            if (!content) {
                print("Error: File '");
                print(filename);
                print("' not found.\n");
            } else {
                char line_buf[256];
                int line_idx = 0;
                int char_idx = 0;
                while (1) {
                    char c = content[char_idx++];
                    if (c == '\n' || c == '\r' || c == '\0') {
                        line_buf[line_idx] = '\0';
                        if (line_idx > 0) {
                            if (strstr_local(line_buf, pattern) != 0) {
                                print(line_buf);
                                print("\n");
                            }
                        }
                        line_idx = 0;
                        if (c == '\0') break;
                    } else {
                        if (line_idx < 254) {
                            line_buf[line_idx++] = c;
                        }
                    }
                }
            }
        }
    }
    else if (strncmp_local(input, "touch ", 6) == 0) {
        char *filename = input + 6;
        char target_path[64];
        build_full_path(current_dir, filename, target_path);
        
        if (create_file(target_path, 0)) {
            print("File '");
            print(filename);
            print("' created successfully.\n");
        } else {
            print("Error: Cannot create '");
            print(filename);
            print("'. Path may exist or disk is full.\n");
        }
    }
    else if (strncmp_local(input, "mkdir ", 6) == 0) {
        char *dirname = input + 6;
        char target_path[64];
        build_full_path(current_dir, dirname, target_path);
        
        if (create_file(target_path, 1)) {
            print("Directory '");
            print(dirname);
            print("' created successfully.\n");
        } else {
            print("Error: Cannot create directory '");
            print(dirname);
            print("'. Path may exist or disk is full.\n");
        }
    }
    else if (strncmp_local(input, "cnode ", 6) == 0) {
        char* filename = input + 6;
        char target_path[64];
        build_full_path(current_dir, filename, target_path);
        
        // Auto-create file if it does not exist (mirroring Pico/Nano behavior)
        if (!read_file(target_path)) {
            if (!create_file(target_path, 0)) {
                print("Error: Could not instantiate '");
                print(filename);
                print("'.\n");
                return;
            }
        }
        
        char* content = read_file(target_path);
        int len = strlen(content);
        if (len > 2040) len = 2040; // Upper limit of local buffer
        
        memory_copy(content, edit_buffer, len);
        edit_buffer[len] = '\0';
        edit_len = len;
        
        memory_copy(target_path, editing_filename, strlen(target_path) + 1);
        editor_active = 1;
        
        draw_editor_interface();
        return; // Retain current execution status without dropping prompt until saved
    }
    else if (strcmp(input, "pci") == 0) {
        print("PCI Bus Device Enumeration:\n");
        print("Idx Bus:Slot.Func  Vendor   Device   Class Description\n");
        print("--------------------------------------------------------------------------------\n");
        int count = pci_get_device_count();
        if (count == 0) {
            print("  No PCI devices detected.\n");
        } else {
            for (int i = 0; i < count; i++) {
                pci_device_t* dev = pci_get_device(i);
                
                char bus_str[16], slot_str[16], func_str[16];
                char vend_str[16], dev_str[16], idx_str[16];
                
                itoa(i, idx_str);
                itoa(dev->bus, bus_str);
                itoa(dev->slot, slot_str);
                itoa(dev->func, func_str);
                hex_to_string(dev->vendor_id, vend_str, 4);
                hex_to_string(dev->device_id, dev_str, 4);
                
                print("[");
                print(idx_str);
                print("] ");
                print(bus_str);
                print(":");
                print(slot_str);
                print(".");
                print(func_str);
                
                print("       ");
                print(vend_str);
                print("   ");
                print(dev_str);
                print("   ");
                print(pci_class_to_string(dev->class_code));
                
                if (dev->msi_supported) {
                    print(" [MSI");
                    if (dev->msi_enabled) {
                        char vec_str[16];
                        itoa(dev->msi_vector, vec_str);
                        print(":v=");
                        print(vec_str);
                    }
                    print("]");
                }
                if (dev->msix_supported) {
                    print(" [MSI-X");
                    if (dev->msix_enabled) {
                        char vec_str[16];
                        itoa(dev->msix_vector, vec_str);
                        print(":v=");
                        print(vec_str);
                    }
                    print("]");
                }
                print("\n");
            }
        }
    }
    else if (strncmp_local(input, "pci msi-enable ", 15) == 0) {
        char* args = input + 15;
        int idx = 0, vec = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            idx = idx * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            i++;
            while (args[i] >= '0' && args[i] <= '9') {
                vec = vec * 10 + (args[i] - '0');
                i++;
            }
            pci_device_t* dev = pci_get_device(idx);
            if (!dev) {
                print("Error: Invalid device index.\n");
            } else {
                if (pci_enable_msi(dev, vec)) {
                    print("MSI enabled successfully on device ");
                    char idx_str[16], vec_str[16];
                    itoa(idx, idx_str); itoa(vec, vec_str);
                    print(idx_str);
                    print(" with vector ");
                    print(vec_str);
                    print("\n");
                } else {
                    print("Error: MSI configuration failed or unsupported.\n");
                }
            }
        } else {
            print("Usage: pci msi-enable [device_index] [vector]\n");
        }
    }
    else if (strncmp_local(input, "pci msi-disable ", 16) == 0) {
        char* args = input + 16;
        int idx = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            idx = idx * 10 + (args[i] - '0');
            i++;
        }
        pci_device_t* dev = pci_get_device(idx);
        if (!dev) {
            print("Error: Invalid device index.\n");
        } else {
            if (pci_disable_msi(dev)) {
                print("MSI disabled on device ");
                char idx_str[16];
                itoa(idx, idx_str);
                print(idx_str);
                print("\n");
            } else {
                print("Error: MSI disable failed or unsupported.\n");
            }
        }
    }
    else if (strncmp_local(input, "pci msix-enable ", 16) == 0) {
        char* args = input + 16;
        int idx = 0, vec = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            idx = idx * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            i++;
            while (args[i] >= '0' && args[i] <= '9') {
                vec = vec * 10 + (args[i] - '0');
                i++;
            }
            pci_device_t* dev = pci_get_device(idx);
            if (!dev) {
                print("Error: Invalid device index.\n");
            } else {
                if (pci_enable_msix(dev, vec, 0)) {
                    print("MSI-X enabled successfully on device ");
                    char idx_str[16], vec_str[16];
                    itoa(idx, idx_str); itoa(vec, vec_str);
                    print(idx_str);
                    print(" with vector ");
                    print(vec_str);
                    print("\n");
                } else {
                    print("Error: MSI-X configuration failed or unsupported.\n");
                }
            }
        } else {
            print("Usage: pci msix-enable [device_index] [vector]\n");
        }
    }
    else if (strncmp_local(input, "pci msix-disable ", 17) == 0) {
        char* args = input + 17;
        int idx = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            idx = idx * 10 + (args[i] - '0');
            i++;
        }
        pci_device_t* dev = pci_get_device(idx);
        if (!dev) {
            print("Error: Invalid device index.\n");
        } else {
            if (pci_disable_msix(dev)) {
                print("MSI-X disabled on device ");
                char idx_str[16];
                itoa(idx, idx_str);
                print(idx_str);
                print("\n");
            } else {
                print("Error: MSI-X disable failed or unsupported.\n");
            }
        }
    }
    else if (strcmp(input, "status") == 0) {
        print("Nano OS System Status:\n");
        print("  Core state : RUNNING (32-bit Protected Mode)\n");
        print("  Memory map : OK [Segment Limits: 4GB Flat]\n");
        print("  Interrupts : ACTIVE [IDT installed, PIC initialized]\n");
        
        char mx_str[16];
        char my_str[16];
        itoa(get_mouse_x(), mx_str);
        itoa(get_mouse_y(), my_str);
        print("  Mouse pos  : X=");
        print(mx_str);
        print(" Y=");
        print(my_str);
        print("\n");
    } 
    else if (strcmp(input, "nano --status") == 0) {
        print("      \\|/  \n");
        print("     (o o)  \n");
        print("~~oOO-(_)-OOo~~\n");
        print("Nano is currently grounded. System restricted!\n");
        print("Please contact Scratch mascot security operators.\n");
    } 
    else if (strcmp(input, "halt") == 0) {
        print("Halting the system safely. Goodbye!\n");
        __asm__ __volatile__("cli\n\thlt");
    } 
    else {
        print("Error: Unknown command '");
        print(input);
        print("'. Type 'help' for instructions.\n");
    }

    print("\n");
    print_prompt();
}