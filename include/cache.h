#ifndef CACHE_H
#define CACHE_H

#include "vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE         512
#define BUFFER_CACHE_SIZE  128  // 128 blocks * 512 bytes = 64 KB block cache

#define PAGE_SIZE_4K       4096
#define PAGE_CACHE_SIZE    32   // 32 pages * 4 KB = 128 KB page cache

typedef struct {
    unsigned int lba;
    unsigned char data[BLOCK_SIZE];
    int valid;
    int dirty;
    unsigned int last_accessed;
} block_cache_entry_t;

typedef struct {
    unsigned int inode;
    unsigned int page_index;
    unsigned char data[PAGE_SIZE_4K];
    int valid;
    int dirty;
    unsigned int last_accessed;
    unsigned int length;
    
    // Writeback and filesystem identity
    read_type_t read_func;
    write_type_t write_func;
    unsigned int impl;
} page_cache_entry_t;

typedef struct {
    unsigned int block_reads;
    unsigned int block_read_hits;
    unsigned int block_writes;
    unsigned int block_write_hits;
    unsigned int page_reads;
    unsigned int page_read_hits;
    unsigned int page_writes;
    unsigned int page_write_hits;
} cache_stats_t;

void init_cache();
cache_stats_t cache_get_stats();
void cache_reset_stats();

// Buffer Cache API
int buffer_cache_read(unsigned int lba, unsigned char* buffer);
int buffer_cache_write(unsigned int lba, const unsigned char* buffer);
int buffer_cache_read_sectors(unsigned int lba, unsigned int count, unsigned char* buffer);
int buffer_cache_write_sectors(unsigned int lba, unsigned int count, const unsigned char* buffer);
void buffer_cache_flush();

// Page Cache API
unsigned int page_cache_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer);
unsigned int page_cache_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer);
void page_cache_flush_node(vfs_node_t* node);
void page_cache_flush_all();

#ifdef __cplusplus
}
#endif

#endif