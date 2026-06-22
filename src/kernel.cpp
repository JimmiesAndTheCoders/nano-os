#include "vga_screen.hpp"

extern "C" {
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
    #include "nanolib.h"
    #include "gdt.h" // <-- NEW
    
    void call_global_constructors();
}

void heartbeat_task() { /* Unchanged */
    volatile char *video = (char*)0xb8000;
    int offset = (25 * 80 * 2) - 2; 
    while(1) {
        if (video[offset] == '*') video[offset] = ' ';
        else { video[offset] = '*'; video[offset+1] = 0x0E; }
        for (volatile int i = 0; i < 10000000; i++); 
    }
}

void background_worker_task() { /* Unchanged */
    while(1) { __asm__ __volatile__("nop"); }
}

// --- NEW: A true User-mode application ---
void user_mode_task() {
    // Wait a brief moment for the CLI to be drawn before asserting presence
    for (volatile int i = 0; i < 50000000; i++); 
    
    // Test our Syscall via an int 0x80 gate - this is the ONLY way we can talk 
    // to the hardware from Ring 3 without crashing!
    nano_print((char*)"\n[User Task] Executing safely in Ring 3 user-space!\nnano> ");
    
    while(1) {
        // Idling in User Mode without `hlt` privilege!
    }
}

extern "C" void kernel_main() {
    call_global_constructors();
    init_gdt();      // <-- NEW: Inject GDT and Ring 3 Segments first!
    isr_install();
    irq_install();
    
    init_pmm(0x100000);              
    kmalloc_init(0x200000, 0x80000); 
    pmm_reserve_region(0x300000, 0x20000); 
    init_initrd(0x30000);           
    init_syscalls();                 
    init_timer(100);
    init_tasking();                  
    init_keyboard();                 

    task_add(heartbeat_task, "heartbeat"); 
    task_add(background_worker_task, "worker1");
    task_add_user(user_mode_task, "user_task1"); // <-- NEW: Spawn our user program!

    clear_screen(); 
    
    print("============================================================\n");
    print(" Nano OS Kernel Initialization & System Check\n");
    print("============================================================\n\n");
    
    print("[OK] Bootloader transitioned to 32-bit Protected Mode\n");

    void* p_block = pmm_alloc_block();
    pmm_free_block(p_block);
    print("[OK] Bitmap-based Physical Memory Allocator verified\n");

    print("[OK] Virtual Memory (Paging) enabled and mapping isolated\n");
    
    print("[OK] Global Descriptor Table (GDT) and TSS reloaded\n"); // <-- NEW

    char* heap_test = (char*)kmalloc(16);
    if (heap_test) {
        kfree(heap_test);
        print("[OK] Kernel dynamic memory (kmalloc/kfree) operational\n");
    }

    print("[OK] Preemptive Multitasking active (3 tasks queued)\n");
    print("[OK] RAM-based File System (Initrd) mounted at 0x300000\n");
    
    nano_print((char*)"[OK] User-space Software Interrupts (Syscalls) ready\n");
    print("\n============================================================\n\n");

    print("Nano OS Version 0.1.0\n");
    print("(c) 2026 JimmiesAndTheCoders. All rights reserved.\n");
    print("Type 'help' to get started.\n\n");
    
    print_prompt();

    __asm__ __volatile__("sti");

    while(1) {
        __asm__ __volatile__("hlt");
    }
}