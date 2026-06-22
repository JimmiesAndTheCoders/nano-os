#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[32];
    unsigned int offset;
    unsigned int length;
} initrd_file_t;

typedef struct {
    unsigned int nfiles;
    initrd_file_t files[16];
} initrd_header_t;

// Helper to extract just the file name, ignoring the folder path
const char* get_filename(const char* path) {
    const char* p = path;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\') p = &path[i + 1];
    }
    return p;
}

int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Usage: %s <output.img> <file1> [file2...]\n", argv[0]);
        return 1;
    }

    int num_files = argc - 2;
    if (num_files > 16) num_files = 16;

    initrd_header_t header;
    memset(&header, 0, sizeof(initrd_header_t));
    header.nfiles = num_files;

    unsigned int current_offset = sizeof(initrd_header_t);

    // Calculate file sizes and offsets
    for(int i = 0; i < num_files; i++) {
        strncpy(header.files[i].name, get_filename(argv[i+2]), 31);
        FILE *in = fopen(argv[i+2], "rb");
        if(!in) { printf("Error opening %s\n", argv[i+2]); return 1; }
        
        fseek(in, 0, SEEK_END);
        long file_len = ftell(in);
        fclose(in);
        
        header.files[i].length = file_len + 1; // +1 to inject a null terminator!
        header.files[i].offset = current_offset;
        current_offset += header.files[i].length;
    }

    FILE *out = fopen(argv[1], "wb");
    fwrite(&header, sizeof(initrd_header_t), 1, out);

    // Read files, append \0, and write to image
    for(int i = 0; i < num_files; i++) {
        FILE *in = fopen(argv[i+2], "rb");
        long file_len = header.files[i].length - 1;
        char *buf = malloc(header.files[i].length);
        
        if (file_len > 0) fread(buf, 1, file_len, in);
        buf[file_len] = '\0'; // Guarantee safe reading in the OS shell
        
        fwrite(buf, 1, header.files[i].length, out);
        fclose(in);
        free(buf);
    }

    fclose(out);
    printf("[Initrd] Packed %d files into %s\n", num_files, argv[1]);
    return 0;
}