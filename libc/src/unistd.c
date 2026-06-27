#include "unistd.h"
#include "sys/syscall.h"

int write(int fd, const void *buf, size_t count) {
    if (fd == 1 || fd == 2) { // Routing stdout & stderr to the display
        const char *s = (const char *)buf;
        char chunk[129];
        size_t remaining = count;
        size_t written = 0;

        while (remaining > 0) {
            size_t size = (remaining > 128) ? 128 : remaining;
            for (size_t i = 0; i < size; i++) {
                chunk[i] = s[written + i];
            }
            chunk[size] = '\0';
            __syscall1(SYS_PRINT, (int)chunk);
            remaining -= size;
            written += size;
        }
        return count;
    }
    return -1;
}

int read(int fd, void *buf, size_t count) {
    // Standard file read stub - files are processed directly via mounts for now
    (void)fd; (void)buf; (void)count;
    return -1;
}

unsigned int get_ticks(void) {
    return (unsigned int)__syscall0(SYS_GET_TICKS);
}

void *sbrk(int increment) {
    return (void *)__syscall1(SYS_SBRK, increment);
}

int kill(int pid, int sig) {
    return __syscall2(SYS_KILL, pid, sig);
}

/* IPC System Call Implementations */
int pipe_create(int id) {
    return __syscall1(SYS_PIPE_CREATE, id);
}

int pipe_read(int id, char* buf, unsigned int count) {
    return __syscall3(SYS_PIPE_READ, id, (int)buf, count);
}

int pipe_write(int id, const char* buf, unsigned int count) {
    return __syscall3(SYS_PIPE_WRITE, id, (int)buf, count);
}

int mbox_send(int id, const char* msg, unsigned int size) {
    return __syscall3(SYS_MBOX_SEND, id, (int)msg, size);
}

int mbox_recv(int id, char* msg, unsigned int max_size) {
    return __syscall3(SYS_MBOX_RECV, id, (int)msg, max_size);
}

void* shm_at(int id, unsigned int virtual_addr) {
    return (void*)__syscall2(SYS_SHM_AT, id, virtual_addr);
}