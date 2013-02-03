#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include "vfs.h"
#include "fs.h"

struct ext2_bgd
{
	uint32_t block_bitmap_block_address;
	uint32_t inode_bitmap_block_address;
	uint32_t inode_table_start_block;
	uint16_t unallocated_block_count;
	uint16_t unallocated_inode_count;
	uint16_t directory_count;
	uint8_t reserved[14];
} __attribute__ ((packed));

struct ext2_fs {
	struct fs b;

	uint32_t total_inodes;
	uint32_t total_blocks;
	uint32_t inode_size;
	uint32_t block_size;
	uint32_t inodes_per_group;
	uint32_t blocks_per_group;
	uint32_t total_groups;
	int major_version;
	int minor_version;
	uint32_t pointers_per_indirect_block;
	uint32_t pointers_per_indirect_block_2;
	uint32_t pointers_per_indirect_block_3;

	// cache the block group descriptor table
	struct ext2_bgd *bgdt;
};

struct ext2_inode {
	uint16_t type_permissions;
	uint16_t user_id;
	uint32_t size;
	uint32_t last_access_time;
	uint32_t creation_time;
	uint32_t last_modification_time;
	uint32_t deletion_time;
	uint16_t group_id;
	uint16_t hard_link_count;
	uint32_t sector_count;
	uint32_t flags;
	uint32_t os_specific_1;
	uint32_t db0;
	uint32_t db1;
	uint32_t db2;
	uint32_t db3;
	uint32_t db4;
	uint32_t db5;
	uint32_t db6;
	uint32_t db7;
	uint32_t db8;
	uint32_t db9;
	uint32_t db10;
	uint32_t db11;
	uint32_t sibp;
	uint32_t dibp;
	uint32_t tibp;
	uint32_t generation_number;
	uint32_t extended_attribute_block;
	uint32_t size_upper_bits;
	uint32_t fragment_block_address;
	uint32_t os_opecific[3];
} __attribute__ ((packed));

static uint32_t get_sector_num(struct ext2_fs *fs, uint32_t block_no)
{
	return block_no * fs->block_size / fs->b.parent->block_size;
}

static uint8_t *read_block(struct ext2_fs *fs, uint32_t block_no)
{
	uint8_t *ret = (uint8_t *)malloc(fs->block_size);
	int br_ret = block_read(fs->b.parent, ret, fs->block_size,
			get_sector_num(fs, block_no));
	if(br_ret < 0)
	{
		printf("EXT2: block_read returned %i\n");
		return (void*)0;
	}
	return ret;
}

static uint32_t get_block_no_from_inode(struct ext2_fs *fs, struct ext2_inode *i, uint32_t index)
{
	// If the block index is < 12 use the direct block pointers
	if(index < 12)
		return ((uint32_t *)&i->db0)[index];
	else
	{
		// If the block index is < (12 + pointers_per_indirect_block),
		// use the singly-indirect block pointer
		index -= 12;

		if(index < fs->pointers_per_indirect_block)
		{
			// Load the singly indirect block
			uint8_t *sib = read_block(fs, i->sibp);
			if(!sib)
				return 0;

			uint32_t ret = ((uint32_t *)sib)[index];
			free(sib);
			return ret;
		}
		else
		{
			index -= fs->pointers_per_indirect_block;

			// If the index is < pointers_per_indirect_block ^ 2,
			// use the doubly-indirect block pointer

			if(index < (fs->pointers_per_indirect_block_2))
			{
				uint32_t dib_index = index / fs->pointers_per_indirect_block;
				uint32_t sib_index = index % fs->pointers_per_indirect_block;

				// Load the doubly indirect block
				uint8_t *dib = read_block(fs, i->dibp);
				if(!dib)
					return 0;

				uint32_t sib_block = ((uint32_t *)dib)[dib_index];
				free(dib);

				// Load the appropriate singly indirect block
				uint8_t *sib = read_block(fs, sib_block);
				if(!sib)
					return 0;

				uint32_t ret = ((uint32_t *)sib)[sib_index];
				free(sib);
				return ret;
			}
			else
			{
				index -= fs->pointers_per_indirect_block_2;

				// Else use a triply indirect block or fail
				if(index < fs->pointers_per_indirect_block_3)
				{
					uint32_t tib_index = index /
						fs->pointers_per_indirect_block_2;
					uint32_t tib_rem = index %
						fs->pointers_per_indirect_block_2;
					uint32_t dib_index = tib_rem /
						fs->pointers_per_indirect_block;
					uint32_t sib_index = tib_rem %
						fs->pointers_per_indirect_block;

					// Load the triply indirect block
					uint8_t *tib = read_block(fs, i->tibp);
					if(!tib)
						return 0;

					uint32_t dib_block = ((uint32_t *)tib)[tib_index];
					free(tib);

					// Load the appropriate doubly indirect block
					uint8_t *dib = read_block(fs, dib_block);
					if(!dib)
						return 0;

					uint32_t sib_block = ((uint32_t *)dib)[dib_index];
					free(dib);

					// Load the appropriate singly indirect block
					uint8_t *sib = read_block(fs, sib_block);
					if(!sib)
						return 0;

					uint32_t ret = ((uint32_t *)sib)[sib_index];
					free(sib);
					return ret;
				}
				else
				{
					printf("EXT2: invalid block number\n");
					return 0;
				}
			}
		}
	}
}


