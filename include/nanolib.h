#ifndef NANOLIB_H
#define NANOLIB_H

#include "syscall.h"

static void nano_print(char* msg) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYS_PRINT), "b"(msg));
}

static unsigned int nano_get_ticks() {
    unsigned int ticks;
    __asm__ __volatile__ ("int $0x80" : "=a"(ticks) : "a"(SYS_GET_TICKS));
    return ticks;
}

#endif