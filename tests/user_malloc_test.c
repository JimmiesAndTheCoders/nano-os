#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

void _start(int argc, char** argv, char** envp) {
    printf("\n--- Malloc Test Program Starting ---\n");
    printf("Current System Ticks: %d\n", get_ticks());

    char *buffer = (char *)malloc(128);
    if (buffer != NULL) {
        printf("Successfully allocated 128 bytes at address: %x\n", (unsigned int)buffer);
        
        // Populate and display content from user heap memory
        memcpy(buffer, "Dynamic Memory is fully functional!", 36);
        printf("Heap Buffer Content: %s\n", buffer);
        
        free(buffer);
        printf("Successfully deallocated heap memory.\n");
    } else {
        printf("Error: Malloc allocation failed!\n");
    }

    printf("--- Malloc Test Program Completed ---\n");
    exit(0);
}