//static struct dirent *fat_read_dir(struct fat_fs *fs, struct dirent *d);
//struct dirent *fat_read_directory(struct fs *fs, char **name);
//static size_t fat_read_from_file(struct fat_fs *fs, uint32_t starting_cluster, uint8_t *buf,
//		size_t byte_count, size_t offset);

/*static FILE *ext2_fopen(struct fs *fs, struct dirent *path, const char *mode)
{
	if(fs != path->fs)
		return (FILE *)0;

	struct vfs_file *ret = (struct vfs_file *)malloc(sizeof(struct vfs_file));
	memset(ret, 0, sizeof(struct vfs_file));
	ret->fs = fs;
	ret->pos = 0;
	ret->opaque = path->opaque;
	ret->len = (long)path->byte_size;

	(void)mode;
	return ret;
}

static size_t ext2_fread(struct fs *fs, void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	if(stream->fs != fs)
		return -1;
	if(stream->opaque == (void *)0)
		return -1;

	return ext2_read_from_file((struct fat_fs *)fs, (uint32_t)stream->opaque, (uint8_t *)ptr,
			size * nmemb, (size_t)stream->pos);
}

static int ext2_fclose(struct fs *fs, FILE *fp)
{
	(void)fs;
	(void)fp;
	return 0;
} */

void read_test(struct ext2_fs *fs)
{
	uint32_t inode_number = 1;

	// First find which block group the inode is in
	uint32_t block_idx = inode_number / fs->inodes_per_group;
	uint32_t block_offset = inode_number % fs->inodes_per_group;
	struct ext2_bgd *b = &fs->bgdt[block_idx];

	// Now find which block in its inode table its in
	uint32_t inodes_per_block = fs->block_size / fs->inode_size;
	block_idx = block_offset / inodes_per_block;
	block_offset = block_offset % inodes_per_block;

	// Now read the appropriate block and extract the inode
	uint8_t *itable = read_block(fs, b->inode_table_start_block + block_idx);
	struct ext2_inode *inode = (struct ext2_inode *)&itable[block_offset * fs->inode_size];

	// Now read the first block of the root directory
	uint32_t root_dir_block_0 = get_block_no_from_inode(fs, inode, 0);
	uint8_t *rdir = read_block(fs, root_dir_block_0);

	(void)rdir;
}

