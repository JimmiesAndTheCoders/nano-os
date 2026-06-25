#include "sys/syscall.h"

int __syscall0(int num) {
    int val;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(val)
        : "a"(num)
        : "memory"
    );
    return val;
}

int __syscall1(int num, int p1) {
    int val;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(val)
        : "a"(num), "b"(p1)
        : "memory"
    );
    return val;
}

int __syscall2(int num, int p1, int p2) {
    int val;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(val)
        : "a"(num), "b"(p1), "c"(p2)
        : "memory"
    );
    return val;
}

int __syscall3(int num, int p1, int p2, int p3) {
    int val;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(val)
        : "a"(num), "b"(p1), "c"(p2), "d"(p3)
        : "memory"
    );
    return val;
}