#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

int write(int fd, const void *buf, size_t count);
int read(int fd, void *buf, size_t count);
unsigned int get_ticks(void);

#endif