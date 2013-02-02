#include <stdint.h>
#include <stdio.h>
#include "block.h"

int block_read(struct block_device *dev, uint8_t *buf, size_t buf_size, uint32_t starting_block)
{
	// Read the required number of blocks to satisfy the request
	int buf_offset = 0;
	uint32_t block_offset = 0;

	do
	{
		size_t to_read = buf_size;
		if(to_read > dev->block_size)
			to_read = dev->block_size;

#ifdef DEBUG
		printf("block_read: reading %i bytes from block %i on %s\n", to_read,
				starting_block + block_offset, dev->device_name);
#endif

		int ret = dev->read(dev, &buf[buf_offset], to_read, starting_block + block_offset);
		if(ret < 0)
			return ret;

		buf_offset += (int)to_read;
		block_offset++;

		if(buf_size < dev->block_size)
			buf_size = 0;
		else
			buf_size -= dev->block_size;
	} while(buf_size > 0);

	return buf_offset;
}

