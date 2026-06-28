#ifndef UTIL_H
#define UTIL_H

void memory_copy(const char *source, char *dest, int no_bytes);
void memset(void *dest, unsigned char val, int count);
void *memcpy(void *dest, const void *src, unsigned int n);

/* String Utility Prototypes */
int strlen(const char *s);
void append(char s[], char n);
void backspace(char s[]);
int strcmp(const char *s1, const char *s2);
void itoa(int n, char str[]);

#endif