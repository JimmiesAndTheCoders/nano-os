#include "rtc.h"
#include "shell.h"
#include "screen.h"
#include "util.h"
#include "initrd.h"
#include "vfs.h"
#include "mouse.h"
#include "pci.h"
#include "kmalloc.h"
#include "ata.h"
#include "cache.h"

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

static void draw_editor_interface() {
    clear_screen();
    print("--------------------------------------------------------------------------------\n");
    print("  CNODE TERMINAL EDITOR v1.0   |   Editing File: ");
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
        vfs_node_t* file = vfs_resolve_path(editing_filename);
        if (file) {
            vfs_write(file, 0, edit_len, (const unsigned char*)edit_buffer);
            vfs_close(file);
        }
        editor_active = 0;
        clear_screen();
        print("File '");
        print(editing_filename);
        print("' saved successfully via VFS.\n\n");
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
            draw_editor_interface();
        }
    } 
    else if (scancode == 28) { // Enter key
        if (edit_len < 2046) {
            edit_buffer[edit_len++] = '\n';
            edit_buffer[edit_len] = '\0';
            draw_editor_interface();
        }
    } 
    else if (scancode <= 57) { // Char keys
        char letter = editor_sc_name[scancode];
        if (letter != '?') {
            if (edit_len < 2046) {
                edit_buffer[edit_len++] = letter;
                edit_buffer[edit_len] = '\0';
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
        print("  touch [file] - Create an empty file.\n");
        print("  mkdir [dir]  - Create a directory.\n");
        print("  cnode [file] - Run the terminal text/code editor.\n");
        print("  pci          - List all detected PCI bus devices.\n");
        print("  pci msi-enable [index] [vector]  - Enable MSI on specified device.\n");
        print("  pci msi-disable [index]          - Disable MSI on specified device.\n");
        print("  pci msix-enable [index] [vector] - Enable MSI-X on specified device.\n");
        print("  pci msix-disable [index]         - Disable MSI-X on specified device.\n");
        print("  ata-identify                     - Identify primary master ATA drive.\n");
        print("  ata-read [lba] [count]           - Read sectors using PIO.\n");
        print("  ata-write [lba] [text]           - Write text to sector using PIO.\n");
        print("  ata-dma-read [lba] [count]       - Read sectors using DMA.\n");
        print("  ata-dma-write [lba] [text]       - Write text to sector using DMA.\n");
        print("  date         - View current Real-Time Clock date and time.\n");
        print("  sync         - Flush all dirty pages and blocks to disk.\n");
        print("  cache-stats  - View system page and buffer cache statistics.\n");
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
        char target_path[128];
        
        if (strcmp(dir, "..") == 0) {
            get_parent_directory(current_dir, target_path);
        } else if (strcmp(dir, ".") == 0) {
            target_path[0] = '\0';
        } else {
            build_full_path(current_dir, dir, target_path);
        }
        
        if (target_path[0] != '\0') {
            vfs_node_t* node = vfs_resolve_path(target_path);
            if (node) {
                if (node->flags & VFS_DIRECTORY) {
                    int path_len = strlen(target_path);
                    if (path_len < 64) {
                        memory_copy(target_path, current_dir, path_len + 1);
                    }
                } else {
                    print("Error: '");
                    print(dir);
                    print("' is not a directory.\n");
                }
                vfs_close(node);
            } else {
                print("Error: Directory '");
                print(dir);
                print("' not found.\n");
            }
        }
    }
    else if (strcmp(input, "ls") == 0) {
        vfs_node_t* dir = vfs_resolve_path(current_dir);
        if (dir) {
            if (dir->flags & VFS_DIRECTORY) {
                struct dirent* de;
                int i = 0;
                while ((de = vfs_readdir(dir, i++)) != 0) {
                    vfs_node_t* child = vfs_finddir(dir, de->name);
                    if (child) {
                        if (child->flags & VFS_DIRECTORY) {
                            print("  <DIR>  ");
                        } else {
                            print("  <FILE> ");
                        }
                        vfs_close(child);
                    } else {
                        print("  <FILE> ");
                    }
                    print(de->name);
                    print("\n");
                }
            } else {
                print("Error: Path is not a directory.\n");
            }
            vfs_close(dir);
        } else {
            print("Error: Could not list directory.\n");
        }
    } 
    else if (strncmp_local(input, "cat ", 4) == 0) {
        if (strlen(input) > 4) {
            char* filename = input + 4; 
            char target_path[128];
            build_full_path(current_dir, filename, target_path);
            
            vfs_node_t* file = vfs_resolve_path(target_path);
            if (file) {
                if (!(file->flags & VFS_DIRECTORY)) {
                    char* content = (char*)kmalloc(file->length + 1);
                    if (content) {
                        unsigned int read_bytes = vfs_read(file, 0, file->length, (unsigned char*)content);
                        content[read_bytes] = '\0';
                        print(content);
                        print("\n");
                        kfree(content);
                    } else {
                        print("Memory allocation failed.\n");
                    }
                } else {
                    print("Error: '");
                    print(filename);
                    print("' is a directory.\n");
                }
                vfs_close(file);
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
            char target_path[128];
            build_full_path(current_dir, filename, target_path);
            
            vfs_node_t* file = vfs_resolve_path(target_path);
            if (!file) {
                print("Error: File '");
                print(filename);
                print("' not found.\n");
            } else {
                char* content = (char*)kmalloc(file->length + 1);
                if (content) {
                    vfs_read(file, 0, file->length, (unsigned char*)content);
                    content[file->length] = '\0';
                    
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
                    kfree(content);
                }
                vfs_close(file);
            }
        }
    }
    else if (strncmp_local(input, "touch ", 6) == 0) {
        char *filename = input + 6;
        char target_path[128];
        build_full_path(current_dir, filename, target_path);
        
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
            if (parent->create) {
                if (parent->create(parent, just_filename, VFS_FILE)) {
                    print("File '");
                    print(just_filename);
                    print("' created successfully.\n");
                } else {
                    print("Error: Could not create file.\n");
                }
            } else {
                print("Error: Path does not support writing.\n");
            }
            vfs_close(parent);
        } else {
            print("Error: Target parent path not found.\n");
        }
    }
    else if (strncmp_local(input, "mkdir ", 6) == 0) {
        char *dirname = input + 6;
        char target_path[128];
        build_full_path(current_dir, dirname, target_path);
        
        char parent_path[128];
        get_parent_directory(target_path, parent_path);
        
        const char* just_dirname = dirname;
        for (int k = strlen(dirname) - 1; k >= 0; k--) {
            if (dirname[k] == '/') {
                just_dirname = dirname + k + 1;
                break;
            }
        }
        
        vfs_node_t* parent = vfs_resolve_path(parent_path);
        if (parent) {
            if (parent->create) {
                if (parent->create(parent, just_dirname, VFS_DIRECTORY)) {
                    print("Directory '");
                    print(just_dirname);
                    print("' created successfully.\n");
                } else {
                    print("Error: Could not instantiate folder.\n");
                }
            } else {
                print("Error: Path does not support folder creation.\n");
            }
            vfs_close(parent);
        } else {
            print("Error: Target parent path not found.\n");
        }
    }
    else if (strncmp_local(input, "cnode ", 6) == 0) {
        char* filename = input + 6;
        char target_path[128];
        build_full_path(current_dir, filename, target_path);
        
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
            return;
        } else {
            print("Error: Could not open file for editing.\n");
        }
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
    else if (strcmp(input, "ata-identify") == 0) {
        unsigned short buf[256];
        int res = ata_identify_device(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, buf);
        if (res == 0) {
            print("ATA Primary Master identified successfully!\n");
            print("Model: ");
            char model[41];
            for (int i = 0; i < 20; i++) {
                model[i * 2] = (char)(buf[27 + i] >> 8);
                model[i * 2 + 1] = (char)(buf[27 + i] & 0xFF);
            }
            model[40] = '\0';
            print(model);
            print("\n");
            
            unsigned int sectors = *((unsigned int*)&buf[60]);
            print("Capacity: ");
            char sectors_str[16];
            itoa(sectors, sectors_str);
            print(sectors_str);
            print(" sectors (");
            itoa(sectors * 512 / 1024, sectors_str);
            print(sectors_str);
            print(" KB)\n");
        } else if (res == -1) {
            print("No device detected on Primary Master.\n");
        } else if (res == -2) {
            print("ATAPI device detected on Primary Master.\n");
        } else {
            print("Error identifying device.\n");
        }
    }
    else if (strncmp_local(input, "ata-read ", 9) == 0) {
        char* args = input + 9;
        int lba = 0, count = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            lba = lba * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            i++;
            while (args[i] >= '0' && args[i] <= '9') {
                count = count * 10 + (args[i] - '0');
                i++;
            }
            if (count > 0 && count <= 8) {
                unsigned short* sector_buf = (unsigned short*)kmalloc(count * 512);
                if (sector_buf) {
                    int res = ata_read_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, sector_buf);
                    if (res == 0) {
                        print("Read successful (PIO):\n");
                        char* char_buf = (char*)sector_buf;
                        char_buf[count * 512 - 1] = '\0';
                        print(char_buf);
                        print("\n");
                    } else {
                        print("Error reading via PIO.\n");
                    }
                    kfree(sector_buf);
                } else {
                    print("Memory allocation failed.\n");
                }
            } else {
                print("Count must be between 1 and 8.\n");
            }
        } else {
            print("Usage: ata-read [lba] [count]\n");
        }
    }
    else if (strncmp_local(input, "ata-write ", 10) == 0) {
        char* args = input + 10;
        int lba = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            lba = lba * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            char* text = args + i + 1;
            int text_len = strlen(text);
            unsigned short sector_buf[256];
            for (int j = 0; j < 256; j++) sector_buf[j] = 0;
            memory_copy(text, (char*)sector_buf, text_len > 512 ? 512 : text_len);
            
            int res = ata_write_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, 1, sector_buf);
            if (res == 0) {
                print("Write successful (PIO) to sector ");
                char lba_str[16];
                itoa(lba, lba_str);
                print(lba_str);
                print("\n");
            } else {
                print("Error writing via PIO.\n");
            }
        } else {
            print("Usage: ata-write [lba] [text]\n");
        }
    }
    else if (strncmp_local(input, "ata-dma-read ", 13) == 0) {
        char* args = input + 13;
        int lba = 0, count = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            lba = lba * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            i++;
            while (args[i] >= '0' && args[i] <= '9') {
                count = count * 10 + (args[i] - '0');
                i++;
            }
            if (count > 0 && count <= 8) {
                unsigned short* sector_buf = (unsigned short*)kmalloc(count * 512);
                if (sector_buf) {
                    int res = ata_read_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, sector_buf);
                    if (res == 0) {
                        print("Read successful (DMA):\n");
                        char* char_buf = (char*)sector_buf;
                        char_buf[count * 512 - 1] = '\0';
                        print(char_buf);
                        print("\n");
                    } else if (res == -2) {
                        print("DMA not supported by controller.\n");
                    } else {
                        print("Error reading via DMA.\n");
                    }
                    kfree(sector_buf);
                } else {
                    print("Memory allocation failed.\n");
                }
            } else {
                print("Count must be between 1 and 8.\n");
            }
        } else {
            print("Usage: ata-dma-read [lba] [count]\n");
        }
    }
    else if (strncmp_local(input, "ata-dma-write ", 14) == 0) {
        char* args = input + 14;
        int lba = 0, i = 0;
        while (args[i] >= '0' && args[i] <= '9') {
            lba = lba * 10 + (args[i] - '0');
            i++;
        }
        if (args[i] == ' ') {
            char* text = args + i + 1;
            int text_len = strlen(text);
            unsigned short sector_buf[256];
            for (int j = 0; j < 256; j++) sector_buf[j] = 0;
            memory_copy(text, (char*)sector_buf, text_len > 512 ? 512 : text_len);
            
            int res = ata_write_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, 1, sector_buf);
            if (res == 0) {
                print("Write successful (DMA) to sector ");
                char lba_str[16];
                itoa(lba, lba_str);
                print(lba_str);
                print("\n");
            } else if (res == -2) {
                print("DMA not supported by controller.\n");
            } else {
                print("Error writing via DMA.\n");
            }
        } else {
            print("Usage: ata-dma-write [lba] [text]\n");
        }
    }
    else if (strcmp(input, "date") == 0) {
        rtc_time_t t;
        rtc_get_time(&t);
        
        char buf[16];
        itoa(t.year, buf); print(buf); print("-");
        if (t.month < 10) print("0");
        itoa(t.month, buf); print(buf); print("-");
        if (t.day < 10) print("0");
        itoa(t.day, buf); print(buf); print(" ");
        if (t.hour < 10) print("0");
        itoa(t.hour, buf); print(buf); print(":");
        if (t.minute < 10) print("0");
        itoa(t.minute, buf); print(buf); print(":");
        if (t.second < 10) print("0");
        itoa(t.second, buf); print(buf); print("\n");
    }
    else if (strcmp(input, "sync") == 0) {
        print("Flushing caches...\n");
        page_cache_flush_all();
        buffer_cache_flush();
        print("Caches successfully synced to disk.\n");
    }
    else if (strcmp(input, "cache-stats") == 0) {
        cache_stats_t stats = cache_get_stats();
        print("Unified Page & Buffer Cache Statistics:\n");
        print("---------------------------------------\n");
        
        char buf[32];
        print("Block Cache Reads        : "); itoa(stats.block_reads, buf); print(buf); print("\n");
        print("Block Cache Read Hits    : "); itoa(stats.block_read_hits, buf); print(buf); print("\n");
        print("Block Cache Writes       : "); itoa(stats.block_writes, buf); print(buf); print("\n");
        print("Block Cache Write Hits   : "); itoa(stats.block_write_hits, buf); print(buf); print("\n");
        print("\n");
        print("Page Cache Reads         : "); itoa(stats.page_reads, buf); print(buf); print("\n");
        print("Page Cache Read Hits     : "); itoa(stats.page_read_hits, buf); print(buf); print("\n");
        print("Page Cache Writes        : "); itoa(stats.page_writes, buf); print(buf); print("\n");
        print("Page Cache Write Hits    : "); itoa(stats.page_write_hits, buf); print(buf); print("\n");
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