#ifndef NANOLIB_H
#define NANOLIB_H

#include "syscall.h"
#include "rtc.h"

static inline void nano_print(char* msg) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYS_PRINT), "b"(msg));
}

static inline unsigned int nano_get_ticks() {
    unsigned int ticks;
    __asm__ __volatile__ ("int $0x80" : "=a"(ticks) : "a"(SYS_GET_TICKS));
    return ticks;
}

static inline void nano_get_time(rtc_time_t* t) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYS_GET_TIME), "b"(t));
}

#endif