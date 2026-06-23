#include "fat32.h"
#include "ata.h"
#include "kmalloc.h"
#include "util.h"
#include "screen.h"

static unsigned int partition_start_lba = 0;
static unsigned int partition_sector_count = 0;
static fat32_bpb_t bpb;
static vfs_node_t fat32_root_node;
static struct dirent fat32_dirent;

static int strncmp_local(const char *s1, const char *s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (s1[i] == '\0') return 0;
    }
    return 0;
}

static int fat32_read_sectors(unsigned int lba, unsigned int count, unsigned char* buffer) {
    if (ata_has_dma()) {
        return ata_read_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, (unsigned short*)buffer);
    } else {
        return ata_read_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, (unsigned short*)buffer);
    }
}

static int fat32_write_sectors(unsigned int lba, unsigned int count, const unsigned char* buffer) {
    if (ata_has_dma()) {
        return ata_write_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, (const unsigned short*)buffer);
    } else {
        return ata_write_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, count, (const unsigned short*)buffer);
    }
}

static unsigned int fat_lba() {
    return partition_start_lba + bpb.reserved_sector_count;
}

static unsigned int data_lba() {
    return fat_lba() + (bpb.num_fats * bpb.fat_size_32);
}

static unsigned int cluster_to_lba(unsigned int cluster) {
    if (cluster < 2) return 0;
    return data_lba() + (cluster - 2) * bpb.sectors_per_cluster;
}

static unsigned int read_fat_entry(unsigned int cluster) {
    unsigned int sector = fat_lba() + (cluster * 4) / bpb.bytes_per_sector;
    unsigned int offset = (cluster * 4) % bpb.bytes_per_sector;
    unsigned char sector_buf[512];
    
    if (fat32_read_sectors(sector, 1, sector_buf) != 0) return 0x0FFFFFFF;
    return *(unsigned int*)(sector_buf + offset) & 0x0FFFFFFF;
}

static void write_fat_entry(unsigned int cluster, unsigned int value) {
    unsigned int sector = fat_lba() + (cluster * 4) / bpb.bytes_per_sector;
    unsigned int offset = (cluster * 4) % bpb.bytes_per_sector;
    unsigned char sector_buf[512];
    
    if (fat32_read_sectors(sector, 1, sector_buf) == 0) {
        unsigned int* entry_ptr = (unsigned int*)(sector_buf + offset);
        *entry_ptr = (*entry_ptr & 0xF0000000) | (value & 0x0FFFFFFF);
        fat32_write_sectors(sector, 1, sector_buf);
    }
}

static unsigned int fat32_allocate_cluster() {
    unsigned int total_clusters = bpb.total_sectors_32 / bpb.sectors_per_cluster;
    unsigned char sector_buf[512];
    unsigned int current_sector = 0xFFFFFFFF;
    
    for (unsigned int cluster = 2; cluster < total_clusters; cluster++) {
        unsigned int sector = fat_lba() + (cluster * 4) / bpb.bytes_per_sector;
        unsigned int offset = (cluster * 4) % bpb.bytes_per_sector;
        
        if (sector != current_sector) {
            if (fat32_read_sectors(sector, 1, sector_buf) != 0) continue;
            current_sector = sector;
        }
        
        unsigned int entry = *(unsigned int*)(sector_buf + offset) & 0x0FFFFFFF;
        if (entry == 0) {
            *(unsigned int*)(sector_buf + offset) = 0x0FFFFFFF; // EOC
            fat32_write_sectors(sector, 1, sector_buf);
            return cluster;
        }
    }
    return 0; // Disk Full
}

int init_fat32() {
    unsigned char sector_buf[512];
    if (fat32_read_sectors(0, 1, sector_buf) != 0) return -1;
    
    fat32_mbr_t* mbr = (fat32_mbr_t*)sector_buf;
    if (mbr->signature != 0xAA55) return -2;
    
    int found = 0;
    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].partition_type == 0x0B || mbr->partitions[i].partition_type == 0x0C) {
            partition_start_lba = mbr->partitions[i].start_lba;
            partition_sector_count = mbr->partitions[i].sector_count;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        fat32_bpb_t* temp_bpb = (fat32_bpb_t*)sector_buf;
        if (temp_bpb->boot_signature == 0x29 && strncmp_local(temp_bpb->fs_type, "FAT32   ", 8) == 0) {
            partition_start_lba = 0;
            partition_sector_count = temp_bpb->total_sectors_32;
            found = 1;
        }
    }
    
    if (!found) return -3;
    if (fat32_read_sectors(partition_start_lba, 1, (unsigned char*)&bpb) != 0) return -4;
    if (bpb.boot_signature != 0x29) return -5;
    
    return 0;
}

