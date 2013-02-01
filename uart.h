#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init();
int uart_putc(int byte);
void uart_puts(const char *str);

#endif

