// ==============================================================================
// INITRD - VFS Adapter Implementations
// ==============================================================================

static vfs_node_t initrd_root_node;
static struct dirent initrd_dirent;

unsigned int initrd_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    unsigned int idx = node->impl;
    if (idx >= header->nfiles) return 0;
    
    char* content = (char*)(root_address + header->files[idx].offset);
    unsigned int len = header->files[idx].length;
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    memory_copy(content + offset, (char*)buffer, size);
    return size;
}

unsigned int initrd_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    unsigned int idx = node->impl;
    if (idx >= header->nfiles) return 0;
    
    char* content = (char*)(root_address + header->files[idx].offset);
    unsigned int new_len = offset + size;
    if (new_len > header->files[idx].length) {
        char* new_buf = (char*)kmalloc(new_len + 1);
        if (header->files[idx].length > 0) {
            memory_copy(content, new_buf, header->files[idx].length);
        }
        memory_copy((const char*)buffer, new_buf + offset, size);
        new_buf[new_len] = '\0';
        write_file_content(header->files[idx].name, new_buf, new_len);
        kfree(new_buf);
    } else {
        memory_copy((const char*)buffer, content + offset, size);
    }
    node->length = header->files[idx].length;
    return size;
}

struct dirent* initrd_vfs_readdir(vfs_node_t* node, unsigned int index) {
    unsigned int count = 0;
    const char* parent_path = node->name;
    
    for (unsigned int i = 0; i < header->nfiles; i++) {
        const char* name = header->files[i].name;
        char std_name[64];
        if (name[0] != '/') {
            std_name[0] = '/';
            memory_copy(name, std_name + 1, strlen(name) + 1);
        } else {
            memory_copy(name, std_name, strlen(name) + 1);
        }
        
        if (is_direct_child(parent_path, std_name)) {
            if (count == index) {
                int parent_len = strlen(parent_path);
                const char* display_name = std_name + parent_len;
                if (display_name[0] == '/') display_name++;
                
                memory_copy(display_name, initrd_dirent.name, strlen(display_name) + 1);
                initrd_dirent.inode = i;
                return &initrd_dirent;
            }
            count++;
        }
    }
    return 0;
}

vfs_node_t* initrd_vfs_finddir(vfs_node_t* node, const char* name) {
    char target_path[128];
    int parent_len = strlen(node->name);
    memory_copy(node->name, target_path, parent_len);
    if (node->name[parent_len - 1] != '/') {
        target_path[parent_len] = '/';
        memory_copy(name, target_path + parent_len + 1, strlen(name) + 1);
    } else {
        memory_copy(name, target_path + parent_len, strlen(name) + 1);
    }
    
    for (unsigned int i = 0; i < header->nfiles; i++) {
        char f_name[64];
        if (header->files[i].name[0] != '/') {
            f_name[0] = '/';
            memory_copy(header->files[i].name, f_name + 1, strlen(header->files[i].name) + 1);
        } else {
            memory_copy(header->files[i].name, f_name, strlen(header->files[i].name) + 1);
        }
        
        if (strcmp(f_name, target_path) == 0) {
            vfs_node_t* child = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
            if (header->files[i].is_dir) {
                memory_copy(target_path, child->name, strlen(target_path) + 1);
            } else {
                memory_copy(name, child->name, strlen(name) + 1);
            }
            child->flags = header->files[i].is_dir ? VFS_DIRECTORY : VFS_FILE;
            child->inode = i;
            child->length = header->files[i].length;
            child->impl = i;
            
            child->read = initrd_vfs_read;
            child->write = initrd_vfs_write;
            child->readdir = initrd_vfs_readdir;
            child->finddir = initrd_vfs_finddir;
            child->create = 0;
            child->open = 0;
            child->close = 0;
            child->ioctl = 0;
            return child;
        }
    }
    return 0;
}

int initrd_vfs_create(vfs_node_t* parent, const char* name, unsigned int flags) {
    char target_path[128];
    int parent_len = strlen(parent->name);
    memory_copy(parent->name, target_path, parent_len);
    if (parent->name[parent_len - 1] != '/') {
        target_path[parent_len] = '/';
        memory_copy(name, target_path + parent_len + 1, strlen(name) + 1);
    } else {
        memory_copy(name, target_path + parent_len, strlen(name) + 1);
    }
    
    unsigned int is_dir = (flags & VFS_DIRECTORY) ? 1 : 0;
    return create_file(target_path, is_dir);
}

vfs_node_t* init_initrd_vfs() {
    memory_copy("/", initrd_root_node.name, 2);
    initrd_root_node.flags = VFS_DIRECTORY;
    initrd_root_node.inode = 0;
    initrd_root_node.length = 0;
    initrd_root_node.impl = 0;
    
    initrd_root_node.read = 0;
    initrd_root_node.write = 0;
    initrd_root_node.readdir = initrd_vfs_readdir;
    initrd_root_node.finddir = initrd_vfs_finddir;
    initrd_root_node.create = initrd_vfs_create;
    initrd_root_node.open = 0;
    initrd_root_node.close = 0;
    initrd_root_node.ioctl = 0;
    return &initrd_root_node;
}