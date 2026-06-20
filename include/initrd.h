#ifndef INITRD_H
#define INITRD_H

typedef struct {
    char name[32];
    unsigned int offset; // Offset from the start of the RAM disk
    unsigned int length;
} initrd_file_t;

typedef struct {
    unsigned int nfiles;
    initrd_file_t files[16]; // Supporting up to 16 files for now
} initrd_header_t;

void init_initrd(unsigned int location);
void list_files();
char* read_file(char* name);

#endif