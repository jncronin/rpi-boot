#ifndef BCM2836_H
#define BCM2836_H

#ifdef __cplusplus
extern "C" {
#endif

#include "core_ca7.h"
#include <stdint.h>

typedef volatile uint32_t vuint32_t;

// #ifdef RPI2
    ////  Peripheral address ////
    #define PHY_PERI_BASE   (0x3F000000uL)
// #else
    // #define PHY_PERI_BASE   (0x20000000uL)
// #endif

// GPIO
#define PHY_GPIO_BASE   (PHY_PERI_BASE + 0x00200000)
#define GPFSEL(n)       (PHY_GPIO_BASE + 0x00 + (n*4))
#define GPSET(n)        (PHY_GPIO_BASE + 0x1c + (n*4))
#define GPCLR(n)        (PHY_GPIO_BASE + 0x28 + (n*4))
#define GPLEV(n)        (PHY_GPIO_BASE + 0x34 + (n*4))
#define GPPUD           (PHY_GPIO_BASE + 0x94)
#define GPPUDCLK(n)     (PHY_GPIO_BASE + 0x98 + (n * 4))
// UART0
#define PHY_UART0_BASE  (PHY_PERI_BASE + 0x00201000)
#define UART0_DR        (PHY_UART0_BASE + 0x00)
#define UART0_RSRECR    (PHY_UART0_BASE + 0x04)
#define UART0_FR        (PHY_UART0_BASE + 0x18)
// #define UART0_ILPR       (PHY_UART0_BASE + 0x20) // not use
#define UART0_IBRD      (PHY_UART0_BASE + 0x24)
#define UART0_FBRD      (PHY_UART0_BASE + 0x28)
#define UART0_LCRH      (PHY_UART0_BASE + 0x2c)
#define UART0_CR        (PHY_UART0_BASE + 0x30)
#define UART0_IFLS      (PHY_UART0_BASE + 0x34)
#define UART0_IMSC      (PHY_UART0_BASE + 0x38)
#define UART0_RIS       (PHY_UART0_BASE + 0x3c)
#define UART0_MIS       (PHY_UART0_BASE + 0x40)
#define UART0_ICR       (PHY_UART0_BASE + 0x44)
#define UART0_DMACR     (PHY_UART0_BASE + 0x48)
#define UART0_ITCR      (PHY_UART0_BASE + 0x80)
#define UART0_ITIP      (PHY_UART0_BASE + 0x84)
#define UART0_ITOP      (PHY_UART0_BASE + 0x88)
#define UART0_TDR       (PHY_UART0_BASE + 0x8c)
// Timer
#define SYSTIMER_BASE   (PHY_PERI_BASE + 0x00003000)
#define SYSTIMER_CS     (SYSTIMER_BASE + 0x00)
#define SYSTIMER_CLO    (SYSTIMER_BASE + 0x04)
#define SYSTIMER_CHI    (SYSTIMER_BASE + 0x08)
#define SYSTIMER_C0     (SYSTIMER_BASE + 0x0c)
#define SYSTIMER_C1     (SYSTIMER_BASE + 0x10)
#define SYSTIMER_C2     (SYSTIMER_BASE + 0x14)
#define SYSTIMER_C3     (SYSTIMER_BASE + 0x18)
// USB
#define DWC_USB_BASE    (PHY_PERI_BASE + 0x00980000)
// EMMC
#define EMMC_BASE       (PHY_PERI_BASE + 0x00300000)

//// objects ////
typedef union{
    struct{
        unsigned int reserved:24;
        // Stick Parity Select
        unsigned int SPS:1;
        // Word Length
        unsigned int WLEN:2;
        // Enable FIFOs
        unsigned int FEN:1;
        unsigned int STP2:1;
        unsigned int EPS:1;
        unsigned int PEN:1;
        unsigned int BRK:1;
    } bits;
    vuint32_t raw;
} UART0_LCRH_BF;

typedef union{
    struct{
        unsigned int reserved2:16;
        unsigned int CTSEN:1;
        unsigned int RTSEN:1;
        unsigned int OUT2:1; // unsupported
        unsigned int OUT1:1; // unsupported
        unsigned int RTS:1;
        unsigned int DTR:1; // unsupported
        unsigned int RXE:1;
        unsigned int TXE:1;
        unsigned int LBE:1;
        unsigned int reserved1:4;
        unsigned int SIRLP:1; // unsupported
        unsigned int SIREN:1; // unsupported
        unsigned int UARTEN:1;
    } bits;
    vuint32_t raw;
} UART0_CR_BF;

typedef union{
    struct{
        unsigned int reserved:23;
        unsigned int RI:1; // unsupported
        unsigned int TXFE:1;
        unsigned int RXFF:1;
        unsigned int TXFF:1;
        unsigned int RXFE:1;
        unsigned int BUSY:1;
        unsigned int DCD:1; // unsupported
        unsigned int DSR:1; // unsupported
        unsigned int CTS:1;
    } bits;
    vuint32_t raw;
} UART0_FR_BF;

#ifdef __cplusplus
}
#endif

#endif
