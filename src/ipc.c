#include "ipc.h"
#include "pmm.h"
#include "util.h"
#include "kmalloc.h"
#include "task.h"
#include "timer.h"
#include "screen.h"

static pipe_t pipes[MAX_PIPES];
static mailbox_t mailboxes[MAX_MAILBOXES];
static shm_segment_t shm_segments[MAX_SHM_SEGMENTS];

static vfs_node_t ipc_root;
static vfs_node_t ipc_nodes[12];
static struct dirent ipc_dirent;

static int strncmp_local(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

static void map_page_shm(unsigned int* pd, unsigned int vaddr, unsigned int paddr, unsigned int flags) {
    unsigned int pde_idx = vaddr >> 22;
    unsigned int pte_idx = (vaddr >> 12) & 0x3FF;

    if ((pd[pde_idx] & 0x1) == 0) {
        unsigned int* pt = (unsigned int*)pmm_alloc_block();
        for (int i = 0; i < 1024; i++) {
            pt[i] = 0;
        }
        pd[pde_idx] = (unsigned int)pt | flags;
    }

    unsigned int* pt = (unsigned int*)(pd[pde_idx] & 0xFFFFF000);
    pt[pte_idx] = (paddr & 0xFFFFF000) | flags;

    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void init_ipc() {
    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].head = 0;
        pipes[i].tail = 0;
        pipes[i].count = 0;
        pipes[i].active = 1;
    }

    for (int i = 0; i < MAX_MAILBOXES; i++) {
        mailboxes[i].head = 0;
        mailboxes[i].tail = 0;
        mailboxes[i].count = 0;
        mailboxes[i].active = 1;
    }

    for (int i = 0; i < MAX_SHM_SEGMENTS; i++) {
        shm_segments[i].phys_addr = (unsigned int)pmm_alloc_block();
        memset((void*)shm_segments[i].phys_addr, 0, SHM_SIZE);
        shm_segments[i].active = 1;
    }
}

/* VFS Pipe Wrappers */
static unsigned int pipe_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_PIPES) return 0;
    pipe_t* p = &pipes[id];
    if (!p->active) return 0;

    unsigned int bytes_read = 0;
    while (bytes_read < size && p->count > 0) {
        buffer[bytes_read++] = p->buffer[p->tail];
        p->tail = (p->tail + 1) % PIPE_BUF_SIZE;
        p->count--;
    }
    return bytes_read;
}

static unsigned int pipe_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_PIPES) return 0;
    pipe_t* p = &pipes[id];
    if (!p->active) return 0;

    unsigned int bytes_written = 0;
    while (bytes_written < size && p->count < PIPE_BUF_SIZE) {
        p->buffer[p->head] = buffer[bytes_written++];
        p->head = (p->head + 1) % PIPE_BUF_SIZE;
        p->count++;
    }
    return bytes_written;
}

/* VFS Mailbox Wrappers */
static unsigned int mbox_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_MAILBOXES) return 0;
    mailbox_t* m = &mailboxes[id];
    if (!m->active || m->count == 0) return 0;

    mbox_msg_t* msg = &m->messages[m->tail];
    unsigned int len = msg->size;
    if (len > size) len = size;

    memory_copy(msg->data, (char*)buffer, len);
    m->tail = (m->tail + 1) % MBOX_QUEUE_SIZE;
    m->count--;
    return len;
}

static unsigned int mbox_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_MAILBOXES) return 0;
    mailbox_t* m = &mailboxes[id];
    if (!m->active) return 0;
    if (m->count >= MBOX_QUEUE_SIZE) return 0;

    mbox_msg_t* msg = &m->messages[m->head];
    unsigned int len = size;
    if (len >= MBOX_MAX_MSG_SIZE) len = MBOX_MAX_MSG_SIZE - 1;

    memory_copy((const char*)buffer, msg->data, len);
    msg->data[len] = '\0';
    msg->size = len;
    msg->sender_pid = task_get_current();

    m->head = (m->head + 1) % MBOX_QUEUE_SIZE;
    m->count++;
    return len;
}

/* VFS Shared Memory Wrappers */
static unsigned int shm_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_SHM_SEGMENTS) return 0;
    shm_segment_t* s = &shm_segments[id];
    if (!s->active) return 0;

    if (offset >= SHM_SIZE) return 0;
    if (offset + size > SHM_SIZE) size = SHM_SIZE - offset;

    memory_copy((const char*)(s->phys_addr + offset), (char*)buffer, size);
    return size;
}

static unsigned int shm_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    int id = node->impl;
    if (id < 0 || id >= MAX_SHM_SEGMENTS) return 0;
    shm_segment_t* s = &shm_segments[id];
    if (!s->active) return 0;

    if (offset >= SHM_SIZE) return 0;
    if (offset + size > SHM_SIZE) size = SHM_SIZE - offset;

    memory_copy((const char*)buffer, (char*)(s->phys_addr + offset), size);
    return size;
}

static struct dirent* ipc_readdir(vfs_node_t* node, unsigned int index) {
    if (index >= 12) return 0;

