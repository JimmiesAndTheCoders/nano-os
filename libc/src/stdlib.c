#include "stdlib.h"
#include "string.h"

#define USER_HEAP_SIZE (64 * 1024) // 64 KB user space heap
static unsigned char user_heap[USER_HEAP_SIZE];

typedef struct block_header {
    size_t size;
    int free;
    struct block_header *next;
} block_header_t;

static block_header_t *heap_start = NULL;

static void init_user_heap() {
    heap_start = (block_header_t *)user_heap;
    heap_start->size = USER_HEAP_SIZE - sizeof(block_header_t);
    heap_start->free = 1;
    heap_start->next = NULL;
}

void *malloc(size_t size) {
    if (heap_start == NULL) {
        init_user_heap();
    }

    // Align to 4 bytes
    size = (size + 3) & ~3;

    block_header_t *curr = heap_start;
    while (curr != NULL) {
        if (curr->free && curr->size >= size) {
            size_t remaining = curr->size - size;
            if (remaining > sizeof(block_header_t) + 4) {
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
        curr = curr->next;
    }
    return NULL;
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
    // For now, spin inside an infinite loop to halt task execution
    while (1) {
        __asm__ __volatile__ ("nop");
    }
}