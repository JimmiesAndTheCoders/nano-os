#include "stdlib.h"
#include "unistd.h"
#include "string.h"

typedef struct block_header {
    size_t size;
    int free;
    struct block_header *next;
} block_header_t;

static block_header_t *heap_start = NULL;

void *malloc(size_t size) {
    if (size == 0) return NULL;

    // Align allocations to 4-byte boundaries
    size = (size + 3) & ~3;

    block_header_t *curr = heap_start;
    block_header_t *prev = NULL;

    // 1. Search the existing free list
    while (curr != NULL) {
        if (curr->free && curr->size >= size) {
            size_t remaining = curr->size - size;
            if (remaining > sizeof(block_header_t) + 4) {
                // Split the block if excess space is available
                block_header_t *next_block = (block_header_t *)((unsigned char *)curr + sizeof(block_header_t) + size);
                next_block->size = remaining - sizeof(block_header_t);
                next_block->free = 1;
                next_block->next = curr->next;
                curr->next = next_block;
                curr->size = size;
            }
            curr->free = 0;
            return (void *)((unsigned char *)curr + sizeof(block_header_t));
        }
        prev = curr;
        curr = curr->next;
    }

    // 2. No block was large enough. Request virtual heap expansion from the kernel
    size_t total_size = size + sizeof(block_header_t);
    block_header_t *new_block = (block_header_t *)sbrk(total_size);
    if (new_block == (void *)-1 || new_block == NULL) {
        return NULL; // System Out of Memory
    }

    new_block->size = size;
    new_block->free = 0;
    new_block->next = NULL;

    if (prev == NULL) {
        heap_start = new_block;
    } else {
        prev->next = new_block;
    }

    return (void *)((unsigned char *)new_block + sizeof(block_header_t));
}

void free(void *ptr) {
    if (ptr == NULL) return;

    block_header_t *header = (block_header_t *)((unsigned char *)ptr - sizeof(block_header_t));
    header->free = 1;

    // Coalesce adjacent free blocks
    block_header_t *curr = heap_start;
    while (curr != NULL && curr->next != NULL) {
        if (curr->free && curr->next->free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void exit(int status) {
    (void)status;
    while (1) {
        __asm__ __volatile__ ("nop");
    }
}