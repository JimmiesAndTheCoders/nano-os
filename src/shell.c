#include "shell.h"
#include "screen.h"
#include "util.h"

void print_prompt() {
    print("nano> ");
}

void process_command(char *input) {
    if (strcmp(input, "help") == 0) {
        print("Available Commands:\n");
        print("  help         - Display this reference panel.\n");
        print("  clear        - Clear the display terminal.\n");
        print("  status       - Inspect system operational metrics.\n");
        print("  nano --status- Execute system mascot configuration check.\n");
        print("  halt         - Safely power down the hardware processor.\n");
    } 
    else if (strcmp(input, "clear") == 0) {
        clear_screen();
    } 
    else if (strcmp(input, "status") == 0) {
        print("Nano OS System Status:\n");
        print("  Core state : RUNNING (32-bit Protected Mode)\n");
        print("  Memory map : OK [Segment Limits: 4GB Flat]\n");
        print("  Interrupts : ACTIVE [IDT installed, PIC initialized]\n");
    } 
    else if (strcmp(input, "nano --status") == 0) {
        print("      \\|/  \n");
        print("     (o o)  \n");
        print("~~oOO-(_)-OOo~~\n");
        print("Nano is currently grounded. System restricted!\n");
        print("Please contact Scratch mascot security operators.\n");
    } 
    else if (strcmp(input, "halt") == 0) {
        print("Halting the system safely. Goodbye!\n");
        __asm__ __volatile__("cli\n\thlt");
    } 
    else if (strlen(input) == 0) {
        /* User hit enter without typing anything, do nothing */
    } 
    else {
        print("Error: Unknown command '");
        print(input);
        print("'. Type 'help' for instructions.\n");
    }

    /* Print a fresh prompt on a new line */
    if (strcmp(input, "clear") != 0) {
        print("\n");
    }
    print_prompt();
}