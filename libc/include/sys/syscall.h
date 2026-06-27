#ifndef _SYS_SYSCALL_H
#define _SYS_SYSCALL_H

/* System Call Vector Codes matching kernel registers */
#define SYS_PRINT       0
#define SYS_READ_FILE   1
#define SYS_LIST_FILES  2
#define SYS_GET_TICKS   3
#define SYS_GET_TIME    4
#define SYS_KILL        5

#define SYS_PIPE_CREATE 6
#define SYS_PIPE_READ   7
#define SYS_PIPE_WRITE  8
#define SYS_MBOX_SEND   9
#define SYS_MBOX_RECV   10
#define SYS_SHM_AT      12
#define SYS_SBRK        13

/* Register-packaging system call execution stubs */
int __syscall0(int num);
int __syscall1(int num, int p1);
int __syscall2(int num, int p1, int p2);
int __syscall3(int num, int p1, int p2, int p3);

#endif