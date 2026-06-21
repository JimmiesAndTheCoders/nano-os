#include "shell.h"
#include "screen.h"
#include "util.h"
#include "initrd.h"

void print_prompt() {
    print("nano> ");
}

void process_command(char *input) {
    if (strlen(input) == 0) {
        print_prompt();
        return;
    }

    if (strcmp(input, "help") == 0) {
        print("Available Commands:\n");
        print("  help         - Display this reference panel.\n");
        print("  clear        - Clear the display terminal.\n");
        print("  ls           - List available files on the RAM disk.\n");
        print("  cat [file]   - Display the contents of a file.\n");
        print("  status       - Inspect system operational metrics.\n");
        print("  nano --status- Execute system mascot configuration check.\n");
        print("  halt         - Safely power down the hardware processor.\n");
    } 
    else if (strcmp(input, "clear") == 0) {
        clear_screen();
    } 
    else if (strcmp(input, "ls") == 0) {
        list_files();
    } 
    else if (strcmp(input, "cat ") == 0 || (input[0]=='c' && input[1]=='a' && input[2]=='t')) {
        if (strlen(input) > 4) {
            char* filename = input + 4; 
            char* content = read_file(filename);
            if (content) {
                print(content);
                print("\n");
            } else {
                print("Error: File '");
                print(filename);
                print("' not found.\n");
            }
        } else {
            print("Usage: cat [filename]\n");
        }
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

    print("\n");
    print_prompt();
}