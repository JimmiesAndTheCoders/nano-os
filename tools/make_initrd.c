#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Helper to extract just the file name, ignoring the folder path
const char* get_filename(const char* path) {
    const char* p = path;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\') p = &path[i + 1];
    }
    return p;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_initrd> [input_files...]\n", argv[0]);
        return 1;
    }

    const char *out_name = argv[1];
    initrd_header_t header;
    memset(&header, 0, sizeof(initrd_header_t));

    // Open output file
    FILE *out = fopen(out_name, "wb");
    if (!out) {
        fprintf(stderr, "Error: Could not open output file %s\n", out_name);
        return 1;
    }

    // Prepare workspace offset directly after the metadata header
    unsigned int current_offset = sizeof(initrd_header_t);
    
    // Write a blank header placeholder first
    if (fwrite(&header, sizeof(initrd_header_t), 1, out) != 1) {
        fprintf(stderr, "Error: Failed to write initial header\n");
        fclose(out);
        return 1;
    }

    int file_idx = 0;
    for (int i = 2; i < argc; i++) {
        if (file_idx >= 32) {
            fprintf(stderr, "Warning: Maximum of 32 files reached. Skipping %s\n", argv[i]);
            break;
        }

        const char *filepath = argv[i];
        FILE *in = fopen(filepath, "rb");
        if (!in) {
            fprintf(stderr, "Error: Could not open input file %s\n", filepath);
            fclose(out);
            return 1;
        }

        // Determine file size
        fseek(in, 0, SEEK_END);
        unsigned int size = ftell(in);
        fseek(in, 0, SEEK_SET);

        char *buffer = malloc(size);
        if (!buffer && size > 0) {
            fprintf(stderr, "Error: Out of memory reading %s\n", filepath);
            fclose(in);
            fclose(out);
            return 1;
        }

        if (size > 0 && fread(buffer, size, 1, in) != 1) {
            fprintf(stderr, "Error: Failed to read %s\n", filepath);
            free(buffer);
            fclose(in);
            fclose(out);
            return 1;
        }
        fclose(in);

        // Populate header entry metadata
        const char *filename = get_filename(filepath);
        strncpy(header.files[file_idx].name, filename, 31);
        header.files[file_idx].name[31] = '\0';
        header.files[file_idx].offset = current_offset;
        header.files[file_idx].length = size;
        header.files[file_idx].is_dir = 0;

        // Append file data directly to output stream
        if (size > 0) {
            if (fwrite(buffer, size, 1, out) != 1) {
                fprintf(stderr, "Error: Failed to write data of %s to initrd\n", filepath);
                free(buffer);
                fclose(out);
                return 1;
            }
        }
        free(buffer);

        current_offset += size;
        file_idx++;
    }

    header.nfiles = file_idx;

    // Seek back to write the completed metadata header
    fseek(out, 0, SEEK_SET);
    if (fwrite(&header, sizeof(initrd_header_t), 1, out) != 1) {
        fprintf(stderr, "Error: Failed to write final header\n");
        fclose(out);
        return 1;
    }

    fclose(out);
    return 0;
}