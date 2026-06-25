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

#define SYS_PIPE_CREATE 6
#define SYS_PIPE_READ   7
#define SYS_PIPE_WRITE  8
#define SYS_MBOX_SEND   9
#define SYS_MBOX_RECV   10
#define SYS_SHM_AT      12
#define SYS_SBRK        13 // FIX: POSIX dynamic break expansion identifier

void init_syscalls();
unsigned int syscall_handler(registers_t *regs);

#endif