/* ==============================================================================
 * NANO OS - C Kernel Core
 * Description: The main entry point of our operating system.
 * ============================================================================== */

#include "screen.h"
#include "cpu.h"
#include "keyboard.h"
#include "shell.h"

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
    print("Keyboard driver loaded....... [OK]\n\n");
    
    /* Start the shell */
    print_prompt();

    /* Keep running the processor while waiting for hardware interrupts */
    while(1) {
        __asm__ __volatile__("hlt");
    }
}