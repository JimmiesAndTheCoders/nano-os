#include "cache.h"
#include "ata.h"
#include "timer.h"
#include "util.h"
#include "screen.h"

static block_cache_entry_t block_cache[BUFFER_CACHE_SIZE];
static page_cache_entry_t page_cache[PAGE_CACHE_SIZE];
static cache_stats_t cache_stats;

void init_cache() {
    memset(block_cache, 0, sizeof(block_cache));
    memset(page_cache, 0, sizeof(page_cache));
    memset(&cache_stats, 0, sizeof(cache_stats));
}

cache_stats_t cache_get_stats() {
    return cache_stats;
}

void cache_reset_stats() {
    memset(&cache_stats, 0, sizeof(cache_stats));
}

// ==============================================================================
// Buffer Cache (Block Cache) Implementation
// ==============================================================================

int buffer_cache_read(unsigned int lba, unsigned char* buffer) {
    int slot = -1;
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (block_cache[i].valid && block_cache[i].lba == lba) {
            slot = i;
            break;
        }
    }
    
    if (slot != -1) {
        block_cache[slot].last_accessed = timer_get_ticks();
        cache_stats.block_read_hits++;
        memory_copy((const char*)block_cache[slot].data, (char*)buffer, BLOCK_SIZE);
        return 0;
    }
    
    cache_stats.block_reads++;
    
    slot = -1;
    unsigned int oldest = 0xFFFFFFFF;
    int lru_slot = 0;
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (!block_cache[i].valid) {
            slot = i;
            break;
        }
        if (block_cache[i].last_accessed < oldest) {
            oldest = block_cache[i].last_accessed;
            lru_slot = i;
        }
    }
    
    if (slot == -1) {
        slot = lru_slot;
        if (block_cache[slot].dirty) {
            int res;
            if (ata_has_dma()) {
                res = ata_write_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[slot].lba, 1, (const unsigned short*)block_cache[slot].data);
            } else {
                res = ata_write_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[slot].lba, 1, (const unsigned short*)block_cache[slot].data);
            }
            if (res != 0) return res;
            block_cache[slot].dirty = 0;
        }
    }
    
    int res;
    if (ata_has_dma()) {
        res = ata_read_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, 1, (unsigned short*)block_cache[slot].data);
    } else {
        res = ata_read_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, lba, 1, (unsigned short*)block_cache[slot].data);
    }
    if (res != 0) return res;
    
    block_cache[slot].lba = lba;
    block_cache[slot].valid = 1;
    block_cache[slot].dirty = 0;
    block_cache[slot].last_accessed = timer_get_ticks();
    
    memory_copy((const char*)block_cache[slot].data, (char*)buffer, BLOCK_SIZE);
    return 0;
}

int buffer_cache_write(unsigned int lba, const unsigned char* buffer) {
    int slot = -1;
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (block_cache[i].valid && block_cache[i].lba == lba) {
            slot = i;
            break;
        }
    }
    
    if (slot != -1) {
        block_cache[slot].last_accessed = timer_get_ticks();
        cache_stats.block_write_hits++;
    } else {
        cache_stats.block_writes++;
        
        slot = -1;
        unsigned int oldest = 0xFFFFFFFF;
        int lru_slot = 0;
        for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
            if (!block_cache[i].valid) {
                slot = i;
                break;
            }
            if (block_cache[i].last_accessed < oldest) {
                oldest = block_cache[i].last_accessed;
                lru_slot = i;
            }
        }
        
        if (slot == -1) {
            slot = lru_slot;
            if (block_cache[slot].dirty) {
                int res;
                if (ata_has_dma()) {
                    res = ata_write_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[slot].lba, 1, (const unsigned short*)block_cache[slot].data);
                } else {
                    res = ata_write_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[slot].lba, 1, (const unsigned short*)block_cache[slot].data);
                }
                if (res != 0) return res;
                block_cache[slot].dirty = 0;
            }
        }
    }
    
    block_cache[slot].lba = lba;
    block_cache[slot].valid = 1;
    block_cache[slot].dirty = 1;
    block_cache[slot].last_accessed = timer_get_ticks();
    memory_copy((const char*)buffer, (char*)block_cache[slot].data, BLOCK_SIZE);
    
    return 0;
}

int buffer_cache_read_sectors(unsigned int lba, unsigned int count, unsigned char* buffer) {
    for (unsigned int i = 0; i < count; i++) {
        if (buffer_cache_read(lba + i, buffer + i * BLOCK_SIZE) != 0) {
            return -1;
        }
    }
    return 0;
}

int buffer_cache_write_sectors(unsigned int lba, unsigned int count, const unsigned char* buffer) {
    for (unsigned int i = 0; i < count; i++) {
        if (buffer_cache_write(lba + i, buffer + i * BLOCK_SIZE) != 0) {
            return -1;
        }
    }
    return 0;
}

