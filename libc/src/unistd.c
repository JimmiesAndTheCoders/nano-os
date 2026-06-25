#include "unistd.h"
#include "sys/syscall.h"

int write(int fd, const void *buf, size_t count) {
    if (fd == 1 || fd == 2) { // stdout or stderr
        const char *s = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            char temp[2] = {s[i], '\0'};
            __syscall1(SYS_PRINT, (int)temp);
        }
        return count;
    }
    return -1;
}

int read(int fd, void *buf, size_t count) {
    (void)fd; (void)buf; (void)count;
    return -1; // Unimplemented for now
}

unsigned int get_ticks(void) {
    return (unsigned int)__syscall0(SYS_GET_TICKS);
}