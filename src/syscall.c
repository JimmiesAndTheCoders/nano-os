#include "syscall.h"
#include "screen.h"
#include "initrd.h"
#include "timer.h"

static void *syscalls[4];

unsigned int syscall_handler(registers_t *regs) {
    if (regs->eax == SYS_PRINT) {
        char* msg = (char*)regs->ebx;
        if (msg) print(msg);
    } 
    else if (regs->eax == SYS_READ_FILE) {
        char* name = (char*)regs->ebx;
        regs->eax = (unsigned int)read_file(name);
    }
    else if (regs->eax == SYS_LIST_FILES) {
        list_files("/");
    }
    else if (regs->eax == SYS_GET_TICKS) {
        regs->eax = timer_get_ticks();
    }
    return (unsigned int)regs;
}

void init_syscalls() {
    register_interrupt_handler(0x80, syscall_handler);
}