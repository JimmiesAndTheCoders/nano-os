#ifndef VFS_H
#define VFS_H

#ifdef __cplusplus
extern "C" {
#endif

#define VFS_FILE        0x01
#define VFS_DIRECTORY   0x02
#define VFS_CHARDEVICE  0x03
#define VFS_BLOCKDEVICE 0x04
#define VFS_PIPE        0x05
#define VFS_SYMLINK     0x06
#define VFS_MOUNTPOINT  0x08
#define MAX_MOUNTS 16

struct vfs_node;

typedef unsigned int (*read_type_t)(struct vfs_node*, unsigned int, unsigned int, unsigned char*);
typedef unsigned int (*write_type_t)(struct vfs_node*, unsigned int, unsigned int, const unsigned char*);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);
typedef struct dirent* (*readdir_type_t)(struct vfs_node*, unsigned int);
typedef struct vfs_node* (*finddir_type_t)(struct vfs_node*, const char* name);
typedef int (*create_type_t)(struct vfs_node*, const char* name, unsigned int flags);
typedef int (*ioctl_type_t)(struct vfs_node*, unsigned long, void*);

typedef struct vfs_node {
    char name[128];
    unsigned int mask;
    unsigned int uid;
    unsigned int gid;
    unsigned int flags;
    unsigned int inode;
    unsigned int length;
    unsigned int impl; // Implementation-specific tag or descriptor index
    
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    create_type_t create;
    ioctl_type_t ioctl;
    
    struct vfs_node* ptr; // Symlink peer or Mountpoint root
} vfs_node_t;

struct dirent {
    char name[128];
    unsigned int inode;
};

typedef struct {
    char path[128];
    vfs_node_t* root_node;
} vfs_mount_t;

extern vfs_mount_t mount_table[MAX_MOUNTS];
extern int mount_count;

// Global root of the VFS
extern vfs_node_t* vfs_root;

// Core VFS API
void init_vfs();
vfs_node_t* vfs_clone(vfs_node_t* node);
void vfs_close(vfs_node_t* node);
unsigned int vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer);
unsigned int vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer);
void vfs_open(vfs_node_t* node);
struct dirent* vfs_readdir(vfs_node_t* node, unsigned int index);
vfs_node_t* vfs_finddir(vfs_node_t* node, const char* name);
int vfs_ioctl(vfs_node_t* node, unsigned long request, void* arg);

// Resolver and Mount Point API
vfs_node_t* vfs_resolve_path(const char* path);
int vfs_mount(const char* path, vfs_node_t* root_node);

#ifdef __cplusplus
}
#endif

#endif