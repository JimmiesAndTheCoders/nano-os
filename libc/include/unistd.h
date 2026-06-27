#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>

/* Standard System Calls */
int write(int fd, const void *buf, size_t count);
int read(int fd, void *buf, size_t count);
unsigned int get_ticks(void);
void *sbrk(int increment);
int kill(int pid, int sig);

/* IPC System Calls */
int pipe_create(int id);
int pipe_read(int id, char* buf, unsigned int count);
int pipe_write(int id, const char* buf, unsigned int count);
int mbox_send(int id, const char* msg, unsigned int size);
int mbox_recv(int id, char* msg, unsigned int max_size);
void* shm_at(int id, unsigned int virtual_addr);

#endif