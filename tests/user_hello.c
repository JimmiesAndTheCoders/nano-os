// We align to standard C ABI parameters layout initialized by our custom loader!
void _start(int argc, char** argv, char** envp) {
    const char* msg = "\n[USER PROCESS] Hello from a dynamically loaded user-space ELF executable running in Ring 3!\n";
    
    // Invoke SYS_PRINT (0) system call: EAX=0, EBX=pointer to string
    __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"(msg));

    // Print command line arguments loop
    if (argc > 0) {
        __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"("\nArguments provided:\n"));
        for (int i = 0; i < argc; i++) {
            __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"(argv[i]));
            __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"("\n"));
        }
    }

    // Print environmental variables loop
    if (envp && envp[0]) {
        __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"("\nEnvironment variables:\n"));
        for (int i = 0; envp[i]; i++) {
            __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"(envp[i]));
            __asm__ __volatile__ ("int $0x80" : : "a"(0), "b"("\n"));
        }
    }

    // Spin processor lock natively
    while (1) {
        __asm__ __volatile__ ("nop");
    }
}