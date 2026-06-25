#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

int main(int argc, char** argv, char** envp) {
    (void)argc; (void)argv; (void)envp;
    
    printf("\n--- Malloc Test Program Starting ---\n");
    printf("Current System Ticks: %d\n", get_ticks());

    char *buffer = (char *)malloc(128);
    if (buffer != NULL) {
        printf("Successfully allocated 128 bytes at address: %x\n", (unsigned int)buffer);
        
        memcpy(buffer, "Dynamic Memory is fully functional!", 36);
        printf("Heap Buffer Content: %s\n", buffer);
        
        free(buffer);
        printf("Successfully deallocated heap memory.\n");
    } else {
        printf("Error: Malloc allocation failed!\n");
    }

    printf("--- Malloc Test Program Completed ---\n");
    return 0; // Handled by CRT0 exit sequence
}