static void copy_utf16_to_ascii(const unsigned short* src, char* dest, int count) {
    for (int i = 0; i < count; i++) {
        unsigned short c = src[i];
        if (c == 0 || c == 0xFFFF) {
            dest[i] = '\0';
            return;
        }
        dest[i] = (char)(c & 0xFF);
    }
    dest[count] = '\0';
}

static void extract_lfn_chars(fat32_lfn_entry_t* lfn, unsigned short* unicode_buf) {
    unsigned char seq = lfn->order & ~0x40;
    if (seq == 0 || seq > 20) return;
    
    int index = (seq - 1) * 13;
    for (int i = 0; i < 5; i++)  unicode_buf[index + i] = lfn->name1[i];
    for (int i = 0; i < 6; i++)  unicode_buf[index + 5 + i] = lfn->name2[i];
    for (int i = 0; i < 2; i++)  unicode_buf[index + 11 + i] = lfn->name3[i];
}

static void format_short_name(const char* fat_name, char* dest) {
    int name_len = 0;
    for (int i = 0; i < 8; i++) {
        if (fat_name[i] != ' ') dest[name_len++] = fat_name[i];
    }
    int ext_len = 0;
    char ext[4];
    for (int i = 8; i < 11; i++) {
        if (fat_name[i] != ' ') ext[ext_len++] = fat_name[i];
    }
    if (ext_len > 0) {
        dest[name_len++] = '.';
        for (int i = 0; i < ext_len; i++) dest[name_len++] = ext[i];
    }
    dest[name_len] = '\0';
}

typedef int (*fat32_entry_callback_t)(fat32_dir_entry_t* entry, const char* name, void* arg);

static int fat32_iterate_dir(unsigned int first_cluster, fat32_entry_callback_t callback, void* arg) {
    unsigned int cluster = first_cluster;
    unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
    unsigned char* cluster_buf = (unsigned char*)kmalloc(bytes_per_cluster);
    
    unsigned short unicode_lfn[256];
    int has_lfn = 0;
    memset(unicode_lfn, 0, sizeof(unicode_lfn));
    
    while (cluster >= 2 && cluster < 0x0FFFFFFF) {
        unsigned int lba = cluster_to_lba(cluster);
        if (fat32_read_sectors(lba, bpb.sectors_per_cluster, cluster_buf) != 0) {
            kfree(cluster_buf);
            return -1;
        }
        
        unsigned int num_entries = bytes_per_cluster / sizeof(fat32_dir_entry_t);
        for (unsigned int i = 0; i < num_entries; i++) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(cluster_buf + i * sizeof(fat32_dir_entry_t));
            if (entry->name[0] == 0x00) {
                kfree(cluster_buf);
                return 0;
            }
            if (entry->name[0] == 0xE5) {
                has_lfn = 0;
                continue;
            }
            if (entry->attr == 0x0F) {
                extract_lfn_chars((fat32_lfn_entry_t*)entry, unicode_lfn);
                has_lfn = 1;
                continue;
            }
            
            char name_buf[256];
            if (has_lfn) {
                copy_utf16_to_ascii(unicode_lfn, name_buf, 255);
                has_lfn = 0;
                memset(unicode_lfn, 0, sizeof(unicode_lfn));
            } else {
                format_short_name(entry->name, name_buf);
            }
            
            if (callback(entry, name_buf, arg) != 0) {
                kfree(cluster_buf);
                return 1;
            }
        }
        cluster = read_fat_entry(cluster);
    }
    
    kfree(cluster_buf);
    return 0;
}

typedef struct {
    unsigned int target_index;
    unsigned int current_index;
    char name[256];
    unsigned int first_cluster;
    unsigned int file_size;
    unsigned char attr;
    int found;
} readdir_search_t;

