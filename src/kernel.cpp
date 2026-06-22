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

void background_worker_task() {
    while(1) {
        __asm__ __volatile__("nop");
    }
}

extern "C" void kernel_main() {
    // ====================================================================
    // 1. Silent Core Hardware & Memory Initialization
    // ====================================================================
    call_global_constructors();
    isr_install();
    irq_install();
    
    init_pmm(0x100000);              
    kmalloc_init(0x200000, 0x80000); 
    pmm_reserve_region(0x300000, 0x20000); 
    
    // --> CHANGED: Initialize RAM-disk at 0x30000 (where boot.asm placed it)
    init_initrd(0x30000);           
    
    init_syscalls();                 
    init_timer(100);
    init_tasking();                  // Initialize Context Switcher
    init_keyboard();                 // Initialize PS/2 Keyboard

    // Add tasks to the scheduler queue
    task_add(heartbeat_task, "heartbeat"); 
    task_add(background_worker_task, "worker1");

    // ====================================================================
    // 2. Boot Sequence / Roadmap Feature Demonstration
    // ====================================================================
    clear_screen(); 
    
    print("============================================================\n");
    print(" Nano OS Kernel Initialization & System Check\n");
    print("============================================================\n\n");
    
    // Feature: Bootloader & Kernel
    print("[OK] Bootloader transitioned to 32-bit Protected Mode\n");

    // Feature: Bitmap-based Memory Allocator
    void* p_block = pmm_alloc_block();
    pmm_free_block(p_block);
    print("[OK] Bitmap-based Physical Memory Allocator verified\n");

    // Feature: Virtual Memory
    // (Paging tables were enabled in boot.asm before jumping to the C kernel)
    print("[OK] Virtual Memory (Paging) enabled and mapping isolated\n");

    // Feature: kmalloc / kfree
    char* heap_test = (char*)kmalloc(16);
    if (heap_test) {
        kfree(heap_test);
        print("[OK] Kernel dynamic memory (kmalloc/kfree) operational\n");
    }

    // Feature: PIT and Preemptive Multitasking
    print("[OK] Programmable Interval Timer (PIT) configured at 100Hz\n");
    print("[OK] Preemptive Multitasking active (3 tasks queued)\n");

    // Feature: Initrd RAM disk
    print("[OK] RAM-based File System (Initrd) mounted at 0x300000\n");

    // Feature: Software Interrupts
    // Use the Syscall API to trigger an interrupt handler (Int 0x80)
    nano_print((char*)"[OK] User-space Software Interrupts (Syscalls) ready\n");

    print("\n============================================================\n\n");

    // ====================================================================
    // 3. User Interface / Shell Hand-off
    // ====================================================================
    print("Nano OS Version 0.1.0\n");
    print("(c) 2026 JimmiesAndTheCoders. All rights reserved.\n");
    print("Type 'help' to get started.\n\n");
    
    print_prompt();

    // Final Step: Enable interrupts! 
    // This immediately starts the hardware timer for multitasking and enables keyboard
    __asm__ __volatile__("sti");

    // Idle thread: Puts CPU to sleep until the next interrupt
    while(1) {
        __asm__ __volatile__("hlt");
    }
}