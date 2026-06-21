#ifndef SYSCALL_H
#define SYSCALL_H

#include "cpu.h"

/* Syscall defines */
#define SYS_PRINT     0
#define SYS_READ_FILE 1
#define SYS_LIST_FILES 2
#define SYS_GET_TICKS 3

void init_syscalls();
void syscall_handler(registers_t *regs);

#endif