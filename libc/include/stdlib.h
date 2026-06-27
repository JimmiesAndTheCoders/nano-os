#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void exit(int status);

int atoi(const char *str);
char *itoa(int value, char *str, int base);
int abs(int j);

#endif