    if (index < 4) {
        char name[] = "pipe0";
        name[4] = '0' + index;
        memory_copy(name, ipc_dirent.name, 6);
    } else if (index < 8) {
        char name[] = "mbox0";
        name[4] = '0' + (index - 4);
        memory_copy(name, ipc_dirent.name, 6);
    } else {
        char name[] = "shm0";
        name[3] = '0' + (index - 8);
        memory_copy(name, ipc_dirent.name, 5);
    }

    ipc_dirent.inode = index + 1;
    return &ipc_dirent;
}

static vfs_node_t* ipc_finddir(vfs_node_t* node, const char* name) {
    int index = -1;
    if (strncmp_local(name, "pipe", 4) == 0 && name[4] >= '0' && name[4] <= '3' && name[5] == '\0') {
        index = name[4] - '0';
    } else if (strncmp_local(name, "mbox", 4) == 0 && name[4] >= '0' && name[4] <= '3' && name[5] == '\0') {
        index = 4 + (name[4] - '0');
    } else if (strncmp_local(name, "shm", 3) == 0 && name[3] >= '0' && name[3] <= '3' && name[4] == '\0') {
        index = 8 + (name[3] - '0');
    }

    if (index != -1) {
        return vfs_clone(&ipc_nodes[index]);
    }
    return 0;
}

vfs_node_t* ipc_get_vfs_root() {
    memory_copy("ipc", ipc_root.name, 4);
    ipc_root.flags = VFS_DIRECTORY;
    ipc_root.inode = 999;
    ipc_root.length = 0;
    ipc_root.impl = 0;
    ipc_root.read = 0;
    ipc_root.write = 0;
    ipc_root.readdir = ipc_readdir;
    ipc_root.finddir = ipc_finddir;
    ipc_root.create = 0;
    ipc_root.open = 0;
    ipc_root.close = 0;
    ipc_root.ioctl = 0;

    for (int i = 0; i < 4; i++) {
        char name[] = "pipe0";
        name[4] = '0' + i;
        memory_copy(name, ipc_nodes[i].name, 6);
        ipc_nodes[i].flags = VFS_FILE;
        ipc_nodes[i].inode = i + 1;
        ipc_nodes[i].length = PIPE_BUF_SIZE;
        ipc_nodes[i].impl = i;
        ipc_nodes[i].read = pipe_vfs_read;
        ipc_nodes[i].write = pipe_vfs_write;
        ipc_nodes[i].readdir = 0;
        ipc_nodes[i].finddir = 0;
        ipc_nodes[i].create = 0;
        ipc_nodes[i].open = 0;
        ipc_nodes[i].close = 0;
        ipc_nodes[i].ioctl = 0;
    }

    for (int i = 0; i < 4; i++) {
        char name[] = "mbox0";
        name[4] = '0' + i;
        memory_copy(name, ipc_nodes[4 + i].name, 6);
        ipc_nodes[4 + i].flags = VFS_FILE;
        ipc_nodes[4 + i].inode = 4 + i + 1;
        ipc_nodes[4 + i].length = MBOX_MAX_MSG_SIZE;
        ipc_nodes[4 + i].impl = i;
        ipc_nodes[4 + i].read = mbox_vfs_read;
        ipc_nodes[4 + i].write = mbox_vfs_write;
        ipc_nodes[4 + i].readdir = 0;
        ipc_nodes[4 + i].finddir = 0;
        ipc_nodes[4 + i].create = 0;
        ipc_nodes[4 + i].open = 0;
        ipc_nodes[4 + i].close = 0;
        ipc_nodes[4 + i].ioctl = 0;
    }

    for (int i = 0; i < 4; i++) {
        char name[] = "shm0";
        name[3] = '0' + i;
        memory_copy(name, ipc_nodes[8 + i].name, 5);
        ipc_nodes[8 + i].flags = VFS_FILE;
        ipc_nodes[8 + i].inode = 8 + i + 1;
        ipc_nodes[8 + i].length = SHM_SIZE;
        ipc_nodes[8 + i].impl = i;
        ipc_nodes[8 + i].read = shm_vfs_read;
        ipc_nodes[8 + i].write = shm_vfs_write;
        ipc_nodes[8 + i].readdir = 0;
        ipc_nodes[8 + i].finddir = 0;
        ipc_nodes[8 + i].create = 0;
        ipc_nodes[8 + i].open = 0;
        ipc_nodes[8 + i].close = 0;
        ipc_nodes[8 + i].ioctl = 0;
    }

    return &ipc_root;
}

/* Sycall Implementations */
int sys_pipe_create(int id) {
    if (id < 0 || id >= MAX_PIPES) return -1;
    pipes[id].head = 0;
    pipes[id].tail = 0;
    pipes[id].count = 0;
    pipes[id].active = 1;
    return 0;
}

