#ifndef MBOX_H
#define MBOX_H

#include <stdint.h>

#define MBOX_BASE 0x2000b880

#define MBOX_PEEK 0x10
#define MBOX_READ 0x00
#define MBOX_WRITE 0x20
#define MBOX_STATUS 0x18
#define MBOX_SENDER 0x14
#define MBOX_CONFIG 0x1c

#define MBOX_FB		1
#define MBOX_PROP	8

#define MBOX_SUCCESS	0x80000000

uint32_t mbox_read(uint8_t channel);
void mbox_write(uint8_t channel, uint32_t data);

#endif

