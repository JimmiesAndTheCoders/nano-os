#ifndef KMALLOC_H
#define KMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

void kmalloc_init(unsigned int heap_start, unsigned int heap_size);
void* kmalloc(unsigned int size);
void kfree(void* ptr);

#ifdef __cplusplus
}
#endif

#endif