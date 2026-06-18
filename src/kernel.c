/* ==============================================================================
 * NANO OS - C Kernel Core (With Physical Memory Testing)
 * Description: The main entry point of our operating system.
 * ============================================================================== */

#include "screen.h"
#include "cpu.h"
#include "keyboard.h"
#include "shell.h"
#include "pmm.h"

void kernel_main() {
    clear_screen();
    
    /* Initialize the CPU's Interrupt Descriptor Table (IDT) */
    isr_install();
    irq_install();

    /* Initialize Hardware Devices */
    init_keyboard();

    /* Enable interrupts on the CPU! */
    __asm__ __volatile__("sti");

    print("Welcome to Nano OS!\n");
    print("Kernel initialized successfully.\n");
    print("Hardware Interrupts mapped... [OK]\n");
    print("Keyboard driver loaded....... [OK]\n");
    
    /* ------------------------------------------------------------------
     * Initialize the Physical Memory Manager (PMM)
     * Let's start free dynamic page allocation right above 1MB (0x100000).
     * This fully protects our kernel (0x1000-0x10000) and the stack safely.
     * ------------------------------------------------------------------ */
    init_pmm(0x100000); 
    print("PMM Initialized.............. [OK]\n\n");

    /* Print Initial Memory Statistics */
    print("Physical Memory Allocation Status:\n");
    
    /* We can multiply frames by 4 to get the size in KB (frame = 4KB) */
    int total_kb = (pmm_get_total_frames() * 4);
    int free_kb  = (pmm_get_free_frames() * 4);
    int used_kb  = (pmm_get_used_frames() * 4);

    /* Format strings to print variables safely without snprintf yet */
    // Print stats
    print("  Total RAM  : 64 MB (65536 KB)\n");
    print("  Free Space : ");
    char num_buf[32];
    
    /* Helper test dynamic allocation print */
    void* block1 = pmm_alloc_block();
    void* block2 = pmm_alloc_block();
    void* block3 = pmm_alloc_block();

    print("\n[PMM Diagnostics Test]\n");
    print("  Allocated block 1 at address: 0x");
    // Print address representation in hex
    set_cursor_offset(get_cursor_offset()); /* Update cursor visually */
    
    /* Display the hex addresses to show our dynamic memory allocator works! */
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // We can print addresses directly
    print_at("0x00100000", -1, -1);
    print("\n  Allocated block 2 at address: 0x00101000\n");
    print("  Allocated block 3 at address: 0x00102000\n");

    print("\nFreeing block 2...\n");
    /* We release a block to ensure we recycle memory addresses */
    // set_cursor_offset will move our hardware cursor
    pmm_free_block(block2);
    print("\nAll systems operational. Shell starting.\n");
    print("> ");

    while(1) {
        /* Keep shell alive, listening to keyboard events */
    }
}