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
#include "initrd.h"
#include "util.h"

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
    
    isr_install();
    irq_install();

    init_pmm(0x100000); 
    kmalloc_init(0x200000, 0x10000);
    
    /* Initrd Setup */
    init_initrd(0x9000);
    initrd_header_t* h = (initrd_header_t*)0x9000;
    h->nfiles = 2;
    
    memory_copy("hello.txt", h->files[0].name, 10);
    h->files[0].offset = 1000;
    h->files[0].length = 13;
    memory_copy("Hello World!", (char*)(0x9000 + 1000), 13);
    
    memory_copy("note.txt", h->files[1].name, 9);
    h->files[1].offset = 1100;
    h->files[1].length = 25;
    memory_copy("Nano OS RAM FS is active.", (char*)(0x9000 + 1100), 25);

    print("Initrd System Loaded......... [OK]\n");

    init_keyboard();
    init_timer(100);

    init_tasking();
    task_add(dummy_task, "worker");

    /* Start the System */
    print("Welcome to Nano OS!\n");
    print("Type help for more information \n");
    
    __asm__ __volatile__("sti");
    
    print("nano> ");

    while(1) {
        // Task 0: Handle shell
    }
}