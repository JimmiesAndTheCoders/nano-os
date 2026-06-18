#ifndef KMALLOC_H
#define KMALLOC_H

void kmalloc_init(unsigned int heap_start, unsigned int heap_size);
void* kmalloc(unsigned int size);
void kfree(void* ptr);

#endif
