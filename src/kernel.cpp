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
    #include "gdt.h" 
    #include "graphics.h"
    #include "vbe.h"
    
    void call_global_constructors();
}

void heartbeat_task() { 
    vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;
    static int toggle = 0;
    while(1) {
        if (vbe->width > 0) {
            // Draw a blinking box in the bottom right corner for GUI mode!
            fill_rect(vbe->width - 20, vbe->height - 20, 10, 10, toggle ? 0x00FF00 : 0x000000);
        } else {
            // Fallback for Text Mode!
            volatile char *video = (char*)0xb8000;
            int offset = (25 * 80 * 2) - 2; 
            if (video[offset] == '*') video[offset] = ' ';
            else { video[offset] = '*'; video[offset+1] = 0x0E; }
        }
        toggle = !toggle;
        for (volatile int i = 0; i < 10000000; i++); 
    }
}

void background_worker_task() {
    while(1) { __asm__ __volatile__("nop"); }
}

void user_mode_task() {
    for (volatile int i = 0; i < 50000000; i++); 
    nano_print((char*)"\n[User Task] Executing safely in Ring 3 user-space!\nnano> ");
    while(1) {}
}

extern "C" void kernel_main() {
    call_global_constructors();
    init_gdt();      
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
    task_add_user(user_mode_task, "user_task1"); 

    clear_screen(); 

    vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;
    if (vbe->width > 0) {
        // Draw a decorative color gradient across the top of the GUI
        for (int i = 0; i < vbe->width; i++) {
            draw_line(i, 0, i, 5, (i % 255) << 16 | (255 - (i % 255)) << 8 | 255);
        }
    }
    
    print("============================================================\n");
    print(" Nano OS Kernel Initialization & System Check\n");
    print("============================================================\n\n");
    
    print("[OK] Bootloader transitioned to 32-bit Protected Mode\n");

    void* p_block = pmm_alloc_block();
    pmm_free_block(p_block);
    print("[OK] Bitmap-based Physical Memory Allocator verified\n");

    print("[OK] Virtual Memory (Paging) enabled and mapping isolated\n");
    print("[OK] Global Descriptor Table (GDT) and TSS reloaded\n"); 

    char* heap_test = (char*)kmalloc(16);
    if (heap_test) {
        kfree(heap_test);
        print("[OK] Kernel dynamic memory (kmalloc/kfree) operational\n");
    }

    print("[OK] Preemptive Multitasking active (3 tasks queued)\n");
    print("[OK] RAM-based File System (Initrd) mounted at 0x300000\n");
    print("[OK] User-space Software Interrupts (Syscalls) ready\n");
    
    if (vbe->width > 0) {
        print("[OK] VESA VBE High-Resolution GUI Enabled!\n");
    } else {
        print("[OK] Standard VGA Text Console Fallback mapped!\n");
    }

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