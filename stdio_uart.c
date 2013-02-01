#include "uart.h"
#include "stdio.h"

int uart_putc(int c)
{
	uart_putc((uint8_t)c);
	return c;
}

