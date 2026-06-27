#include "stdio.h"
#include "unistd.h"
#include <stdarg.h>

/* Define and map standard stream descriptors */
static FILE _stdin  = { 0 };
static FILE _stdout = { 1 };
static FILE _stderr = { 2 };

FILE *stdin  = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* Internal integer-to-string format helper */
static void itoa_unsigned(unsigned int val, char *buf, int base, int uppercase) {
    int i = 0;
    do {
        int rem = val % base;
        if (rem < 10) {
            buf[i++] = rem + '0';
        } else {
            buf[i++] = rem - 10 + (uppercase ? 'A' : 'a');
        }
    } while ((val /= base) > 0);
    buf[i] = '\0';
    
    /* Reverse the digits in place */
    for (int j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - 1 - j];
        buf[i - 1 - j] = temp;
    }
}

static void itoa_signed(int val, char *buf, int base) {
    if (val < 0 && base == 10) {
        *buf++ = '-';
        itoa_unsigned((unsigned int)(-val), buf, base, 0);
    } else {
        itoa_unsigned((unsigned int)val, buf, base, 0);
    }
}

/* Core string formatter */
int vsprintf(char *str, const char *format, va_list ap) {
    char *ptr = str;
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'c': {
                    *ptr++ = (char)va_arg(ap, int);
                    break;
                }
                case 's': {
                    const char *s = va_arg(ap, const char *);
                    if (!s) s = "(null)";
                    while (*s) {
                        *ptr++ = *s++;
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    int val = va_arg(ap, int);
                    char buf[32];
                    itoa_signed(val, buf, 10);
                    char *b = buf;
                    while (*b) {
                        *ptr++ = *b++;
                    }
                    break;
                }
                case 'u': {
                    unsigned int val = va_arg(ap, unsigned int);
                    char buf[32];
                    itoa_unsigned(val, buf, 10, 0);
                    char *b = buf;
                    while (*b) {
                        *ptr++ = *b++;
                    }
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(ap, unsigned int);
                    char buf[32];
                    itoa_unsigned(val, buf, 16, 0);
                    char *b = buf;
                    while (*b) {
                        *ptr++ = *b++;
                    }
                    break;
                }
                case 'X': {
                    unsigned int val = va_arg(ap, unsigned int);
                    char buf[32];
                    itoa_unsigned(val, buf, 16, 1);
                    char *b = buf;
                    while (*b) {
                        *ptr++ = *b++;
                    }
                    break;
                }
                case '%': {
                    *ptr++ = '%';
                    break;
                }
                default:
                    *ptr++ = format[i];
                    break;
            }
        } else {
            *ptr++ = format[i];
        }
    }
    *ptr = '\0';
    return (int)(ptr - str);
}

/* Standard stream format writers */
int vfprintf(FILE *stream, const char *format, va_list ap) {
    char buf[2048];
    int len = vsprintf(buf, format, ap);
    if (len > 0) {
        write(stream->fd, buf, len);
    }
    return len;
}

int fprintf(FILE *stream, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int len = vfprintf(stream, format, ap);
    va_end(ap);
    return len;
}

int printf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int len = vfprintf(stdout, format, ap);
    va_end(ap);
    return len;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int len = vsprintf(str, format, ap);
    va_end(ap);
    return len;
}

/* Unformatted Stream Methods */
int fputc(int c, FILE *stream) {
    char ch = (char)c;
    if (write(stream->fd, &ch, 1) == 1) {
        return c;
    }
    return EOF;
}

int fputs(const char *s, FILE *stream) {
    size_t len = 0;
    while (s[len] != '\0') len++;
    if (write(stream->fd, s, len) == (int)len) {
        return 0;
    }
    return EOF;
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char *s) {
    if (fputs(s, stdout) == 0 && fputc('\n', stdout) != EOF) {
        return 0;
    }
    return EOF;
}

int fgetc(FILE *stream) {
    char c;
    if (read(stream->fd, &c, 1) == 1) {
        return (unsigned char)c;
    }
    return EOF;
}

int getchar(void) {
    return fgetc(stdin);
}