static int readdir_callback(fat32_dir_entry_t* entry, const char* name, void* arg) {
    readdir_search_t* search = (readdir_search_t*)arg;
    if (entry->attr & 0x08) return 0; // Skip volume IDs
    
    if (search->current_index == search->target_index) {
        memory_copy(name, search->name, strlen(name) + 1);
        search->first_cluster = entry->first_cluster_low | ((unsigned int)entry->first_cluster_high << 16);
        search->file_size = entry->file_size;
        search->attr = entry->attr;
        search->found = 1;
        return 1;
    }
    search->current_index++;
    return 0;
}

struct dirent* fat32_vfs_readdir(vfs_node_t* node, unsigned int index) {
    readdir_search_t search;
    search.target_index = index;
    search.current_index = 0;
    search.found = 0;
    
    fat32_iterate_dir(node->impl, readdir_callback, &search);
    if (search.found) {
        memory_copy(search.name, fat32_dirent.name, strlen(search.name) + 1);
        fat32_dirent.inode = search.first_cluster;
        return &fat32_dirent;
    }
    return 0;
}

typedef struct {
    const char* target_name;
    char name[256];
    unsigned int first_cluster;
    unsigned int file_size;
    unsigned char attr;
    int found;
} finddir_search_t;

static int finddir_callback(fat32_dir_entry_t* entry, const char* name, void* arg) {
    finddir_search_t* search = (finddir_search_t*)arg;
    if (strcmp(name, search->target_name) == 0) {
        memory_copy(name, search->name, strlen(name) + 1);
        search->first_cluster = entry->first_cluster_low | ((unsigned int)entry->first_cluster_high << 16);
        search->file_size = entry->file_size;
        search->attr = entry->attr;
        search->found = 1;
        return 1;
    }
    return 0;
}

unsigned int fat32_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer);
unsigned int fat32_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer);
int fat32_vfs_create(vfs_node_t* parent, const char* name, unsigned int flags);

vfs_node_t* fat32_vfs_finddir(vfs_node_t* node, const char* name) {
    finddir_search_t search;
    search.target_name = name;
    search.found = 0;
    
    fat32_iterate_dir(node->impl, finddir_callback, &search);
    if (search.found) {
        vfs_node_t* child = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
        memory_copy(search.name, child->name, strlen(search.name) + 1);
        child->flags = (search.attr & 0x10) ? VFS_DIRECTORY : VFS_FILE;
        child->inode = search.first_cluster;
        child->length = search.file_size;
        child->impl = search.first_cluster;
        child->gid = node->impl; // Keep track of parent cluster for metadata changes
        
        child->read = fat32_vfs_read;
        child->write = fat32_vfs_write;
        child->readdir = fat32_vfs_readdir;
        child->finddir = fat32_vfs_finddir;
        child->create = fat32_vfs_create;
        child->open = 0;
        child->close = 0;
        child->ioctl = 0;
        return child;
    }
    return 0;
}

unsigned int fat32_vfs_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    if (offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    
    unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
    unsigned int skip_clusters = offset / bytes_per_cluster;
    unsigned int cluster_offset = offset % bytes_per_cluster;
    unsigned int cluster = node->impl;
    
    for (unsigned int i = 0; i < skip_clusters; i++) {
        cluster = read_fat_entry(cluster);
        if (cluster < 2 || cluster >= 0x0FFFFFFF) return 0;
    }
    
    unsigned int bytes_read = 0;
    unsigned char* cluster_buf = (unsigned char*)kmalloc(bytes_per_cluster);
    
    while (cluster >= 2 && cluster < 0x0FFFFFFF && bytes_read < size) {
        unsigned int lba = cluster_to_lba(cluster);
        if (fat32_read_sectors(lba, bpb.sectors_per_cluster, cluster_buf) != 0) {
            kfree(cluster_buf);
            return bytes_read;
        }
        
        unsigned int to_copy = size - bytes_read;
        unsigned int available = bytes_per_cluster - cluster_offset;
        if (to_copy > available) to_copy = available;
        
        memory_copy((const char*)(cluster_buf + cluster_offset), (char*)buffer + bytes_read, to_copy);
        bytes_read += to_copy;
        cluster_offset = 0;
        cluster = read_fat_entry(cluster);
    }
    
    kfree(cluster_buf);
    return bytes_read;
}

