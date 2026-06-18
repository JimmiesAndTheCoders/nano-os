/* ==============================================================================
 * NANO OS - Physical Memory Manager Header
 * Description: Constants, macros, and function prototypes for the bitmap-based
 * physical frame allocator.
 * ============================================================================== */

#ifndef PMM_H
#define PMM_H

#define PAGE_SIZE       4096                    /* Each physical page frame is 4KB */
#define MEMORY_LIMIT    (64 * 1024 * 1024)       /* Hardcoded limit for 64MB VirtualBox setup */
#define TOTAL_BLOCKS    (MEMORY_LIMIT / PAGE_SIZE) /* 16,384 blocks */
#define BITMAP_SIZE     (TOTAL_BLOCKS / 32)      /* 512 integers needed to represent 16,384 blocks */

/* Core Physical Memory Allocator API */
void init_pmm(unsigned int start_free_mem);
void* pmm_alloc_block();
void pmm_free_block(void* addr);

/* Utility allocation boundaries */
void pmm_reserve_region(unsigned int start_addr, unsigned int size);
void pmm_free_region(unsigned int start_addr, unsigned int size);

/* Statistics debug counters */
unsigned int pmm_get_total_frames();
unsigned int pmm_get_free_frames();
unsigned int pmm_get_used_frames();

#endif