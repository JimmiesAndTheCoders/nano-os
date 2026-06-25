#include "sys/syscall.h"
#include "string.h"

void _start(int argc, char** argv, char** envp) {
    const char* msg = "\n[USER PROCESS] Compiled with nanolibc and running in Ring 3!\n";
    
    // Invoke SYS_PRINT (0) system call via our clean __syscall1 wrapper
    __syscall1(SYS_PRINT, (int)msg);

    if (argc > 0) {
        __syscall1(SYS_PRINT, (int)"\nCommand-line arguments:\n");
        for (int i = 0; i < argc; i++) {
            __syscall1(SYS_PRINT, (int)argv[i]);
            __syscall1(SYS_PRINT, (int)"\n");
        }
    }

    if (envp && envp[0]) {
        __syscall1(SYS_PRINT, (int)"\nEnvironment variables:\n");
        for (int i = 0; envp[i]; i++) {
            __syscall1(SYS_PRINT, (int)envp[i]);
            __syscall1(SYS_PRINT, (int)"\n");
        }
    }

    while (1) {
        __asm__ __volatile__ ("nop");
    }
}