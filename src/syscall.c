#include "syscall.h"
#include "screen.h"
#include "initrd.h"
#include "timer.h"
#include "rtc.h"
#include "task.h"
#include "ipc.h"

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
    else if (regs->eax == SYS_GET_TIME) {
        rtc_time_t *out_time = (rtc_time_t*)regs->ebx;
        if (out_time) {
            rtc_get_time(out_time);
        }
    }
    else if (regs->eax == SYS_KILL) {
        int pid = (int)regs->ebx;
        int sig = (int)regs->ecx;
        task_signal(pid, sig);
    }
    else if (regs->eax == SYS_PIPE_CREATE) {
        regs->eax = sys_pipe_create(regs->ebx);
    }
    else if (regs->eax == SYS_PIPE_READ) {
        regs->eax = sys_pipe_read(regs->ebx, (char*)regs->ecx, regs->edx);
    }
    else if (regs->eax == SYS_PIPE_WRITE) {
        regs->eax = sys_pipe_write(regs->ebx, (const char*)regs->ecx, regs->edx);
    }
    else if (regs->eax == SYS_MBOX_SEND) {
        regs->eax = sys_mbox_send(regs->ebx, (const char*)regs->ecx, regs->edx);
    }
    else if (regs->eax == SYS_MBOX_RECV) {
        regs->eax = sys_mbox_recv(regs->ebx, (char*)regs->ecx, regs->edx);
    }
    else if (regs->eax == SYS_SHM_AT) {
        regs->eax = (unsigned int)sys_shm_at(regs->ebx, regs->ecx);
    }
    return (unsigned int)regs;
}

void init_syscalls() {
    register_interrupt_handler(0x80, syscall_handler);
}