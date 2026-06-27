#include <stddef.h>
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

typedef struct block_header {
    size_t size;
    int free;
    struct block_header *next;
} block_header_t;

static block_header_t *heap_start = NULL;

// --- Memory Management Core Allocator ---

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

    // 2. Request virtual heap expansion from the kernel via sbrk
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

void *calloc(size_t num, size_t size) {
    size_t total = num * size;
    
    // Multiplicative overflow check
    if (num != 0 && total / num != size) {
        return NULL;
    }

    void *ptr = malloc(total);
    if (ptr != NULL) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    block_header_t *header = (block_header_t *)((unsigned char *)ptr - sizeof(block_header_t));
    
    // If the block is already of sufficient size, return the original pointer
    if (header->size >= size) {
        return ptr;
    }

    // Allocate a new segment
    void *new_ptr = malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // Copy original data and release the old allocation
    size_t copy_size = header->size;
    if (copy_size > size) {
        copy_size = size;
    }
    memcpy(new_ptr, ptr, copy_size);
    free(ptr);

    return new_ptr;
}

void exit(int status) {
    (void)status;
    while (1) {
        __asm__ __volatile__ ("nop");
    }
}

// --- Utility Conversion functions ---

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    size_t i = 0;

    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' ||
           str[i] == '\v' || str[i] == '\f' || str[i] == '\r') {
        i++;
    }

    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + (str[i] - '0');
        i++;
    }

    return sign * res;
}

static void reverse_str(char *str, size_t len) {
    size_t start = 0;
    size_t end = len - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

char *itoa(int value, char *str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    size_t i = 0;
    int is_negative = 0;
    unsigned int num = value;

    if (value < 0 && base == 10) {
        is_negative = 1;
        num = -value;
    }

    do {
        int rem = num % base;
        str[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
    } while ((num /= base) > 0);

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';
    reverse_str(str, i);
    return str;
}

int abs(int j) {
    return (j < 0) ? -j : j;
}