int ext2_init(struct block_device *parent, struct fs **fs)
{
	// Interpret an EXT2 file system
#ifdef DEBUG
	printf("EXT2: looking for a filesytem on %s\n", parent->device_name);
#endif

	// Read superblock
	uint8_t *sb = (uint8_t *)malloc(1024);
	int r = block_read(parent, sb, 1024, 1024 / parent->block_size);
	if(r < 0)
	{
		printf("EXT2: error %i reading block 0\n", r);
		return r;
	}
	if(r != 1024)
	{
		printf("EXT2: error reading superblock (only %i bytes read)\n", r);
		return -1;
	}

	// Confirm its ext2
	if(*(uint16_t *)&sb[56] != 0xef53)
	{
		printf("EXT2: not a valid ext2 filesystem on %s\n", parent->device_name);
		return -1;
	}

	struct ext2_fs *ret = (struct ext2_fs *)malloc(sizeof(struct ext2_fs));
	//ret->b.fopen = ext2_fopen;
	//ret->b.fread = ext2_fread;
	//ret->b.fclose = ext2_fclose;
	//ret->b.read_directory = ext2_read_directory;
	ret->b.parent = parent;

	ret->total_inodes = *(uint32_t *)&sb[0];
	ret->total_blocks = *(uint32_t *)&sb[4];
	ret->inodes_per_group = *(uint32_t *)&sb[40];
	ret->blocks_per_group = *(uint32_t *)&sb[32];
	ret->block_size = 1024 << *(uint32_t *)&sb[24];
	ret->minor_version = *(int16_t *)&sb[62];
	ret->major_version = *(int32_t *)&sb[76];

	if(ret->major_version >= 1)
	{
		// Read extended superblock
		ret->inode_size = *(uint16_t *)&sb[88];
	}
	else
		ret->inode_size = 128;

	// Calculate the number of block groups by two different methods and ensure they tally
	uint32_t i_calc_val = ret->total_inodes / ret->inodes_per_group;
	uint32_t i_calc_rem = ret->total_inodes % ret->inodes_per_group;
	if(i_calc_rem)
		i_calc_val++;

	uint32_t b_calc_val = ret->total_blocks / ret->blocks_per_group;
	uint32_t b_calc_rem = ret->total_blocks % ret->blocks_per_group;
	if(b_calc_rem)
		b_calc_val++;

	if(i_calc_val != b_calc_val)
	{
		printf("EXT2: total group calculation by block method (%i) and inode "
				"method (%i) differs\n", b_calc_val, i_calc_val);
		return -1;
	}

	ret->total_groups = i_calc_val;
	ret->pointers_per_indirect_block = ret->block_size / 32;
	ret->pointers_per_indirect_block_2 = ret->pointers_per_indirect_block *
		ret->pointers_per_indirect_block;
	ret->pointers_per_indirect_block_3 = ret->pointers_per_indirect_block_2 *
		ret->pointers_per_indirect_block;

	// Read the block group descriptor table
	ret->bgdt = (struct ext2_bgd *)malloc(ret->total_groups * sizeof(struct ext2_bgd));
	int bgdt_block = 1;
	if(ret->block_size == 1024)
		bgdt_block = 2;
	block_read(parent, (uint8_t *)ret->bgdt, ret->total_groups * sizeof(struct ext2_bgd),
			get_sector_num(ret, bgdt_block));




	*fs = (struct fs *)ret;
	free(sb);

	printf("EXT2: found an ext2 filesystem on %s\n", ret->b.parent->device_name);


	read_test(ret);

	while(1);

	return 0;
}

