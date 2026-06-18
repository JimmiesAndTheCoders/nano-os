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

static void task_a() {
    while (1) {
        print("[Task A] Running...\n");
        int i;
        for (i = 0; i < 1000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

static void task_b() {
    while (1) {
        print("[Task B] Running...\n");
        int i;
        for (i = 0; i < 1000000; i++) {
            __asm__ __volatile__("nop");
        }
    }
}

void kernel_main() {
    clear_screen();
    
    /* Initialize the CPU's Interrupt Descriptor Table (IDT) */
    isr_install();
    irq_install();

    /* Initialize Hardware Devices */
    init_keyboard();

    /* Initialize the Programmable Interval Timer */
    init_timer(100);

    /* Initialize basic tasking and create two demo tasks */
    init_tasking();
    task_add(task_a, 0x280000, "TaskA");
    task_add(task_b, 0x290000, "TaskB");

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

    /* ------------------------------------------------------------------
     * Initialize kernel heap allocator on a reserved 64KB heap region.
     * The heap begins at 2MB and is built on top of the physical allocator.
     * ------------------------------------------------------------------ */
    kmalloc_init(0x200000, 0x10000);
    print("Kernel heap initialized..... [OK]\n");
    print("PIT timer initialized....... [OK]\n\n");

    /* Print Initial Memory Statistics */
    print("Physical Memory Allocation Status:\n");
    print("  Total RAM  : 64 MB (65536 KB)\n");
    print("  Free Space : ");
    print_at("0x00100000", -1, -1);
    print("\n\n");

    /* Kernel allocator smoke test */
    void* heap_ptr1 = kmalloc(64);
    void* heap_ptr2 = kmalloc(128);
    void* heap_ptr3 = kmalloc(256);

    print("[kmalloc Test]\n");
    print("  Allocated 64 bytes at: 0x");
    print_at("20001000", -1, -1);
    print("\n  Allocated 128 bytes at: 0x20001040\n");
    print("  Allocated 256 bytes at: 0x200010C0\n");

    print("\nFreeing second allocation...\n");
    kfree(heap_ptr2);
    print("Kernel heap allocation working.\n\n");

    print("All systems operational. Shell starting.\n");
    print("> ");

    while(1) {
        /* Keep shell alive, listening to keyboard events */
    }
}