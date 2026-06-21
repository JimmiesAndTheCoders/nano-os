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
#include "syscall.h"

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

void user_print(char* message) {
    __asm__ __volatile__ (
        "int $0x80"
        : /* No output registers */
        : "a"(SYS_PRINT), "b"(message) /* 'a' forces EAX, 'b' forces EBX */
        : "memory"
    );
}

void test_user_task() {
    // Wait for 2 full seconds before starting
    for(volatile int i = 0; i < 200000000; i++);

    while(1) {
        // Use the syscall to print
        user_print("."); // Just print a small dot so it doesn't mess up the screen
        for(volatile int i = 0; i < 100000000; i++);
    }
}
void kernel_main() {
    clear_screen();
    
    // 1. Initializations...
    isr_install();
    irq_install();
    init_pmm(0x100000); 
    kmalloc_init(0x200000, 0x10000);
    init_keyboard();
    init_timer(100);

    // 2. Setup Initrd...
    unsigned int rd_location = 0x10000; // 64KB mark is safe
    init_initrd(rd_location);
    
    initrd_header_t* h = (initrd_header_t*)rd_location;
    h->nfiles = 2;
    
    memory_copy("hello.txt", h->files[0].name, 10);
    h->files[0].offset = 1000;
    h->files[0].length = 13;
    memory_copy("Hello World!", (char*)(rd_location + 1000), 13);
    
    memory_copy("note.txt", h->files[1].name, 9);
    h->files[1].offset = 1100;
    h->files[1].length = 25;
    memory_copy("Nano OS RAM FS is active.", (char*)(rd_location + 1100), 25);

    /* 2. Print success message BEFORE starting multitasking */
    print("Nano OS Kernel Booting...\n");
    print("Initrd System Loaded......... [OK]\n");
    print("Multitasking Contexts........ [OK]\n");
    print("Starting Shell...\n\n> ");

    // 4. Setup Tasking
    init_syscalls();
    init_tasking();
    task_add(test_user_task, "heartbeat"); 

    // 5. THE MOMENT OF TRUTH
    __asm__ __volatile__("sti");

    while(1) {
        // The main thread is Task 0. It just waits for keyboard interrupts.
        __asm__ __volatile__("hlt"); // Save power while waiting
    }
}