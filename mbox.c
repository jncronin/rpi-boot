#include <stdint.h>
#include "mbox.h"
#include "mmio.h"

#define MBOX_FULL		0x80000000
#define	MBOX_EMPTY		0x40000000

uint32_t mbox_read(uint8_t channel)
{
	while(1)
	{
		while(mmio_read(MBOX_BASE + MBOX_STATUS) & MBOX_EMPTY);

		uint32_t data = mmio_read(MBOX_BASE + MBOX_READ);
		uint8_t read_channel = (uint8_t)(data & 0xf);
		if(read_channel == channel)
			return (data & 0xfffffff0);
	}
}

void mbox_write(uint8_t channel, uint32_t data)
{
	while(mmio_read(MBOX_BASE + MBOX_STATUS) & MBOX_FULL);
	mmio_write(MBOX_BASE + MBOX_WRITE, (data & 0xfffffff0) | (uint32_t)(channel & 0xf));
}