static void fat32_update_file_size(unsigned int parent_cluster, unsigned int file_cluster, unsigned int new_size) {
    unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
    unsigned char* cluster_buf = (unsigned char*)kmalloc(bytes_per_cluster);
    unsigned int cluster = parent_cluster;
    
    while (cluster >= 2 && cluster < 0x0FFFFFFF) {
        unsigned int lba = cluster_to_lba(cluster);
        if (fat32_read_sectors(lba, bpb.sectors_per_cluster, cluster_buf) == 0) {
            unsigned int num_entries = bytes_per_cluster / sizeof(fat32_dir_entry_t);
            for (unsigned int i = 0; i < num_entries; i++) {
                fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(cluster_buf + i * sizeof(fat32_dir_entry_t));
                if (entry->name[0] == 0) break;
                if (entry->name[0] == 0xE5) continue;
                if (entry->attr == 0x0F) continue;
                
                unsigned int entry_cluster = entry->first_cluster_low | ((unsigned int)entry->first_cluster_high << 16);
                if (entry_cluster == file_cluster) {
                    entry->file_size = new_size;
                    fat32_write_sectors(lba, bpb.sectors_per_cluster, cluster_buf);
                    kfree(cluster_buf);
                    return;
                }
            }
        }
        cluster = read_fat_entry(cluster);
    }
    kfree(cluster_buf);
}

unsigned int fat32_vfs_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
    unsigned int cluster = node->impl;
    
    if (cluster == 0) {
        cluster = fat32_allocate_cluster();
        if (cluster == 0) return 0;
        node->impl = cluster;
        node->inode = cluster;
    }
    
    unsigned int skip_clusters = offset / bytes_per_cluster;
    unsigned int cluster_offset = offset % bytes_per_cluster;
    
    for (unsigned int i = 0; i < skip_clusters; i++) {
        unsigned int next = read_fat_entry(cluster);
        if (next < 2 || next >= 0x0FFFFFFF) {
            next = fat32_allocate_cluster();
            if (next == 0) return 0;
            write_fat_entry(cluster, next);
        }
        cluster = next;
    }
    
    unsigned int bytes_written = 0;
    unsigned char* cluster_buf = (unsigned char*)kmalloc(bytes_per_cluster);
    
    while (bytes_written < size) {
        unsigned int lba = cluster_to_lba(cluster);
        if (cluster_offset > 0 || (size - bytes_written) < bytes_per_cluster) {
            fat32_read_sectors(lba, bpb.sectors_per_cluster, cluster_buf);
        }
        
        unsigned int to_write = size - bytes_written;
        unsigned int available = bytes_per_cluster - cluster_offset;
        if (to_write > available) to_write = available;
        
        memory_copy((const char*)buffer + bytes_written, (char*)cluster_buf + cluster_offset, to_write);
        if (fat32_write_sectors(lba, bpb.sectors_per_cluster, cluster_buf) != 0) {
            kfree(cluster_buf);
            return bytes_written;
        }
        
        bytes_written += to_write;
        cluster_offset = 0;
        
        if (bytes_written < size) {
            unsigned int next = read_fat_entry(cluster);
            if (next < 2 || next >= 0x0FFFFFFF) {
                next = fat32_allocate_cluster();
                if (next == 0) break;
                write_fat_entry(cluster, next);
            }
            cluster = next;
        }
    }
    
    kfree(cluster_buf);
    
    if (offset + size > node->length) {
        node->length = offset + size;
        fat32_update_file_size(node->gid, node->impl, node->length);
    }
    return bytes_written;
}

