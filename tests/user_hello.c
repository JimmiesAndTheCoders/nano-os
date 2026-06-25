#include "stdio.h"

int main(int argc, char** argv, char** envp) {
    printf("\n[USER PROCESS] Hello from a dynamically loaded user-space ELF executable running in Ring 3!\n");

    if (argc > 0) {
        printf("\nArguments provided:\n");
        for (int i = 0; i < argc; i++) {
            printf("  argv[%d]: %s\n", i, argv[i]);
        }
    }

    if (envp && envp[0]) {
        printf("\nEnvironment variables:\n");
        for (int i = 0; envp[i]; i++) {
            printf("  %s\n", envp[i]);
        }
    }

    return 0;
}