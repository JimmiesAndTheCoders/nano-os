#ifndef IPC_H
#define IPC_H

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PIPE_BUF_SIZE 2048
#define MBOX_MAX_MSG_SIZE 256
#define MBOX_QUEUE_SIZE 16
#define MAX_PIPES 4
#define MAX_MAILBOXES 4
#define MAX_SHM_SEGMENTS 4
#define SHM_SIZE 4096

typedef struct {
    unsigned char buffer[PIPE_BUF_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
    int active;
} pipe_t;

typedef struct {
    char data[MBOX_MAX_MSG_SIZE];
    unsigned int size;
    int sender_pid;
} mbox_msg_t;

typedef struct {
    mbox_msg_t messages[MBOX_QUEUE_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
    int active;
} mailbox_t;

typedef struct {
    unsigned int phys_addr;
    int active;
} shm_segment_t;

void init_ipc();
vfs_node_t* ipc_get_vfs_root();
void run_ipc_demo();

/* System call implementations */
int sys_pipe_create(int id);
int sys_pipe_read(int id, char* buf, unsigned int count);
int sys_pipe_write(int id, const char* buf, unsigned int count);

int sys_mbox_send(int id, const char* msg, unsigned int size);
int sys_mbox_recv(int id, char* msg, unsigned int max_size);

void* sys_shm_at(int id, unsigned int virtual_addr);

#ifdef __cplusplus
}
#endif

#endif