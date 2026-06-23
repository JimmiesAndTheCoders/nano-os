#ifndef INITRD_H
#define INITRD_H

#include "vfs.h"

typedef struct {
    char name[32];
    unsigned int offset; // Offset from the start of the RAM disk
    unsigned int length;
    unsigned int is_dir; // 0 for file, 1 for directory
} initrd_file_t;

typedef struct {
    unsigned int nfiles;
    initrd_file_t files[32]; // Supporting up to 32 files
} initrd_header_t;

void init_initrd(unsigned int location);
void list_files(const char* current_dir);
char* read_file(const char* name);
int create_file(const char* name, unsigned int is_dir);
int write_file_content(const char* name, const char* content, unsigned int length);
int directory_exists(const char* path);

// VFS Adapter Generator
vfs_node_t* init_initrd_vfs();

#endif