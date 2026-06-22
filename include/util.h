#ifndef UTIL_H
#define UTIL_H

void memory_copy(const char *source, char *dest, int no_bytes);

/* String Utility Prototypes */
int strlen(const char *s);
void append(char s[], char n);
void backspace(char s[]);
int strcmp(const char *s1, const char *s2);

#endif