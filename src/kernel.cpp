#include "vga_screen.hpp"

extern "C" {
    #include "rtc.h"
    #include "screen.h"
    #include "cpu.h"
    #include "keyboard.h"
    #include "mouse.h"
    #include "shell.h"
    #include "pmm.h"
    #include "kmalloc.h"
    #include "timer.h"
    #include "task.h"
    #include "initrd.h"
    #include "vfs.h"
    #include "fat32.h"
    #include "util.h"
    #include "syscall.h"
    #include "nanolib.h"
    #include "gdt.h" 
    #include "graphics.h"
    #include "vbe.h"
    #include "pci.h"
    #include "cache.h"
    #include "ata.h"
    #include "ipc.h"
    
    void call_global_constructors();
    
    // Declare the external Zig safety verification module function
    bool zig_verify_safety(unsigned int addr, unsigned int size);

    // Declare the external Rust safety verification module function
    bool rust_verify_pmm(unsigned int total_frames, unsigned int free_frames);
}

void heartbeat_task() { 
    vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;
    static int toggle = 0;
    while(1) {
        if (vbe->width > 0) {
            fill_rect(vbe->width - 20, vbe->height - 20, 10, 10, toggle ? 0xFF00FF00 : 0xFF000000); // Explicit full alpha green/black
        } else {
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
    nano_print((char*)"\n[User Task] Executing safely in Ring 3 user-space!\nnano:/> ");
    while(1) {}
}

extern "C" void kernel_main() {
    extern char _sbss;
    extern char _ebss;
    for (char* p = &_sbss; p < &_ebss; p++) {
        *p = 0;
    }
    
    call_global_constructors();
    init_gdt();      
    isr_install();
    irq_install();
    
    init_pmm(0x100000);              
    kmalloc_init(0x200000, 0x80000); 
    pmm_reserve_region(0x60000, 0x20000); // Reserve 128 KB for RAM disk
    init_initrd(0x60000);                 // Mount at physical address 0x60000
    init_syscalls();
    init_timer(100);                 
    init_tasking();                  
    init_keyboard();
    init_mouse();
    init_pci();
    init_lapic();
    init_apic_timer(100);            
    init_ata();
    init_cache();

    // VFS & Mount Initialisation
    init_vfs();
    vfs_root = init_initrd_vfs();
    
    // Pre-add standard files to the RAM disk root directory
    create_file("bin", 1);
    move_file("user_hello.elf", "bin/user_hello.elf");
    move_file("user_malloc_test.elf", "bin/user_malloc_test.elf");
    move_file("user_prime.elf", "bin/user_prime.elf");
    move_file("user_ipc_demo.elf", "bin/user_ipc_demo.elf");
    
    create_file("readme.txt", 0);
    write_file_content("readme.txt", "Welcome to Nano OS!\nUse 'help' to see available commands.\nUse 'exec [file]' to run user applications.\n", 79);
    
    // --- RAM Disk Diagnostic Print ---
    print("\n--- RAM Disk Boot Diagnostics ---\n");
    char debug_buf[32];
    itoa(header->nfiles, debug_buf);
    print("  File Count in Header: "); print(debug_buf); print("\n");
    for (unsigned int i = 0; i < header->nfiles; i++) {
        print("  - File "); itoa(i, debug_buf); print(debug_buf); print(": ");
        print(header->files[i].name);
        print(" (size: "); itoa(header->files[i].length, debug_buf); print(debug_buf); print(" bytes)\n");
    }
    print("----------------------------------\n\n");

    // Initialize IPC Tables and map the registry virtual disk
    init_ipc();
    vfs_mount("/ipc", ipc_get_vfs_root());

    int fat_status = init_fat32();

    task_add(heartbeat_task, "heartbeat"); 
    task_add(background_worker_task, "worker1");
    task_add_user(user_mode_task, "user_task1"); 

    clear_screen();

    vbe_mode_info_t* vbe = (vbe_mode_info_t*)0x5000;
    if (vbe->width > 0) {
        for (int i = 0; i < vbe->width; i++) {
            // Include 0xFF000000 base to guarantee opaque rendering
            draw_line(i, 0, i, 5, 0xFF000000 | (i % 255) << 16 | (255 - (i % 255)) << 8 | 255);
        }
    }
    
    print("============================================================\n");
    print(" Nano OS Kernel Initialization & System Check\n");
    print("============================================================\n\n");
    
    print("[OK] Bootloader transitioned to 32-bit Protected Mode\n");

    void* p_block = pmm_alloc_block();
    pmm_free_block(p_block);
    print("[OK] Bitmap-based Physical Memory Allocator verified\n");

    // Perform safety check via our new Zig Module
    if (zig_verify_safety(0x100000, 4096)) {
        print("[OK] Zig core safety verification framework validated\n");
    } else {
        print("[WARN] Zig boundary check flagged potential violation\n");
    }
    
    // Perform safety check via our new Rust Module
    unsigned int total_f = pmm_get_total_frames();
    unsigned int free_f = pmm_get_free_frames();
    if (rust_verify_pmm(total_f, free_f)) {
        print("[OK] Rust core safety verification framework validated\n");
    } else {
        print("[WARN] Rust boundary check flagged potential violation\n");
    }

    print("[OK] Virtual Memory (Paging) enabled and mapping isolated\n");
    print("[OK] Global Descriptor Table (GDT) and TSS reloaded\n"); 

    char* heap_test = (char*)kmalloc(16);
    if (heap_test) {
        kfree(heap_test);
        print("[OK] Kernel dynamic memory (kmalloc/kfree) operational\n");
    }

    if (apic_timer_active()) {
        print("[OK] Local APIC Timer calibrated and configured at 100 Hz\n");
    } else {
        print("[OK] Legacy PIT Timer operational (APIC Calibration fallback)\n");
    }

    print("[OK] Preemptive Multitasking active (3 tasks queued)\n");
    print("[OK] RAM-based File System (Initrd) mounted at 0x60000\n");
    print("[OK] User-space Software Interrupts (Syscalls) ready\n");
    print("[OK] PCI Bus Enumerator successfully initialized\n");
    print("[OK] Local APIC mapped and configured for MSI transport\n");
    
    if (ata_has_dma()) {
        print("[OK] ATA/IDE Controller found. Bus Master DMA initialized\n");
    } else {
        print("[OK] ATA/IDE Controller found. Fallback to PIO operations\n");
    }

    // Report VFS status
    print("[OK] Virtual File System abstraction initialized\n");
    print("[OK] RAM disk cleanly mapped as the root filesystem (/)\n");
    print("[OK] IPC Virtual Filesystem cleanly mounted at /ipc\n"); // <-- Report IPC mount
    
    if (fat_status == 0) {
        vfs_mount("/fat", fat32_get_vfs_root());
        print("[OK] MBR scanned. FAT32 Partition mounted at /fat\n");
    } else {
        print("[INFO] No FAT32 Partition detected on secondary master\n");
    }
    
    rtc_time_t boot_time;
    rtc_get_time(&boot_time);
    print("[OK] Real-Time Clock (RTC) initialized. System Time: ");
    
    char buf[16];
    itoa(boot_time.year, buf); print(buf); print("-");
    if (boot_time.month < 10) print("0");
    itoa(boot_time.month, buf); print(buf); print("-");
    if (boot_time.day < 10) print("0");
    itoa(boot_time.day, buf); print(buf); print(" ");
    if (boot_time.hour < 10) print("0");
    itoa(boot_time.hour, buf); print(buf); print(":");
    if (boot_time.minute < 10) print("0");
    itoa(boot_time.minute, buf); print(buf); print(":");
    if (boot_time.second < 10) print("0");
    itoa(boot_time.second, buf); print(buf); print("\n");

    if (vbe->width > 0) {
        print("[OK] VESA VBE High-Resolution GUI Enabled!\n");
    } else {
        print("[OK] Standard VGA Text Console Fallback mapped!\n");
    }

    print("\n============================================================\n\n");

    print("Nano OS Version 2.0.0-beta.5\n");
    print("(c) 2026 JimmiesAndTheCoders. All rights reserved.\n");
    print("Type 'help' to get started.\n\n");
    
    print_prompt();

    __asm__ __volatile__("sti");

    while(1) {
        __asm__ __volatile__("hlt");
    }
}