void buffer_cache_flush() {
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (block_cache[i].valid && block_cache[i].dirty) {
            if (ata_has_dma()) {
                ata_write_dma(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[i].lba, 1, (const unsigned short*)block_cache[i].data);
            } else {
                ata_write_pio(ATA_PRIMARY_IO, ATA_DRIVE_MASTER, block_cache[i].lba, 1, (const unsigned short*)block_cache[i].data);
            }
            block_cache[i].dirty = 0;
        }
    }
}

// ==============================================================================
// Page Cache Implementation
// ==============================================================================

unsigned int page_cache_read(vfs_node_t* node, unsigned int offset, unsigned int size, unsigned char* buffer) {
    if (!node || !node->read) return 0;
    if (offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;
    
    unsigned int bytes_read = 0;
    
    while (bytes_read < size) {
        unsigned int cur_offset = offset + bytes_read;
        unsigned int page_index = cur_offset / PAGE_SIZE_4K;
        unsigned int page_offset = cur_offset % PAGE_SIZE_4K;
        unsigned int chunk_size = PAGE_SIZE_4K - page_offset;
        if (chunk_size > (size - bytes_read)) {
            chunk_size = size - bytes_read;
        }
        
        int page_slot = -1;
        for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
            if (page_cache[i].valid && page_cache[i].inode == node->inode && 
                page_cache[i].read_func == node->read && page_cache[i].page_index == page_index) {
                page_slot = i;
                break;
            }
        }
        
        if (page_slot != -1) {
            page_cache[page_slot].last_accessed = timer_get_ticks();
            cache_stats.page_read_hits++;
        } else {
            cache_stats.page_reads++;
            
            page_slot = -1;
            unsigned int oldest = 0xFFFFFFFF;
            int lru_slot = 0;
            for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
                if (!page_cache[i].valid) {
                    page_slot = i;
                    break;
                }
                if (page_cache[i].last_accessed < oldest) {
                    oldest = page_cache[i].last_accessed;
                    lru_slot = i;
                }
            }
            
            if (page_slot == -1) {
                page_slot = lru_slot;
                if (page_cache[page_slot].dirty && page_cache[page_slot].write_func) {
                    unsigned int write_size = PAGE_SIZE_4K;
                    unsigned int page_start = page_cache[page_slot].page_index * PAGE_SIZE_4K;
                    if (page_start + write_size > page_cache[page_slot].length) {
                        write_size = page_cache[page_slot].length - page_start;
                    }
                    
                    if (write_size > 0) {
                        vfs_node_t dummy;
                        memset(&dummy, 0, sizeof(dummy));
                        dummy.inode = page_cache[page_slot].inode;
                        dummy.impl = page_cache[page_slot].impl;
                        dummy.length = page_cache[page_slot].length;
                        dummy.write = page_cache[page_slot].write_func;
                        
                        dummy.write(&dummy, page_start, write_size, page_cache[page_slot].data);
                    }
                    page_cache[page_slot].dirty = 0;
                }
            }
            
            memset(page_cache[page_slot].data, 0, PAGE_SIZE_4K);
            page_cache[page_slot].inode = node->inode;
            page_cache[page_slot].page_index = page_index;
            page_cache[page_slot].valid = 1;
            page_cache[page_slot].dirty = 0;
            page_cache[page_slot].last_accessed = timer_get_ticks();
            page_cache[page_slot].length = node->length;
            page_cache[page_slot].read_func = node->read;
            page_cache[page_slot].write_func = node->write;
            page_cache[page_slot].impl = node->impl;
            
            unsigned int bytes_to_read_from_disk = PAGE_SIZE_4K;
            unsigned int page_start_offset = page_index * PAGE_SIZE_4K;
            if (page_start_offset + bytes_to_read_from_disk > node->length) {
                bytes_to_read_from_disk = node->length - page_start_offset;
            }
            
            if (bytes_to_read_from_disk > 0) {
                node->read(node, page_start_offset, bytes_to_read_from_disk, page_cache[page_slot].data);
            }
        }
        
        memory_copy((const char*)(page_cache[page_slot].data + page_offset), (char*)buffer + bytes_read, chunk_size);
        bytes_read += chunk_size;
    }
    
    return bytes_read;
}

