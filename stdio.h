#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t FILE;

#define stdin ((FILE *)0)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)

int fputc(int c, FILE *stream);
int fputs(const char *, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);

int printf(const char *format, ...);
int fprintf(FILE *stream, const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);

void puthex(uint32_t val);

#endif

