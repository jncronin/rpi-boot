#include <stdint.h>
#include "mmio.h"
#include "uart.h"

#define GPIO_BASE 			0x20200000
#define GPPUD 				(GPIO_BASE + 0x94)
#define GPPUDCLK0 			(GPIO_BASE + 0x98)
#define UART0_BASE			0x20201000
#define UART0_DR			(UART0_BASE + 0x00)
#define UART0_RSRECR			(UART0_BASE + 0x04)
#define UART0_FR			(UART0_BASE + 0x18)
#define UART0_ILPR			(UART0_BASE + 0x20)
#define UART0_IBRD			(UART0_BASE + 0x24)
#define UART0_FBRD			(UART0_BASE + 0x28)
#define UART0_LCRH			(UART0_BASE + 0x2C)
#define UART0_CR			(UART0_BASE + 0x30)
#define UART0_IFLS			(UART0_BASE + 0x34)
#define UART0_IMSC			(UART0_BASE + 0x38)
#define UART0_RIS			(UART0_BASE + 0x3C)
#define UART0_MIS			(UART0_BASE + 0x40)
#define UART0_ICR			(UART0_BASE + 0x44)
#define UART0_DMACR			(UART0_BASE + 0x48)
#define UART0_ITCR			(UART0_BASE + 0x80)
#define UART0_ITIP			(UART0_BASE + 0x84)
#define UART0_ITOP			(UART0_BASE + 0x88)
#define UART0_TDR			(UART0_BASE + 0x8C)

// delay function
static void delay(int32_t count)
{
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n" : : [count]"r"(count) : "cc");
}

void uart_init()
{
	mmio_write(UART0_CR, 0x0);

	mmio_write(GPPUD, 0x0);
	delay(150);

	mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);

	mmio_write(GPPUDCLK0, 0x0);

	mmio_write(UART0_ICR, 0x7ff);

	mmio_write(UART0_IBRD, 1);
	mmio_write(UART0_FBRD, 40);

	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) |
				(1 << 8) | (1 << 9) | (1 << 10));

	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

int uart_putc(int byte)
{
	while(1) {
		if (!(mmio_read(UART0_FR) & (1 << 5)))
			break;
	}
	mmio_write(UART0_DR, (uint8_t)(byte & 0xff));
	return byte;
}

void uart_puts(const char *str)
{
	while(*str)
		uart_putc(*str++);
}

