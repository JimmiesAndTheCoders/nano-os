#include "initrd.h"
#include "screen.h"
#include "util.h"

static initrd_header_t *header;
static unsigned int root_address;

void init_initrd(unsigned int location) {
    root_address = location;
    header = (initrd_header_t *)location;
}

void list_files() {
    print("Listing files in Initrd:\n");
    for (unsigned int i = 0; i < header->nfiles; i++) {
        print("- ");
        print(header->files[i].name);
        print("\n");
    }
}

char* read_file(char* name) {
    for (unsigned int i = 0; i < header->nfiles; i++) {
        if (strcmp(header->files[i].name, name) == 0) {
            // Return pointer to the actual data in memory
            return (char *)(root_address + header->files[i].offset);
        }
    }
    return 0;
}