/*uint32_t get_sector(struct fat_fs *fs, uint32_t rel_cluster)
{
	rel_cluster -= 2;
	return fs->first_non_root_sector + rel_cluster * fs->sectors_per_cluster;
}

static uint32_t get_next_fat_entry(struct fat_fs *fs, uint32_t current_cluster)
{
	switch(fs->fat_type)
	{
		case FAT16:
			{
				uint32_t fat_offset = current_cluster << 1; // *2
				uint32_t fat_sector = fs->first_fat_sector +
					(fat_offset / fs->bytes_per_sector);
				uint8_t *buf = (uint8_t *)malloc(512);
				int br_ret = block_read(fs->b.parent, buf, 512, fat_sector);
				if(br_ret < 0)
				{
					printf("FAT: block_read returned %i\n");
					return 0x0ffffff7;
				}
				uint32_t fat_index = fat_offset % fs->bytes_per_sector;
				uint32_t next_cluster = (uint32_t)*(uint16_t *)&buf[fat_index];
				free(buf);
				if(next_cluster >= 0xfff7)
					next_cluster |= 0x0fff0000;
				return next_cluster;
			}

		case FAT32:
			{
				uint32_t fat_offset = current_cluster << 2; // *4
				uint32_t fat_sector = fs->first_fat_sector +
					(fat_offset / fs->bytes_per_sector);
				uint8_t *buf = (uint8_t *)malloc(512);
				int br_ret = block_read(fs->b.parent, buf, 512, fat_sector);
				if(br_ret < 0)
				{
					printf("FAT: block_read returned %i\n");
					return 0x0ffffff7;
				}
				uint32_t fat_index = fat_offset % fs->bytes_per_sector;
				uint32_t next_cluster = *(uint32_t *)&buf[fat_index];
				free(buf);
				return next_cluster & 0x0fffffff; // FAT32 is actually FAT28
			}
		default:
			printf("FAT: fat type %s not supported\n", fs->b.fs_name);
			return 0;
	}
}

struct dirent *fat_read_directory(struct fs *fs, char **name)
{
	struct dirent *cur_dir = fat_read_dir((struct fat_fs *)fs, (void*)0);
	while(*name)
	{
		// Search the directory entries for one of the requested name
		int found = 0;
		while(cur_dir)
		{
			if(!strcmp(*name, cur_dir->name))
			{
				found = 1;
				cur_dir = fat_read_dir((struct fat_fs *)fs, cur_dir);
				name++;
				break;
			}
			cur_dir = cur_dir->next;
		}
		if(!found)
		{
			printf("FAT: path part %s not found\n", *name);
			return (void*)0;
		}
	}
	return cur_dir;
}

static size_t fat_read_from_file(struct fat_fs *fs, uint32_t starting_cluster, uint8_t *buf,
		size_t byte_count, size_t offset)
{
	uint32_t cur_cluster = starting_cluster;
	size_t cluster_size = fs->bytes_per_sector * fs->sectors_per_cluster;

	size_t location_in_file = 0;
	int buf_ptr = 0;
	while(cur_cluster < 0x0ffffff8)
	{
		if((location_in_file + cluster_size) > offset)
		{
			if(location_in_file < (offset + byte_count))
			{
				// This cluster contains part of the requested file
				// Load it
				
				uint32_t sector = get_sector(fs, cur_cluster);
				uint8_t *read_buf = (uint8_t *)malloc(cluster_size);
				int rb_ret = block_read(fs->b.parent, read_buf, cluster_size, sector);

				if(rb_ret < 0)
					return rb_ret;

				// Now decide how much of this sector we need to load
				int len;
				int c_ptr;

				if(offset >= location_in_file)
				{
					// This is the first cluster containing the file
					buf_ptr = 0;
					c_ptr = offset - location_in_file;
					len = byte_count;
					if(len > (int)cluster_size)
						len = cluster_size;
				}
				else
				{
					// We are starting somewhere within the file
					c_ptr = 0;
					len = byte_count - buf_ptr;
					if(len > (int)cluster_size)
						len = cluster_size;
					if(len > (int)(byte_count - buf_ptr))
						len = byte_count - buf_ptr;
				}

				// Copy to the buffer
				memcpy(&buf[buf_ptr], &read_buf[c_ptr], len);
				free(read_buf);

				buf_ptr += len;
			}
		}

		cur_cluster = get_next_fat_entry(fs, cur_cluster);
		location_in_file += cluster_size;
	}

	return buf_ptr;
}*/

