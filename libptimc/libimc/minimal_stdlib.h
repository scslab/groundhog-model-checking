#pragma once

#include <stddef.h>

int printf(const char *, ...);
int fprintf(void *, const char *, ...);
void *malloc(size_t);
void *realloc(void*, size_t);
void exit(int);
void *memset(void *, int, size_t);
extern void *stderr;

// FOR RAFT
#include <stdarg.h>
int vsprintf(char *str, const char *format, va_list ap);
void *calloc(size_t, size_t);
void free(void *);
int rand(void);
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void reset_malloc();
#define assert(...) do { } while (0)

#ifdef OX64
#include "uart.h"
static void print_num(size_t x) {
    if (!x) { uart_putc(0, '0'); return; }
    char chars[128] = {0};
    int i = 0;
    for (; i < 128 && x; i++, x /= 10) chars[i] = '0' + (x % 10);
    while (i --> 0) uart_putc(0, chars[i]);
}
#endif