int sys_pipe_read(int id, char* buf, unsigned int count) {
    if (id < 0 || id >= MAX_PIPES) return -1;
    pipe_t* p = &pipes[id];
    if (!p->active) return -1;

    unsigned int bytes_read = 0;
    while (bytes_read < count && p->count > 0) {
        buf[bytes_read++] = p->buffer[p->tail];
        p->tail = (p->tail + 1) % PIPE_BUF_SIZE;
        p->count--;
    }
    return bytes_read;
}

int sys_pipe_write(int id, const char* buf, unsigned int count) {
    if (id < 0 || id >= MAX_PIPES) return -1;
    pipe_t* p = &pipes[id];
    if (!p->active) return -1;

    unsigned int bytes_written = 0;
    while (bytes_written < count && p->count < PIPE_BUF_SIZE) {
        p->buffer[p->head] = buf[bytes_written++];
        p->head = (p->head + 1) % PIPE_BUF_SIZE;
        p->count++;
    }
    return bytes_written;
}

int sys_mbox_send(int id, const char* msg, unsigned int size) {
    if (id < 0 || id >= MAX_MAILBOXES) return -1;
    mailbox_t* m = &mailboxes[id];
    if (!m->active) return -1;
    if (m->count >= MBOX_QUEUE_SIZE) return -1;

    mbox_msg_t* m_msg = &m->messages[m->head];
    unsigned int len = size;
    if (len >= MBOX_MAX_MSG_SIZE) len = MBOX_MAX_MSG_SIZE - 1;

    memory_copy(msg, m_msg->data, len);
    m_msg->data[len] = '\0';
    m_msg->size = len;
    m_msg->sender_pid = task_get_current();

    m->head = (m->head + 1) % MBOX_QUEUE_SIZE;
    m->count++;
    return 0;
}

int sys_mbox_recv(int id, char* msg, unsigned int max_size) {
    if (id < 0 || id >= MAX_MAILBOXES) return -1;
    mailbox_t* m = &mailboxes[id];
    if (!m->active || m->count == 0) return -1;

    mbox_msg_t* m_msg = &m->messages[m->tail];
    unsigned int len = m_msg->size;
    if (len > max_size) len = max_size;

    memory_copy(m_msg->data, msg, len);
    m->tail = (m->tail + 1) % MBOX_QUEUE_SIZE;
    m->count--;
    return len;
}

void* sys_shm_at(int id, unsigned int virtual_addr) {
    if (id < 0 || id >= MAX_SHM_SEGMENTS) return (void*)-1;
    shm_segment_t* s = &shm_segments[id];
    if (!s->active) return (void*)-1;

    unsigned int cr3;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
    unsigned int* pd = (unsigned int*)cr3;

    map_page_shm(pd, virtual_addr, s->phys_addr, 7);
    return (void*)virtual_addr;
}

/* Multitasking Demonstration Code */
static void demo_sender_task() {
    sys_pipe_write(1, "Multitasking Pipe Success!", 27);
    sys_mbox_send(1, "Mailbox Message Dispatched!", 27);

    char* shm = (char*)sys_shm_at(1, 0x1000000);
    if (shm != (char*)-1) {
        memory_copy("Shared Memory Sync Verified!", shm, 29);
    }

    int my_pid = task_get_current();
    task_terminate(my_pid);
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

void run_ipc_demo() {
    print("[IPC DEMO] Clearing pipe 1, mailbox 1, and shared memory 1...\n");
    sys_pipe_create(1);

    mailbox_t* m = &mailboxes[1];
    m->head = m->tail = m->count = 0;

    memset((void*)shm_segments[1].phys_addr, 0, SHM_SIZE);

    print("[IPC DEMO] Adding sender task into scheduler rotation...\n");
    task_add(demo_sender_task, "ipc_sender");

    print("[IPC DEMO] Relinquishing CPU control to let task run...\n");
    unsigned int start_ticks = timer_get_ticks();
    while (timer_get_ticks() - start_ticks < 30) {
        __asm__ __volatile__("hlt");
    }

    print("[IPC DEMO] CPU control recovered. Reading IPC structures via syscalls:\n");

    char pipe_buf[64];
    memset(pipe_buf, 0, sizeof(pipe_buf));
    int pipe_res = sys_pipe_read(1, pipe_buf, sizeof(pipe_buf) - 1);
    print("  Pipe read content  (");
    char num_buf[16];
    itoa(pipe_res, num_buf); print(num_buf); print(" bytes): ");
    print(pipe_buf); print("\n");

    char mbox_buf[64];
    memset(mbox_buf, 0, sizeof(mbox_buf));
    int mbox_res = sys_mbox_recv(1, mbox_buf, sizeof(mbox_buf) - 1);
    print("  Mailbox msg content (");
    itoa(mbox_res, num_buf); print(num_buf); print(" bytes): ");
    print(mbox_buf); print("\n");

    char* mapped_shm = (char*)sys_shm_at(1, 0x2000000);
    if (mapped_shm != (char*)-1) {
        print("  SHM mapped at 0x2000000. Read Content: ");
        print(mapped_shm); print("\n");
    } else {
        print("  SHM Mapping Failed!\n");
    }
    print("[IPC DEMO] Demonstration finalized.\n");
}