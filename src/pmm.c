/* ==============================================================================
 * NANO OS - Physical Memory Manager
 * Description: A robust bitmap frame allocator that maps physical blocks.
 * Keeps track of used/free blocks of 4KB in length.
 * ============================================================================== */

#include "pmm.h"
#include "screen.h"

/* Static allocation bitmap: 512 integers of 32 bits each = 16,384 bits (blocks) */
static unsigned int pmm_bitmap[BITMAP_SIZE];
static unsigned int total_blocks = TOTAL_BLOCKS;
static unsigned int used_blocks  = TOTAL_BLOCKS; /* Default to fully reserved */

/* Helper macros for bitwise manipulation */
#define INDEX_FROM_BIT(b) ((b) / 32)
#define OFFSET_FROM_BIT(b) ((b) % 32)

/* Internal Bitmap helpers */
static void set_bit(int bit) {
    int idx = INDEX_FROM_BIT(bit);
    int off = OFFSET_FROM_BIT(bit);
    pmm_bitmap[idx] |= (1 << off);
}

static void clear_bit(int bit) {
    int idx = INDEX_FROM_BIT(bit);
    int off = OFFSET_FROM_BIT(bit);
    pmm_bitmap[idx] &= ~(1 << off);
}

static int test_bit(int bit) {
    int idx = INDEX_FROM_BIT(bit);
    int off = OFFSET_FROM_BIT(bit);
    return (pmm_bitmap[idx] & (1 << off)) != 0;
}

/* Finds the first free bit (0) in the bitmap array */
static int first_free_bit() {
    int i, j;
    for (i = 0; i < BITMAP_SIZE; i++) {
        /* Skip checking individual bits if all 32 blocks in this chunk are allocated */
        if (pmm_bitmap[i] == 0xFFFFFFFF) continue;

        for (j = 0; j < 32; j++) {
            int bit = 1 << j;
            if (!(pmm_bitmap[i] & bit)) {
                return (i * 32) + j;
            }
        }
    }
    return -1; /* Out of physical memory! */
}

/* Initializes the PMM by reserving all memory, then freeing regions after the kernel space */
void init_pmm(unsigned int start_free_mem) {
    int i;
    /* Start by setting all bits to 1 (mark all blocks as completely reserved/used) */
    for (i = 0; i < BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }
    used_blocks = TOTAL_BLOCKS;

    /* Free the usable physical memory space starting at the end of our kernel up to 64MB */
    unsigned int free_space_size = MEMORY_LIMIT - start_free_mem;
    pmm_free_region(start_free_mem, free_space_size);

    /* Explicitly reserve early low-level system memory regions:
     * - 0x00000000 to 0x00001000 (IVT and BIOS structures)
     * - 0x00001000 to 0x00010000 (Kernel code execution space, loaded at 0x1000)
     * - 0x00090000 to 0x000A0000 (Protected Mode stack and BIOS configuration structures)
     * - 0x000A0000 to 0x00100000 (Video memory, hardware mapping, BIOS ROM)
     */
    pmm_reserve_region(0x0, 0x100000);
}

/* Reserve a specific sequence of bytes (maps to multiple bits) */
void pmm_reserve_region(unsigned int start_addr, unsigned int size) {
    unsigned int align_start = start_addr / PAGE_SIZE;
    unsigned int num_blocks  = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    unsigned int i;

    for (i = 0; i < num_blocks; i++) {
        unsigned int block = align_start + i;
        if (block < total_blocks) {
            if (!test_bit(block)) {
                set_bit(block);
                used_blocks++;
            }
        }
    }
}

/* Frees a specific sequence of bytes (marks them available) */
void pmm_free_region(unsigned int start_addr, unsigned int size) {
    unsigned int align_start = start_addr / PAGE_SIZE;
    unsigned int num_blocks  = size / PAGE_SIZE;
    unsigned int i;

    for (i = 0; i < num_blocks; i++) {
        unsigned int block = align_start + i;
        if (block < total_blocks) {
            if (test_bit(block)) {
                clear_bit(block);
                used_blocks--;
            }
        }
    }
}

/* Allocates a single physical page frame (4KB) and returns its memory address */
void* pmm_alloc_block() {
    int free_bit = first_free_bit();
    if (free_bit == -1) {
        return (void*)0; /* Out of physical memory */
    }

    set_bit(free_bit);
    used_blocks++;
    
    unsigned int physical_addr = free_bit * PAGE_SIZE;
    return (void*)physical_addr;
}

/* Frees a previously allocated physical frame back to the system pool */
void pmm_free_block(void* addr) {
    unsigned int physical_addr = (unsigned int)addr;
    int block = physical_addr / PAGE_SIZE;

    if (block < total_blocks) {
        if (test_bit(block)) {
            clear_bit(block);
            used_blocks--;
        }
    }
}

unsigned int pmm_get_total_frames() { return total_blocks; }
unsigned int pmm_get_free_frames()  { return total_blocks - used_blocks; }
unsigned int pmm_get_used_frames()  { return used_blocks; }