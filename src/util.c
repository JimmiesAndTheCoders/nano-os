#include "util.h"

void memory_copy(const char *source, char *dest, int no_bytes) {
    for (int i = 0; i < no_bytes; i++) {
        dest[i] = source[i];
    }
}

/* Returns the length of a string */
int strlen(const char *s) {
    int i = 0;
    while (s[i] != '\0') {
        ++i;
    }
    return i;
}

/* Appends a character to a string */
void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

/* Removes the last character from a string */
void backspace(char s[]) {
    int len = strlen(s);
    if (len > 0) {
        s[len-1] = '\0';
    }
}

/* Compares two strings. Returns 0 if they are identical */
int strcmp(const char *s1, const char *s2) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}