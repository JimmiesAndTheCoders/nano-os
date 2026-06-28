/* ==============================================================================
 * NANO OS - Physical Memory Manager Header
 * Description: Constants, macros, and function prototypes for the bitmap-based
 * physical frame allocator.
 * ============================================================================== */

#ifndef PMM_H
#define PMM_H

#ifndef PAGE_SIZE
#define PAGE_SIZE       4096
#endif

#ifndef MEMORY_LIMIT
#define MEMORY_LIMIT    (64 * 1024 * 1024)
#endif

#define TOTAL_BLOCKS    (MEMORY_LIMIT / PAGE_SIZE)
#define BITMAP_SIZE     (TOTAL_BLOCKS / 32)

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