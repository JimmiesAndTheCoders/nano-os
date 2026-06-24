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

static inline void nano_kill(int pid, int sig) {
    __asm__ __volatile__ ("int $0x80" : : "a"(SYS_KILL), "b"(pid), "c"(sig));
}

static inline int nano_pipe_create(int id) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_PIPE_CREATE), "b"(id));
    return ret;
}

static inline int nano_pipe_read(int id, char* buf, unsigned int count) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_PIPE_READ), "b"(id), "c"(buf), "d"(count));
    return ret;
}

static inline int nano_pipe_write(int id, const char* buf, unsigned int count) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_PIPE_WRITE), "b"(id), "c"(buf), "d"(count));
    return ret;
}

static inline int nano_mbox_send(int id, const char* msg, unsigned int size) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_MBOX_SEND), "b"(id), "c"(msg), "d"(size));
    return ret;
}

static inline int nano_mbox_recv(int id, char* msg, unsigned int max_size) {
    int ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_MBOX_RECV), "b"(id), "c"(msg), "d"(max_size));
    return ret;
}

static inline void* nano_shm_at(int id, unsigned int virtual_addr) {
    void* ret;
    __asm__ __volatile__ ("int $0x80" : "=a"(ret) : "a"(SYS_SHM_AT), "b"(id), "c"(virtual_addr));
    return ret;
}

#endif