/* ==============================================================================
 * NANO OS - C Kernel Core (With Physical Memory Testing)
 * Description: The main entry point of our operating system.
 * ============================================================================== */

#include "screen.h"
#include "cpu.h"
#include "keyboard.h"
#include "shell.h"
#include "pmm.h"
#include "kmalloc.h"
#include "timer.h"
#include "task.h"

void dummy_task() {
    while(1) {
        volatile char *video = (char*)0xb8000; // 'volatile' prevents optimization
        
        // Offset 158 is the character, 159 is the color attribute
        if (video[158] == '*') {
            video[158] = ' ';
        } else {
            video[158] = '*';
            video[159] = 0x0E; // 0x0E is Bright Yellow on Black
        }

        // Use 'volatile' in the loop so the compiler doesn't delete the delay
        for (volatile int i = 0; i < 5000000; i++); 
    }
}
void kernel_main() {
    clear_screen();
    
    /* 1. Core CPU Setup */
    isr_install();
    irq_install();

    /* 2. Memory Setup (Crucial: Do this BEFORE multitasking) */
    init_pmm(0x100000); 
    kmalloc_init(0x200000, 0x10000);

    /* 3. Hardware Setup */
    init_keyboard();
    init_timer(100);

    /* 4. Tasking Setup */
    init_tasking();
    task_add(dummy_task, "worker");

    /* 5. Start the System */
    print("Welcome to Nano OS!\n");
    print("All systems initialized. Enabling Multitasking...\n");
    
    __asm__ __volatile__("sti"); // Start the PIT timer and scheduler

    print("nano> ");

    while(1) {
        // Task 0: Handle shell
    }
}