static int fat32_add_dir_entry(unsigned int dir_cluster, const char* name, unsigned char attr, unsigned int first_cluster) {
    unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
    unsigned char* cluster_buf = (unsigned char*)kmalloc(bytes_per_cluster);
    unsigned int cluster = dir_cluster;
    
    while (cluster >= 2 && cluster < 0x0FFFFFFF) {
        unsigned int lba = cluster_to_lba(cluster);
        if (fat32_read_sectors(lba, bpb.sectors_per_cluster, cluster_buf) != 0) {
            kfree(cluster_buf);
            return 0;
        }
        
        unsigned int num_entries = bytes_per_cluster / sizeof(fat32_dir_entry_t);
        for (unsigned int i = 0; i < num_entries; i++) {
            fat32_dir_entry_t* entry = (fat32_dir_entry_t*)(cluster_buf + i * sizeof(fat32_dir_entry_t));
            if (entry->name[0] == 0x00 || entry->name[0] == 0xE5) {
                memset(entry, 0, sizeof(fat32_dir_entry_t));
                
                char short_name[11];
                memset(short_name, ' ', 11);
                int name_len = strlen(name);
                int dot_pos = -1;
                for (int k = 0; k < name_len; k++) {
                    if (name[k] == '.') { dot_pos = k; break; }
                }
                
                if (dot_pos != -1) {
                    int to_copy = dot_pos > 8 ? 8 : dot_pos;
                    for (int k = 0; k < to_copy; k++) short_name[k] = name[k];
                    int ext_len = name_len - dot_pos - 1;
                    if (ext_len > 3) ext_len = 3;
                    for (int k = 0; k < ext_len; k++) short_name[8 + k] = name[dot_pos + 1 + k];
                } else {
                    int to_copy = name_len > 8 ? 8 : name_len;
                    for (int k = 0; k < to_copy; k++) short_name[k] = name[k];
                }
                
                for (int k = 0; k < 11; k++) {
                    if (short_name[k] >= 'a' && short_name[k] <= 'z') short_name[k] -= 32;
                }
                
                memory_copy(short_name, entry->name, 11);
                entry->attr = attr;
                entry->first_cluster_low = first_cluster & 0xFFFF;
                entry->first_cluster_high = (first_cluster >> 16) & 0xFFFF;
                entry->file_size = 0;
                
                fat32_write_sectors(lba, bpb.sectors_per_cluster, cluster_buf);
                kfree(cluster_buf);
                return 1;
            }
        }
        
        unsigned int next = read_fat_entry(cluster);
        if (next < 2 || next >= 0x0FFFFFFF) {
            next = fat32_allocate_cluster();
            if (next == 0) break;
            write_fat_entry(cluster, next);
            memset(cluster_buf, 0, bytes_per_cluster);
            fat32_write_sectors(cluster_to_lba(next), bpb.sectors_per_cluster, cluster_buf);
        }
        cluster = next;
    }
    
    kfree(cluster_buf);
    return 0;
}

int fat32_vfs_create(vfs_node_t* parent, const char* name, unsigned int flags) {
    unsigned int new_cluster = fat32_allocate_cluster();
    if (new_cluster == 0) return 0;
    
    unsigned char attr = (flags & VFS_DIRECTORY) ? 0x10 : 0x00;
    if (flags & VFS_DIRECTORY) {
        unsigned int bytes_per_cluster = bpb.sectors_per_cluster * bpb.bytes_per_sector;
        unsigned char* zero_buf = (unsigned char*)kmalloc(bytes_per_cluster);
        memset(zero_buf, 0, bytes_per_cluster);
        fat32_write_sectors(cluster_to_lba(new_cluster), bpb.sectors_per_cluster, zero_buf);
        kfree(zero_buf);
    }
    
    return fat32_add_dir_entry(parent->impl, name, attr, new_cluster);
}

vfs_node_t* fat32_get_vfs_root() {
    memory_copy("fat32", fat32_root_node.name, 6);
    fat32_root_node.flags = VFS_DIRECTORY;
    fat32_root_node.inode = bpb.root_cluster;
    fat32_root_node.length = 0;
    fat32_root_node.impl = bpb.root_cluster;
    
    fat32_root_node.read = 0;
    fat32_root_node.write = 0;
    fat32_root_node.readdir = fat32_vfs_readdir;
    fat32_root_node.finddir = fat32_vfs_finddir;
    fat32_root_node.create = fat32_vfs_create;
    fat32_root_node.open = 0;
    fat32_root_node.close = 0;
    fat32_root_node.ioctl = 0;
    return &fat32_root_node;
}