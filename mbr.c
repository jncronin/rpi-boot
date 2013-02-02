#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "block.h"
#include "vfs.h"

int fat_init(struct block_device *, struct fs **);

// Code for interpreting an mbr

struct mbr_block_dev {
	struct block_device bd;
	struct block_device *parent;
	int part_no;
	uint32_t start_block;
	uint32_t blocks;
	uint8_t part_id;
};

static char driver_name[] = "mbr";

static int mbr_read(struct block_device *, uint8_t *buf, size_t buf_size, uint32_t starting_block);

int read_mbr(struct block_device *parent, struct block_device ***partitions, int *part_count)
{
	(void)partitions;
	(void)part_count;
	(void)driver_name;
	/* Check the validity of the parent device */
	if(parent == (void*)0)
	{
		printf("MBR: invalid parent device\n");
		return -1;
	}

	/* Read the first 512 bytes */
	uint8_t *block_0 = (uint8_t *)malloc(512);
#ifdef DEBUG
	printf("MBR: reading block 0 from device %s\n", parent->device_name);
#endif
	
	int ret = block_read(parent, block_0, 512, 0);
	if(ret < 0)
	{
		printf("MBR: block_read failed (%i)\n", ret);
		return ret;
	}
	if(ret != 512)
	{
		printf("MBR: unable to read first 512 bytes of device %s, only %d bytes read\n",
				parent->device_name, ret);
		return -1;
	}

	/* Check the MBR signature */
	if((block_0[0x1fe] != 0x55) || (block_0[0x1ff] != 0xaa))
	{
		printf("MBR: no valid mbr signature on device %s (bytes are %x %x)\n",
				parent->device_name, block_0[0x1fe], block_0[0x1ff]);
		return -1;
	}
	printf("MBR: found valid MBR on device %s\n", parent->device_name);

	/* Load the partitions */
	struct block_device **parts =
		(struct block_device **)malloc(4 * sizeof(struct block_device *));
	int cur_p = 0;
	for(int i = 0; i < 4; i++)
	{
		int p_offset = 0x1be + (i * 0x10);
		if(block_0[p_offset + 4] != 0x00)
		{
			// Valid partition
			struct mbr_block_dev *d =
				(struct mbr_block_dev *)malloc(sizeof(struct mbr_block_dev));
			memset(d, 0, sizeof(struct mbr_block_dev));
			d->bd.driver_name = driver_name;
			char *dev_name = (char *)malloc(strlen(parent->device_name + 2));
			strcpy(dev_name, parent->device_name);
			dev_name[strlen(parent->device_name)] = '_';
			dev_name[strlen(parent->device_name) + 1] = '0' + i;
			dev_name[strlen(parent->device_name) + 2] = 0;
			d->bd.device_name = dev_name;
			d->bd.device_id = (uint8_t *)malloc(1);
			d->bd.device_id[0] = i;
			d->bd.dev_id_len = 1;
			d->bd.read = mbr_read;
			d->bd.block_size = 512;
			d->part_no = i;
			d->part_id = block_0[p_offset + 4];
			d->start_block = *(uint32_t *)&block_0[p_offset + 8];
			d->blocks = *(uint32_t *)&block_0[p_offset + 12];
			d->parent = parent;
			
			parts[cur_p++] = (struct block_device *)d;
#ifdef DEBUG
			printf("MBR: partition number %i (%s) of type %x\n", 
					d->part_no, d->bd.device_name, d->part_id);
#endif

			switch(d->part_id)
			{
				case 1:
				case 4:
				case 6:
				case 0xb:
				case 0xe:
				case 0xf:
				case 0x11:
				case 0x14:
				case 0x1b:
				case 0x1c:
				case 0x1e:
					fat_init((struct block_device *)d, &d->bd.fs);
					break;
			}

			if(d->bd.fs)
				vfs_register(d->bd.fs);
		}
	}

	*partitions = parts;
	*part_count = cur_p;
	printf("MBR: found total of %i partition(s)\n", cur_p);

	return 0;
}

int mbr_read(struct block_device *dev, uint8_t *buf, size_t buf_size, uint32_t starting_block)
{
	struct block_device *parent = ((struct mbr_block_dev *)dev)->parent;
	// Check the block size of the partition is equal that of the underlying device
	if(dev->block_size != parent->block_size)
	{
		printf("MBR: read() error - block size differs (%i vs %i)\n",
				dev->block_size,
				parent->block_size);
		return -1;
	}

	return parent->read(parent, buf, buf_size,
			starting_block + ((struct mbr_block_dev *)dev)->start_block);
}

