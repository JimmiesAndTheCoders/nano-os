#include "stdio.h"
#include "sys/syscall.h"
#include <stdarg.h>

void putchar(char c) {
    char buf[2] = {c, '\0'};
    __syscall1(SYS_PRINT, (int)buf);
}

void puts(const char *s) {
    __syscall1(SYS_PRINT, (int)s);
    __syscall1(SYS_PRINT, (int)"\n");
}

static void print_int(int n, int base) {
    char buf[32];
    int i = 0;
    unsigned int num = (n < 0 && base == 10) ? -n : n;

    if (n < 0 && base == 10) {
        putchar('-');
    }

    do {
        int rem = num % base;
        buf[i++] = (rem < 10) ? (rem + '0') : (rem - 10 + 'A');
    } while ((num /= base) > 0);

    while (i > 0) {
        putchar(buf[--i]);
    }
}

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = 0;

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (!s) s = "(null)";
                    while (*s) {
                        putchar(*s++);
                        count++;
                    }
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    print_int(val, 10);
                    count++;
                    break;
                }
                case 'x': {
                    int val = va_arg(args, int);
                    print_int(val, 16);
                    count++;
                    break;
                }
                case '%': {
                    putchar('%');
                    count++;
                    break;
                }
                default:
                    putchar(format[i]);
                    count++;
                    break;
            }
        } else {
            putchar(format[i]);
            count++;
        }
    }
    va_end(args);
    return count;
}