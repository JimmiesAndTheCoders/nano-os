#ifndef FAT32_H
#define FAT32_H

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char bootable;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned int start_lba;
    unsigned int sector_count;
} __attribute__((packed)) fat32_mbr_entry_t;

typedef struct {
    unsigned char bootstrap[446];
    fat32_mbr_entry_t partitions[4];
    unsigned short signature;
} __attribute__((packed)) fat32_mbr_t;

typedef struct {
    unsigned char jmp[3];
    char oem_name[8];
    unsigned short bytes_per_sector;
    unsigned char sectors_per_cluster;
    unsigned short reserved_sector_count;
    unsigned char num_fats;
    unsigned short root_entry_count;
    unsigned short total_sectors_16;
    unsigned char media;
    unsigned short fat_size_16;
    unsigned short sectors_per_track;
    unsigned short num_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_32;
    
    // FAT32 Extended BIOS Parameter Block (BPB)
    unsigned int fat_size_32;
    unsigned short ext_flags;
    unsigned short fs_version;
    unsigned int root_cluster;
    unsigned short fs_info;
    unsigned short backup_boot_sector;
    unsigned char reserved[12];
    unsigned char drive_number;
    unsigned char reserved1;
    unsigned char boot_signature;
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    char name[11];
    unsigned char attr;
    unsigned char ntres;
    unsigned char creation_time_tenths;
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short last_access_date;
    unsigned short first_cluster_high;
    unsigned short write_time;
    unsigned short write_date;
    unsigned short first_cluster_low;
    unsigned int file_size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    unsigned char order;
    unsigned short name1[5];
    unsigned char attr; // Expected 0x0F
    unsigned char type; // Expected 0
    unsigned char checksum;
    unsigned short name2[6];
    unsigned short first_cluster; // Expected 0
    unsigned short name3[2];
} __attribute__((packed)) fat32_lfn_entry_t;

// API functions
int init_fat32();
vfs_node_t* fat32_get_vfs_root();

#ifdef __cplusplus
}
#endif

#endif