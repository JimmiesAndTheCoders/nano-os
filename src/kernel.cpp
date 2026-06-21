#include "vga_screen.hpp"

extern "C" {
    #include "cpu.h"
    #include "keyboard.h"
    #include "shell.h"
    #include "pmm.h"
    #include "kmalloc.h"
    #include "timer.h"
    #include "task.h"
    #include "initrd.h"
    #include "util.h"
    #include "syscall.h"

    void call_global_constructors();
}

// A simple C++ task to show preemptive multitasking is working
void heartbeat_task() {
    while(1) {
        // We use direct memory access but we protect it with CLI/STI 
        // to avoid "glitching" the VGA registers used by the main screen
        __asm__ __volatile__("cli");
        
        volatile char *video = (char*)0xb8000;
        // Move to the very last character of the screen (bottom right)
        // to stay out of the way of the shell.
        int offset = (25 * 80 * 2) - 2; 
        
        if (video[offset] == '*') {
            video[offset] = ' ';
        } else {
            video[offset] = '*';
            video[offset+1] = 0x0E; // Yellow
        }
        
        __asm__ __volatile__("sti");

        for (volatile int i = 0; i < 5000000; i++); 
    }
}

extern "C" void kernel_main() {
    // 1. Critical Low-level Setup
    isr_install();
    irq_install();
    init_pmm(0x100000);
    pmm_reserve_region(0x300000, 0x10000); // Reserve Initrd
    kmalloc_init(0x200000, 0x10000);

    // 2. Start C++ (Initializes the screen object)
    call_global_constructors();

    // 3. Clear screen and start output
    screen.clear();
    screen.print("Nano OS [C++ Edition] Online\n");

    init_initrd(0x300000);
    init_syscalls();
    init_timer(100);
    init_tasking();
    init_keyboard();

    // 4. Add the blinking background task
    task_add(heartbeat_task, "heartbeat"); 

    screen.print("Welcome to Nano Shell. Type 'help' for commands.\n\n> ");

    // 5. Start the engine!
    __asm__ __volatile__("sti");

    while(1) {
        // Task 0 stays alive here to process keyboard interrupts
        __asm__ __volatile__("hlt");
    }
}