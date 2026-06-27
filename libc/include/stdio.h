#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

#define EOF (-1)

typedef struct {
    int fd;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Unformatted Input/Output */
int putchar(int c);
int puts(const char *s);
int getchar(void);

int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);
int fgetc(FILE *stream);

/* Formatted Output */
int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);

int vfprintf(FILE *stream, const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);

#endif