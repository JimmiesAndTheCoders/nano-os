#include "vga_screen.hpp"

extern "C" {
    #include "screen.h"  // Use the C interface for guaranteed stability
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

void heartbeat_task() {
    volatile char *video = (char*)0xb8000;
    int offset = (25 * 80 * 2) - 2; 
    while(1) {
        if (video[offset] == '*') {
            video[offset] = ' ';
        } else {
            video[offset] = '*';
            video[offset+1] = 0x0E; 
        }
        for (volatile int i = 0; i < 10000000; i++); 
    }
}

extern "C" void kernel_main() {
    // 1. Silent Initialization
    call_global_constructors();
    isr_install();
    irq_install();
    init_pmm(0x100000);
    kmalloc_init(0x200000, 0x80000); 
    pmm_reserve_region(0x300000, 0x20000); 
    init_initrd(0x300000);
    init_syscalls();
    init_timer(100);
    init_tasking();
    init_keyboard();
    task_add(heartbeat_task, "heartbeat"); 

    // 2. The Visual Reveal (Windows CMD / MS-DOS Style)
    clear_screen(); 
    
    print("Nano OS Version 0.1.0\n");
    print("(c) 2026 JimmiesAndTheCoders. All rights reserved.\n");
    print("Type 'help' to get started.\n\n");
    
    print_prompt(); // Start the shell prompt

    // 3. Final Step: Enable interrupts so the keyboard works!
    __asm__ __volatile__("sti");

    while(1) {
        __asm__ __volatile__("hlt");
    }
}