unsigned int page_cache_write(vfs_node_t* node, unsigned int offset, unsigned int size, const unsigned char* buffer) {
    if (!node || !node->write) return 0;
    
    unsigned int bytes_written = 0;
    
    while (bytes_written < size) {
        unsigned int cur_offset = offset + bytes_written;
        unsigned int page_index = cur_offset / PAGE_SIZE_4K;
        unsigned int page_offset = cur_offset % PAGE_SIZE_4K;
        unsigned int chunk_size = PAGE_SIZE_4K - page_offset;
        if (chunk_size > (size - bytes_written)) {
            chunk_size = size - bytes_written;
        }
        
        int page_slot = -1;
        for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
            if (page_cache[i].valid && page_cache[i].inode == node->inode && 
                page_cache[i].read_func == node->read && page_cache[i].page_index == page_index) {
                page_slot = i;
                break;
            }
        }
        
        if (page_slot != -1) {
            page_cache[page_slot].last_accessed = timer_get_ticks();
            cache_stats.page_write_hits++;
        } else {
            cache_stats.page_writes++;
            
            page_slot = -1;
            unsigned int oldest = 0xFFFFFFFF;
            int lru_slot = 0;
            for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
                if (!page_cache[i].valid) {
                    page_slot = i;
                    break;
                }
                if (page_cache[i].last_accessed < oldest) {
                    oldest = page_cache[i].last_accessed;
                    lru_slot = i;
                }
            }
            
            if (page_slot == -1) {
                page_slot = lru_slot;
                if (page_cache[page_slot].dirty && page_cache[page_slot].write_func) {
                    unsigned int write_size = PAGE_SIZE_4K;
                    unsigned int page_start = page_cache[page_slot].page_index * PAGE_SIZE_4K;
                    if (page_start + write_size > page_cache[page_slot].length) {
                        write_size = page_cache[page_slot].length - page_start;
                    }
                    
                    if (write_size > 0) {
                        vfs_node_t dummy;
                        memset(&dummy, 0, sizeof(dummy));
                        dummy.inode = page_cache[page_slot].inode;
                        dummy.impl = page_cache[page_slot].impl;
                        dummy.length = page_cache[page_slot].length;
                        dummy.write = page_cache[page_slot].write_func;
                        
                        dummy.write(&dummy, page_start, write_size, page_cache[page_slot].data);
                    }
                    page_cache[page_slot].dirty = 0;
                }
            }
            
            memset(page_cache[page_slot].data, 0, PAGE_SIZE_4K);
            page_cache[page_slot].inode = node->inode;
            page_cache[page_slot].page_index = page_index;
            page_cache[page_slot].valid = 1;
            page_cache[page_slot].dirty = 0;
            page_cache[page_slot].last_accessed = timer_get_ticks();
            page_cache[page_slot].length = node->length;
            page_cache[page_slot].read_func = node->read;
            page_cache[page_slot].write_func = node->write;
            page_cache[page_slot].impl = node->impl;
            
            unsigned int page_start_offset = page_index * PAGE_SIZE_4K;
            if (page_start_offset < node->length && node->read) {
                unsigned int to_read = PAGE_SIZE_4K;
                if (page_start_offset + to_read > node->length) {
                    to_read = node->length - page_start_offset;
                }
                node->read(node, page_start_offset, to_read, page_cache[page_slot].data);
            }
        }
        
        memory_copy((const char*)buffer + bytes_written, (char*)page_cache[page_slot].data + page_offset, chunk_size);
        page_cache[page_slot].dirty = 1;
        page_cache[page_slot].last_accessed = timer_get_ticks();
        
        bytes_written += chunk_size;
        
        if (offset + bytes_written > node->length) {
            node->length = offset + bytes_written;
        }
        page_cache[page_slot].length = node->length;
    }
    
    return bytes_written;
}

void page_cache_flush_node(vfs_node_t* node) {
    if (!node) return;
    for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
        if (page_cache[i].valid && page_cache[i].inode == node->inode && page_cache[i].read_func == node->read) {
            if (page_cache[i].dirty && page_cache[i].write_func) {
                unsigned int write_size = PAGE_SIZE_4K;
                unsigned int page_start = page_cache[i].page_index * PAGE_SIZE_4K;
                if (page_start + write_size > page_cache[i].length) {
                    write_size = page_cache[i].length - page_start;
                }
                
                if (write_size > 0) {
                    vfs_node_t dummy;
                    memset(&dummy, 0, sizeof(dummy));
                    dummy.inode = page_cache[i].inode;
                    dummy.impl = page_cache[i].impl;
                    dummy.length = page_cache[i].length;
                    dummy.write = page_cache[i].write_func;
                    
                    dummy.write(&dummy, page_start, write_size, page_cache[i].data);
                }
                page_cache[i].dirty = 0;
            }
        }
    }
}

void page_cache_flush_all() {
    for (int i = 0; i < PAGE_CACHE_SIZE; i++) {
        if (page_cache[i].valid && page_cache[i].dirty && page_cache[i].write_func) {
            unsigned int write_size = PAGE_SIZE_4K;
            unsigned int page_start = page_cache[i].page_index * PAGE_SIZE_4K;
            if (page_start + write_size > page_cache[i].length) {
                write_size = page_cache[i].length - page_start;
            }
            
            if (write_size > 0) {
                vfs_node_t dummy;
                memset(&dummy, 0, sizeof(dummy));
                dummy.inode = page_cache[i].inode;
                dummy.impl = page_cache[i].impl;
                dummy.length = page_cache[i].length;
                dummy.write = page_cache[i].write_func;
                
                dummy.write(&dummy, page_start, write_size, page_cache[i].data);
            }
            page_cache[i].dirty = 0;
        }
    }
}