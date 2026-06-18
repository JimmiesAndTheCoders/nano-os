#include "kmalloc.h"
#include "pmm.h"

#define KALLOC_ALIGN 4

typedef struct kblock_header {
    unsigned int size;
    unsigned int free;
    struct kblock_header* next;
} kblock_header_t;

static kblock_header_t* free_list = 0;

static unsigned int align_up(unsigned int size) {
    return (size + (KALLOC_ALIGN - 1)) & ~(KALLOC_ALIGN - 1);
}

void kmalloc_init(unsigned int heap_start, unsigned int heap_size) {
    if (heap_size <= sizeof(kblock_header_t)) {
        free_list = 0;
        return;
    }

    pmm_reserve_region(heap_start, heap_size);

    free_list = (kblock_header_t*) heap_start;
    free_list->size = heap_size - sizeof(kblock_header_t);
    free_list->free = 1;
    free_list->next = 0;
}

void* kmalloc(unsigned int size) {
    if (size == 0 || free_list == 0) {
        return 0;
    }

    size = align_up(size);
    kblock_header_t* current = free_list;

    while (current) {
        if (current->free && current->size >= size) {
            unsigned int remaining = current->size - size;
            if (remaining > sizeof(kblock_header_t) + 4) {
                kblock_header_t* next = (kblock_header_t*) ((unsigned char*) current + sizeof(kblock_header_t) + size);
                next->size = remaining - sizeof(kblock_header_t);
                next->free = 1;
                next->next = current->next;
                current->next = next;
                current->size = size;
            }
            current->free = 0;
            return (void*) ((unsigned char*) current + sizeof(kblock_header_t));
        }
        current = current->next;
    }

    return 0;
}

void kfree(void* ptr) {
    if (ptr == 0) {
        return;
    }

    kblock_header_t* header = (kblock_header_t*) ((unsigned char*) ptr - sizeof(kblock_header_t));
    header->free = 1;

    kblock_header_t* current = free_list;
    while (current && current->next) {
        if (current->free && current->next->free) {
            current->size += sizeof(kblock_header_t) + current->next->size;
            current->next = current->next->next;
            continue;
        }
        current = current->next;
    }
}