/*struct dirent *fat_read_dir(struct fat_fs *fs, struct dirent *d)
{
	int is_root = 0;
	struct fat_fs *fat = (struct fat_fs *)fs;

	if(d == (void*)0)
		is_root = 1;

	uint32_t cur_cluster;
	uint32_t cur_root_cluster_offset = 0;
	if(is_root)
	{
		switch(fat->fat_type)
		{
			case FAT12:
			case FAT16:
				cur_cluster = 0;
				break;
			case FAT32:
				printf("FAT32 not yet supported\n");
				return (void*)0;
		}
	}
	else
		cur_cluster = (uint32_t)d->opaque;

	struct dirent *ret = (void *)0;
	struct dirent *prev = (void *)0;

	do
	{
		// Read this cluster
		uint32_t cluster_size = fat->bytes_per_sector * fat->sectors_per_cluster;
		uint8_t *buf = (uint8_t *)malloc(cluster_size);

		// Interpret the cluster number to an absolute address
		uint32_t absolute_cluster = cur_cluster;
		uint32_t first_data_sector = fat->first_data_sector;
		if(!is_root)
		{
			absolute_cluster -= 2;
			first_data_sector = fat->first_non_root_sector;
		}
		
#ifdef DEBUG
		printf("FAT: reading cluster %i (sector %i)\n", cur_cluster,
				absolute_cluster * fat->sectors_per_cluster + first_data_sector);
#endif
		int br_ret = block_read(fat->b.parent, buf, cluster_size, 
				absolute_cluster * fat->sectors_per_cluster + first_data_sector);

		if(br_ret < 0)
		{
			printf("FAT: block_read returned %i\n", br_ret);
			return (void*)0;
		}

		int ptr = 0;

		for(uint32_t i = 0; i < cluster_size; i++, ptr += 32)
		{
			// Does the entry exist (if the first byte is zero of 0xe5 it doesn't)
			if((buf[ptr] == 0) || (buf[ptr] == 0xe5))
				continue;

			// Is it the directories '.' or '..'?
			if(buf[ptr] == '.')
				continue;

			// Is it a long filename entry (if so ignore)
			if(buf[ptr + 11] == 0x0f)
				continue;

			// Else read it
			struct dirent *de = (struct dirent *)malloc(sizeof(struct dirent));
			memset(de, 0, sizeof(struct dirent));
			if(ret == (void *)0)
				ret = de;
			if(prev != (void *)0)
				prev->next = de;
			prev = de;

			de->name = (char *)malloc(13);
			de->fs = &fs->b;
			// Convert to lowercase on load
			int d_idx = 0;
			int in_ext = 0;
			int has_ext = 0;
			for(int i = 0; i < 11; i++)
			{
				char cur_v = (char)buf[ptr + i];
				if(i == 8)
				{
					in_ext = 1;
					de->name[d_idx++] = '.';
				}
				if(cur_v == ' ')
					continue;
				if(in_ext)
					has_ext = 1;
				if((cur_v >= 'A') && (cur_v <= 'Z'))
					cur_v = 'a' + cur_v - 'A';
				de->name[d_idx++] = cur_v;
			}
			if(!has_ext)
				de->name[d_idx - 1] = 0;
			else
				de->name[d_idx] = 0;

			if(buf[ptr + 11] & 0x10)
				de->is_dir = 1;
			de->next = (void *)0;
			de->byte_size = *(uint32_t *)&buf[ptr + 28];
			uint32_t opaque = *(uint16_t *)&buf[ptr + 26] | 
				((uint32_t)(*(uint16_t *)&buf[ptr + 20]) << 16);

			de->opaque = (void*)opaque;

#ifdef DEBUG
			printf("FAT: read dir entry: %s, size %i, cluster %i\n", 
					de->name, de->byte_size, opaque);
#endif
		}
		free(buf);

		// Get the next cluster
		if(is_root)
		{
			cur_root_cluster_offset++;
			if(cur_root_cluster_offset < (fat->root_dir_sectors /
						fat->sectors_per_cluster))
				cur_cluster++;
			else
				cur_cluster = 0x0ffffff8;
		}
		else
			cur_cluster = get_next_fat_entry(fat, cur_cluster);
	} while(cur_cluster < 0x0ffffff7);

	return ret;
}

*/

