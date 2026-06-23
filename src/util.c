#include "util.h"

/* Forward declaration for the internal helper */
static void reverse(char s[]);

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

/* Integer to String parser */
void itoa(int n, char str[]) {
    int i = 0;
    long num = n; // Use long to handle INT_MIN edge cases
    int sign = 0;
    
    if (num < 0) {
        sign = 1;
        num = -num;
    }
    
    do {
        str[i++] = (num % 10) + '0';
    } while ((num /= 10) > 0);
    
    if (sign) str[i++] = '-';
    str[i] = '\0';
    
    reverse(str); // This now has a prototype above!
}

/* Internal string reverser */
static void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}