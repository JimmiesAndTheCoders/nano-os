#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define PATH_SEP "\\"
#else
    #define PATH_SEP "/"
#endif

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Nano OS C-to-ELF Compiler Utility (ncc)\n");
        printf("Usage: %s <input.c> <output.elf>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    // Determine compilers and linkers
    const char *compiler = "i686-elf-gcc";
    const char *linker = "i686-elf-ld";

    // Basic check to see if i686-elf-gcc is present, otherwise fallback to local gcc with -m32
    #ifndef _WIN32
    if (system("which i686-elf-gcc > /dev/null 2>&1") != 0) {
        compiler = "gcc -m32";
        linker = "ld -melf_i386";
    }
    #endif

    // Temporary object file name
    char obj_file[256];
    snprintf(obj_file, sizeof(obj_file), "%s.o", output_file);

    // Compilation Command
    // Sets up freestanding mode, disables stack-protector/PIC, and includes the libc folder
    char compile_cmd[1024];
    snprintf(compile_cmd, sizeof(compile_cmd),
             "%s -ffreestanding -fno-pic -fno-pie -fno-stack-protector -nostdlib "
             "-Ilibc" PATH_SEP "include -c %s -o %s",
             compiler, input_file, obj_file);

    printf("[ncc] Compiling %s...\n", input_file);
    if (system(compile_cmd) != 0) {
        fprintf(stderr, "[ncc] Error: Compilation stage failed.\n");
        return 1;
    }

    // Linking Command
    // Links with crt0.o and static library libnano.a at virtual address 0x08048000
    char link_cmd[1024];
    snprintf(link_cmd, sizeof(link_cmd),
             "%s -melf_i386 -Ttext 0x08048000 -e _start build" PATH_SEP "crt0.o %s "
             "-Lbuild -lnano -o %s",
             linker, obj_file, output_file);

    printf("[ncc] Linking %s...\n", output_file);
    if (system(link_cmd) != 0) {
        fprintf(stderr, "[ncc] Error: Linking stage failed.\n");
        remove(obj_file);
        return 1;
    }

    // Clean up temporary object file
    remove(obj_file);
    printf("[ncc] Successfully compiled: %s -> %s\n", input_file, output_file);

    return 0;
}