void _start() {
    const char* msg = "\n[USER PROCESS] Hello from a dynamically loaded user-space ELF executable running in Ring 3!\n";
    
    // Invoke SYS_PRINT (0) system call: EAX=0, EBX=pointer to string
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(0), "b"(msg)
    );

    // Spin in a loop
    while (1) {
        __asm__ __volatile__ ("nop");
    }
}