#include "syscall.h"
#include "screen.h"
#include "initrd.h"
#include "timer.h"
#include "rtc.h"
#include "task.h"
#include "ipc.h"
#include "pmm.h"
#include "util.h"

extern void map_page(unsigned int* pd, unsigned int vaddr, unsigned int paddr, unsigned int flags);

static unsigned int sys_sbrk(int increment) {
    int pid = task_get_current();
    unsigned int old_heap_end = task_get_heap_end(pid);
    if (old_heap_end == 0) {
        return 0; // Kernel task / invalid boundary
    }

    if (increment == 0) {
        return old_heap_end;
    }

    unsigned int new_heap_end = old_heap_end + increment;
    unsigned int pd_addr = task_get_page_directory(pid);
    unsigned int* pd = (unsigned int*)pd_addr;

    // Allocate and map physical pages dynamically across 4KB page crossings
    unsigned int page_start = (old_heap_end + 4095) & ~4095;
    unsigned int page_end = (new_heap_end + 4095) & ~4095;

    for (unsigned int vaddr = page_start; vaddr < page_end; vaddr += 4096) {
        void* phys = pmm_alloc_block();
        if (!phys) {
            return 0; // Out of memory
        }
        memset(phys, 0, 4096);
        map_page(pd, vaddr, (unsigned int)phys, 7); // Present + Read/Write + User (Ring 3)
    }

    task_set_heap_end(pid, new_heap_end);
    return old_heap_end;
}

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
    else if (regs->eax == SYS_SBRK) {
        regs->eax = sys_sbrk((int)regs->ebx);
    }
    return (unsigned int)regs;
}

void init_syscalls() {
    register_interrupt_handler(0x80, syscall_handler);
}