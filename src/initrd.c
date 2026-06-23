#include "initrd.h"
#include "screen.h"
#include "util.h"
#include "kmalloc.h"

static initrd_header_t *header;
static unsigned int root_address;

void init_initrd(unsigned int location) {
    root_address = location;
    header = (initrd_header_t *)location;
}

static int is_direct_child(const char* parent, const char* child) {
    int parent_len = strlen(parent);
    
    for (int i = 0; i < parent_len; i++) {
        if (parent[i] != child[i]) return 0;
    }
    
    if (strlen(child) == parent_len) return 0;
    
    const char* remaining = child + parent_len;
    if (parent_len > 1 && parent[parent_len - 1] != '/') {
        if (remaining[0] != '/') return 0;
        remaining++;
    }
    
    while (*remaining != '\0') {
        if (*remaining == '/') return 0;
        remaining++;
    }
    return 1;
}

void list_files(const char* current_dir) {
    print("Directory of ");
    print(current_dir);
    print(":\n");
    int count = 0;
    for (unsigned int i = 0; i < header->nfiles; i++) {
        const char* name = header->files[i].name;
        char std_name[64];
        
        if (name[0] != '/') {
            std_name[0] = '/';
            memory_copy(name, std_name + 1, strlen(name) + 1);
        } else {
            memory_copy(name, std_name, strlen(name) + 1);
        }

        if (is_direct_child(current_dir, std_name)) {
            count++;
            if (header->files[i].is_dir) {
                print("  <DIR>  ");
            } else {
                print("  <FILE> ");
            }
            int parent_len = strlen(current_dir);
            const char* display_name = std_name + parent_len;
            if (display_name[0] == '/') display_name++;
            print(display_name);
            print("\n");
        }
    }
    if (count == 0) {
        print("  (Empty)\n");
    }
}

char* read_file(const char* name) {
    char std_name[64];
    if (name[0] != '/') {
        std_name[0] = '/';
        memory_copy(name, std_name + 1, strlen(name) + 1);
    } else {
        memory_copy(name, std_name, strlen(name) + 1);
    }

    for (unsigned int i = 0; i < header->nfiles; i++) {
        char f_name[64];
        if (header->files[i].name[0] != '/') {
            f_name[0] = '/';
            memory_copy(header->files[i].name, f_name + 1, strlen(header->files[i].name) + 1);
        } else {
            memory_copy(header->files[i].name, f_name, strlen(header->files[i].name) + 1);
        }

        if (strcmp(f_name, std_name) == 0 && !header->files[i].is_dir) {
            return (char *)(root_address + header->files[i].offset);
        }
    }
    return 0;
}

int directory_exists(const char* path) {
    if (strcmp(path, "/") == 0) return 1;

    char std_path[64];
    if (path[0] != '/') {
        std_path[0] = '/';
        memory_copy(path, std_path + 1, strlen(path) + 1);
    } else {
        memory_copy(path, std_path, strlen(path) + 1);
    }

    int len = strlen(std_path);
    if (len > 1 && std_path[len - 1] == '/') {
        std_path[len - 1] = '\0';
    }

    for (unsigned int i = 0; i < header->nfiles; i++) {
        char f_name[64];
        if (header->files[i].name[0] != '/') {
            f_name[0] = '/';
            memory_copy(header->files[i].name, f_name + 1, strlen(header->files[i].name) + 1);
        } else {
            memory_copy(header->files[i].name, f_name, strlen(header->files[i].name) + 1);
        }

        if (strcmp(f_name, std_path) == 0 && header->files[i].is_dir) {
            return 1;
        }
    }
    return 0;
}

int create_file(const char* name, unsigned int is_dir) {
    if (header->nfiles >= 32) {
        return 0; 
    }

    int name_len = strlen(name);
    if (name_len >= 32) return 0;

    char std_name[64];
    if (name[0] != '/') {
        std_name[0] = '/';
        memory_copy(name, std_name + 1, name_len + 1);
    } else {
        memory_copy(name, std_name, name_len + 1);
    }

    for (unsigned int i = 0; i < header->nfiles; i++) {
        char f_name[64];
        if (header->files[i].name[0] != '/') {
            f_name[0] = '/';
            memory_copy(header->files[i].name, f_name + 1, strlen(header->files[i].name) + 1);
        } else {
            memory_copy(header->files[i].name, f_name, strlen(header->files[i].name) + 1);
        }
        if (strcmp(f_name, std_name) == 0) {
            return 0; 
        }
    }

    unsigned int idx = header->nfiles;
    memory_copy(name, header->files[idx].name, name_len + 1);
    header->files[idx].is_dir = is_dir;

    if (is_dir) {
        header->files[idx].length = 0;
        header->files[idx].offset = 0;
    } else {
        char* file_payload = (char*)kmalloc(1);
        if (!file_payload) return 0;
        file_payload[0] = '\0';
        
        header->files[idx].length = 1;
        header->files[idx].offset = (unsigned int)file_payload - root_address;
    }

    header->nfiles++;
    return 1;
}

int write_file_content(const char* name, const char* content, unsigned int length) {
    char std_name[64];
    if (name[0] != '/') {
        std_name[0] = '/';
        memory_copy(name, std_name + 1, strlen(name) + 1);
    } else {
        memory_copy(name, std_name, strlen(name) + 1);
    }

    for (unsigned int i = 0; i < header->nfiles; i++) {
        char f_name[64];
        if (header->files[i].name[0] != '/') {
            f_name[0] = '/';
            memory_copy(header->files[i].name, f_name + 1, strlen(header->files[i].name) + 1);
        } else {
            memory_copy(header->files[i].name, f_name, strlen(header->files[i].name) + 1);
        }

        if (strcmp(f_name, std_name) == 0 && !header->files[i].is_dir) {
            char* new_space = (char*)kmalloc(length + 1);
            if (!new_space) return 0;
            
            memory_copy(content, new_space, length);
            new_space[length] = '\0';
            
            header->files[i].length = length + 1;
            header->files[i].offset = (unsigned int)new_space - root_address;
            return 1;
        }
    }
    return 0;
}