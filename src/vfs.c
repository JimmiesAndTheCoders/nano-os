#include "vfs.h"
#include "kmalloc.h"
#include "util.h"
#include "screen.h"
#include "cache.h"

#define MAX_MOUNTS 16

typedef struct {
    char path[128];
    vfs_node_t* root_node;
} vfs_mount_t;

static vfs_mount_t mount_table[MAX_MOUNTS];
static int mount_count = 0;

vfs_node_t* vfs_root = 0;
static vfs_node_t root_node;
static struct dirent g_dirent;

static vfs_node_t* get_mount_at(const char* path) {
    for (int i = 0; i < mount_count; i++) {
        if (strcmp(mount_table[i].path, path) == 0) {
            return mount_table[i].root_node;
        }
    }
    return 0;
}

static struct dirent* root_readdir(vfs_node_t* node, unsigned int index) {
    if (index >= (unsigned int)mount_count) return 0;
    
    const char* path = mount_table[index].path;
    if (path[0] == '/') path++;
    
    memory_copy(path, g_dirent.name, strlen(path) + 1);
    g_dirent.inode = index + 1;
    return &g_dirent;
}

static vfs_node_t* root_finddir(vfs_node_t* node, const char* name) {
    char full_mount[132] = "/";
    memory_copy(name, full_mount + 1, strlen(name) + 1);
    
    vfs_node_t* mounted = get_mount_at(full_mount);
    if (mounted) return vfs_clone(mounted);
    return 0;
}

void init_vfs() {
    memory_copy("/", root_node.name, 2);
    root_node.flags = VFS_DIRECTORY;
    root_node.inode = 0;
    root_node.length = 0;
    root_node.impl = 0;
    root_node.read = 0;
    root_node.write = 0;
    root_node.readdir = root_readdir;
    root_node.finddir = root_finddir;
    root_node.create = 0;
    root_node.open = 0;
    root_node.close = 0;
    root_node.ioctl = 0;
    
    vfs_root = &root_node;
}

vfs_node_t* vfs_clone(vfs_node_t* node) {
    if (!node) return 0;
    vfs_node_t* clone = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    memory_copy((const char*)node, (char*)clone, sizeof(vfs_node_t));
    return clone;
}

unsigned int vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    if (node && node->read) {
        if (node->flags == VFS_FILE) {
            return page_cache_read(node, offset, size, buffer);
        }
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

unsigned int vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    if (node && node->write) {
        if (node->flags == VFS_FILE) {
            return page_cache_write(node, offset, size, buffer);
        }
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

void vfs_close(vfs_node_t* node) {
    if (node) {
        if (node->flags == VFS_FILE) {
            page_cache_flush_node(node);
        }
        if (node != vfs_root && node->flags != VFS_MOUNTPOINT) {
            kfree(node);
        }
    }
}

void vfs_open(vfs_node_t* node) {
    if (node && node->open) {
        node->open(node);
    }
}

struct dirent* vfs_readdir(vfs_node_t* node, unsigned int index) {
    if (node && node->readdir) {
        return node->readdir(node, index);
    }
    return 0;
}

vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name) {
    if (node && node->finddir) {
        return node->finddir(node, name);
    }
    return 0;
}

int vfs_mount(const char* path, vfs_node_t* root_node) {
    if (mount_count >= MAX_MOUNTS) return -1;
    memory_copy(path, mount_table[mount_count].path, strlen(path) + 1);
    mount_table[mount_count].root_node = root_node;
    root_node->flags = VFS_MOUNTPOINT;
    mount_count++;
    return 0;
}

vfs_node_t* vfs_resolve_path(const char* path) {
    if (!path || path[0] != '/') return 0;
    if (strcmp(path, "/") == 0) return vfs_root;
    
    vfs_node_t* curr = vfs_root;
    char path_copy[256];
    memory_copy(path, path_copy, strlen(path) + 1);
    
    char current_accum[256] = "";
    int accum_len = 0;
    
    char* token = path_copy + 1;
    while (token && *token != '\0') {
        char* slash = token;
        while (*slash != '/' && *slash != '\0') slash++;
        
        char original_char = *slash;
        *slash = '\0';
        
        current_accum[accum_len++] = '/';
        int tok_len = strlen(token);
        memory_copy(token, current_accum + accum_len, tok_len);
        accum_len += tok_len;
        current_accum[accum_len] = '\0';
        
        vfs_node_t* mounted = get_mount_at(current_accum);
        if (mounted) {
            curr = mounted;
        } else {
            if (!curr->finddir) return 0;
            vfs_node_t* next_node = curr->finddir(curr, token);
            if (curr != vfs_root && curr->flags != VFS_MOUNTPOINT) {
                vfs_close(curr);
            }
            curr = next_node;
            if (!curr) return 0;
        }
        
        if (original_char == '\0') {
            break;
        }
        token = slash + 1;
    }
    return curr;
}