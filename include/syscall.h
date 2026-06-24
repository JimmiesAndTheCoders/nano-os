#ifndef SYSCALL_H
#define SYSCALL_H

#include "cpu.h"

/* Syscall defines */
#define SYS_PRINT      0
#define SYS_READ_FILE  1
#define SYS_LIST_FILES 2
#define SYS_GET_TICKS  3
#define SYS_GET_TIME   4
#define SYS_KILL       5

void init_syscalls();
unsigned int syscall_handler(registers_t *regs);

#endif