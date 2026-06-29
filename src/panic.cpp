#include "panic.h"
#include "vbe.h"
#include "vga_screen.hpp"

extern "C" {
    #include "screen.h"   // This contains the 'print' declaration
    #include "graphics.h" // This contains 'fill_rect'
    #include "util.h"     // This contains 'itoa'
}

struct stack_frame {
    struct stack_frame* ebp;
    unsigned int eip;
};

void kpanic_backtrace(unsigned int max_frames) {
    struct stack_frame* stk;
    __asm__ __volatile__ ("movl %%ebp, %0" : "=r" (stk));

    print("Call Stack Trace:\n");
    for(unsigned int frame = 0; stk && frame < max_frames; ++frame) {
        // Sanity check: stack usually lives in lower memory in this OS
        if ((unsigned int)stk < 0x1000 || (unsigned int)stk > 0x900000) break;

        char addr_buf[16];
        itoa((int)stk->eip, addr_buf);
        print("  [");
        print(addr_buf);
        print("]\n");

        stk = stk->ebp;
    }
}

extern "C" void kpanic(const char* message, registers_t* regs) {
    __asm__ __volatile__ ("cli");

    screen.show_cursor(false);
    screen.set_background_color(0xFF770000);
    
    if (vbe_info->width > 0) {
        fill_rect(0, 0, vbe_info->width, vbe_info->height, 0xFF770000);
    }

    screen.set_cursor(0, 0);
    
    print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    print("  KERNEL PANIC\n");
    print("  Message: "); 
    print(message); 
    print("\n");
    print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

    if (regs) {
        char buf[16];
        print("Registers: EIP=");
        itoa(regs->eip, buf); print(buf);
        print(" CS="); 
        itoa(regs->cs, buf); print(buf);
        print("\n");
    }

    kpanic_backtrace(10);

    print("\nSystem halted. Please manual restart.");
    
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}