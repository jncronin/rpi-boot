// Parses the .cfg file

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "atag.h"
#include "elf.h"
#include "memchunk.h"
#include "vfs.h"
#include "multiboot.h"
#include "console.h"
#include "fb.h"

static int method_multiboot(char *args);
static int method_boot(char *args);
static int method_module(char *args);

static void atag_cb(struct atag *);
static void atag_cb2(struct atag *);

extern uint32_t _atags;
extern uint32_t _arm_m_type;
extern char *rpi_boot_name;
uint32_t *mmap_ptr;

struct multiboot_info *mbinfo = (void *)0;
uint32_t entry_addr = 0;

struct multiboot_method
{
	char *name;
	int (*method)(char *args);
};

static struct multiboot_method methods[] =
{
	{
		.name = "multiboot",
		.method = method_multiboot
	},
	{
		.name = "boot",
		.method = method_boot
	},
	{
		.name = "module",
		.method = method_module
	}
};

struct multiboot_arm_functions funcs =
{
	.printf = printf,
	.clear = clear,
	.fopen = fopen,
	.fread = fread,
	.fclose = fclose,
	.fseek = fseek,
	.opendir = opendir,
	.readdir = readdir,
	.closedir = closedir
};

static char *read_line(char **buf)
{
	char *start = *buf;
	char *ptr = *buf;

	if(!*start)
		return (void*)0;
	while((*ptr != 0) && (*ptr != '\n'))
		ptr++;

	if(*ptr == 0)
	{
		// End of the string
		*buf = ptr;
		return start;
	}
	else
	{
		// End of a line - null terminate
		*ptr = 0;
		ptr++;
		*buf = ptr;
		return start;
	}
}

char empty_string[] = "";

static void split_string(char *str, char **method, char **args)
{
	int state = 0;
	char *p = str;

	// state = 	0 - reading spaces before method
	// 		1 - reading method
	// 		2 - reading spaces before argument
	
	*method = empty_string;
	*args = empty_string;
	
	while(p)
	{
		if(*p == ' ')
		{
			if(state == 1)
			{
				*p = 0;	// null terminate method
				state = 2;
			}
		}
		else
		{
			if(state == 0)
			{
				*method = p;
				state = 1;
			}
			else if(state == 2)
			{
				*args = p;
				return;
			}
		}
		p++;
	}
}

int cfg_parse(char *buf)
{
	char *line;
	char *b = buf;
	while((line = read_line(&b)))
	{
#ifdef DEBUG
		printf("read_line: %s\n", line);
#endif
		char *method, *args;
		split_string(line, &method, &args);
#ifdef DEBUG
		printf("method: %s, args: %s\n", method, args);
#endif

		if(!strcmp(method, empty_string))
			continue;

		// Find and run the method
		int method_count = sizeof(methods) / sizeof(struct multiboot_method);
		int found = 0;
		for(int i = 0; i < method_count; i++)
		{
			if(!strcmp(method, methods[i].name))
			{
				found = 1;
				methods[i].method(args);
				break;
			}
		}

		if(!found)
			printf("cfg_parse: unknown method %s\n", method);	
	}

	return 0;
}

int method_multiboot(char *args)
{
#ifdef DEBUG
	printf("Interpreting multiboot command\n");
#endif
	char *file, *cmd_line;
	split_string(args, &file, &cmd_line);

	// First load up the first 8192 bytes to look for the multiboot header
	FILE *fp = fopen(file, "r");
	if(!fp)
	{
		printf("MULTIBOOT: cannot load %s\n", file);
		return -1;
	}
#ifdef DEBUG
	printf("MULTIBOOT: loading first 8kiB of %s\n", file);
#endif
	uint32_t *first_8k = (uint32_t *)malloc(8192);
	int buf_size = fread(first_8k, 1, 8192, fp);

#ifdef DEBUG
	printf("MULTIBOOT: loaded first %i bytes\n", buf_size);
#endif

	struct multiboot_header *mboot = (void*)0;
	uint32_t header_offset;

	for(int i = 0; i < (int)(buf_size / sizeof(uint32_t)); i++)
	{
		if(first_8k[i] == MULTIBOOT_HEADER_MAGIC)
		{
			// Found a multiboot header
			// Check the checksum
			struct multiboot_header *mb = (struct multiboot_header *)&first_8k[i];
			if((mb->magic + mb->flags + mb->checksum) == 0)
			{
				// Its valid, save it somewhere
				mboot = (struct multiboot_header *)
					malloc(sizeof(struct multiboot_header));
				memcpy(mboot, mb, sizeof(struct multiboot_header));
				header_offset = i * 4;
				break;
			}
		}
	}

	free(first_8k);

	if(!mboot)
	{
		printf("MULTIBOOT: no valid multiboot header found in %s\n", file);
		return -1;
	}

#ifdef DEBUG
	printf("MULTIBOOT: valid multiboot header, flags: %08x\n", mboot->flags);
#endif

	// Create a multiboot info header
	mbinfo = (struct multiboot_info *)malloc(sizeof(struct multiboot_info));
	memset(mbinfo, 0, sizeof(struct multiboot_info));

	// Setup the fields
	if(mboot->flags & (1 << 1))
	{
		// Pass memory information

		// First count the number of memory sections and set mem_lower
		parse_atags(_atags, atag_cb);

		// Allocate the mmap buffer
		mbinfo->mmap_addr = (uint32_t)malloc(mbinfo->mmap_length);
		mmap_ptr = (uint32_t *)mbinfo->mmap_addr;

		// Skip the pointer to the first item (4 bytes in - structure
		// starts at offset -4)
		mbinfo->mmap_addr += 4;

		// Now fill in the buffer
		parse_atags(_atags, atag_cb2);

		// Set flags to say we've provided memory info
		mbinfo->flags |= (1 << 0);
		mbinfo->flags |= (1 << 6);
	}
	

	// Load the file
	if(mboot->flags & (1 << 16))
	{
		// This is an a.out file - load it
		//
		// Check we're loading above 1MiB

		if(mboot->load_addr < 0x100000)
		{
			printf("MULTIBOOT: a.out load below 1 MiB - not supported\n");
			return -1;
		}

		uint32_t file_offset = header_offset - mboot->header_addr + mboot->load_addr;
		uint32_t len;
		if(mboot->load_end_addr)
			len = mboot->load_end_addr - mboot->load_addr;
		else
			len = (uint32_t)fp->len - file_offset;
		uint32_t bss_len = 0;
		if(mboot->bss_end_addr > mboot->load_end_addr)
			bss_len = mboot->bss_end_addr - mboot->load_end_addr;

		entry_addr = mboot->entry_addr;

		// Try and allocate a chunk for the file
		if(!chunk_get_chunk(mboot->load_addr, len + bss_len))
		{
			printf("MULTIBOOT: a.out load - unable to allocate a chunk "
					"between 0x%08x and 0x%08x\n",
					mboot->load_addr, mboot->load_addr + len + bss_len);
			return -1;
		}

		// Load the file
		fseek(fp, (long)file_offset, SEEK_SET);
		size_t b_read = fread((void *)mboot->load_addr, 1, (size_t)len, fp);
		if(b_read != (size_t)len)
		{
			printf("MULTIBOOT: a.out load error - tried to load %i bytes "
					"but could only load %i\n", len, b_read);
			return -1;
		}

		// Zero bss
		if(bss_len)
			memset((void *)(mboot->load_end_addr), 0, bss_len);

		// We don't process a.out symbol tables, therefore don't set bit 4
	}
	else
	{
		// This is an ELF file
		//
		// Load the ELF header

		Elf32_Ehdr *ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
		fseek(fp, 0, SEEK_SET);
		fread(ehdr, 1, sizeof(Elf32_Ehdr), fp);

		// Confirm its an ELF file
		if((ehdr->e_ident[0] != 0x7f) || (ehdr->e_ident[1] != 'E') ||
				(ehdr->e_ident[2] != 'L') ||
				(ehdr->e_ident[3] != 'F'))
		{
			printf("MULTIBOOT: %s is not a valid ELF file (e_ident)\n",
					file);
			return -1;
		}

		// Confirm its a 32 bit file
		if(ehdr->e_ident[EI_CLASS] != ELFCLASS32)
		{
			printf("MULTIBOOT: %s does not describe 32 bit objects\n",
					file);
			return -1;
		}

		// Confirm its a little-endian file
		if(ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
		{
			printf("MULTIBOOT: %s does not contain little-endian data\n",
					file);
			return -1;
		}

		// Confirm its an executable file
		if(ehdr->e_type != ET_EXEC)
		{
			printf("MULTIBOOT: %s is not an executable file\n",
					file);
			return -1;
		}

		// Confirm its for the ARM architecture
		if(ehdr->e_machine != EM_ARM)
		{
			printf("MULTIBOOT: %s is not compiled for the ARM architecture\n",
					file);
			return -1;
		}

		// Load up the section headers
		if(!ehdr->e_shoff || !ehdr->e_shnum)
		{
			printf("MULTIBOOT: %s does not contain a section table\n",
					file);
			return -1;
		}

		size_t bytes_to_load = (size_t)(ehdr->e_shentsize * ehdr->e_shnum);
		fseek(fp, (long)ehdr->e_shoff, SEEK_SET);
		uint8_t *sh_buf = (uint8_t *)malloc(bytes_to_load);
		size_t bytes_read = fread(sh_buf, 1, bytes_to_load, fp);
		if(bytes_read != bytes_to_load)
		{
			printf("MULTIBOOT: error loading section table of %s - only "
					"%i bytes loaded\n", file, bytes_read);
			return -1;
		}

		// Now interpret and load them
		//
		// We do two passes - first loading the sections marked ALLOC to their
		// appropriate addresses, then loading the others (Multiboot requires
		// we load all sections).  This ensures we don't load the sections not
		// marked ALLOC to an address that a later section requires.

		for(unsigned int i = 0; i < ehdr->e_shnum; i++)
		{
			Elf32_Shdr *shdr = (Elf32_Shdr *)&sh_buf[i * ehdr->e_shentsize];

			if(shdr->sh_flags & SHF_ALLOC)
			{
#ifdef DEBUG
				printf("MULTIBOOT: section %i is loadable\n", i);
#endif

				// Try and allocate space for it
				if(!shdr->sh_addr)
				{
					printf("MULTIBOOT: section %i has no defined "
							"load address\n", i);
					return -1;
				}
				if(!shdr->sh_size)
				{
					printf("MULTIBOOT: section %i has no defined "
							"size\n", i);
					return -1;
				}

				if(!chunk_get_chunk(shdr->sh_addr, shdr->sh_size))
				{
					printf("MULTIBOOT: unable to allocate a chunk "
						"between 0x%08x and 0x%08x for section %i\n",
						shdr->sh_addr, shdr->sh_addr + shdr->sh_size,
						i);
					return -1;
				}

				// Now load or zero it
				if(shdr->sh_type == SHT_NOBITS)
					memset((void*)shdr->sh_addr, 0, shdr->sh_size);
				else
				{
					if(!shdr->sh_offset)
					{
						printf("MULTIBOOT: section %i has no defined "
								"file offset\n", i);
						return -1;
					}

					fseek(fp, (long)shdr->sh_offset, SEEK_SET);
					size_t bytes_to_read = (size_t)shdr->sh_size;
					size_t bytes_read = fread((void *)shdr->sh_addr,
							1, bytes_to_read, fp);
					if(bytes_to_read != bytes_read)
					{
						printf("MULTIBOOT: error loading section %i "
								"of size %i - only %i bytes "
								"loaded\n", i, bytes_to_read,
								bytes_read);
						return -1;
					}
				}
			}
		}


		for(unsigned int i = 0; i < ehdr->e_shnum; i++)
		{
			Elf32_Shdr *shdr = (Elf32_Shdr *)&sh_buf[i * ehdr->e_shentsize];

			if(!(shdr->sh_flags & SHF_ALLOC))
			{
#ifdef DEBUG
				printf("MULTIBOOT: section %i is not loadable\n", i);
#endif

				if(shdr->sh_size)
				{
					uint32_t load_addr = chunk_get_any_chunk(shdr->sh_size);

					if(!load_addr)
					{
						printf("MULTIBOOT: unable to allocate chunk of "
								"size %i for section %i\n",
								shdr->sh_size, i);
						return -1;
					}

					if(shdr->sh_type == SHT_NOBITS)
						memset((void*)load_addr, 0, shdr->sh_size);
					else
					{
						if(!shdr->sh_offset)
						{
							printf("MULTIBOOT: section %i has no defined "
									"file offset\n", i);
							return -1;
						}

						fseek(fp, (long)shdr->sh_offset, SEEK_SET);
						size_t bytes_to_read = (size_t)shdr->sh_size;
						size_t bytes_read = fread((void *)shdr->sh_addr,
								1, bytes_to_read, fp);
						if(bytes_to_read != bytes_read)
						{
							printf("MULTIBOOT: error loading section %i "
									"of size %i - only %i bytes "
									"loaded\n", i, bytes_to_read,
									bytes_read);
							return -1;
						}
					}

					// Store the address we've loaded to
					shdr->sh_addr = load_addr;
				}
			}
		}

		// Set the ELF flags
		mbinfo->u.elf_sec.num = ehdr->e_shnum;
		mbinfo->u.elf_sec.size = ehdr->e_shentsize;
		mbinfo->u.elf_sec.addr = (uint32_t)sh_buf;
		mbinfo->u.elf_sec.shndx = ehdr->e_shstrndx;
		mbinfo->flags |= (1 << 6);

		entry_addr = ehdr->e_entry;

		free(ehdr);
	}

	// Set the cmd line
	mbinfo->cmdline = args;
	mbinfo->flags |= (1 << 2);

	// Set the boot device
	mbinfo->boot_device = fp->fs->parent->device_name;
	mbinfo->flags |= (1 << 1);

	// Set the boot loader name
	mbinfo->boot_loader_name = rpi_boot_name;
	mbinfo->flags |= (1 << 9);

	// Set the fb info
	mbinfo->fb_addr = (uint32_t)fb_get_framebuffer();
	mbinfo->fb_size = (fb_get_width() << 16) | (fb_get_height() & 0xffff);
	mbinfo->fb_pitch = fb_get_pitch();
	mbinfo->fb_depth = (fb_get_bpp() << 16) | (0x1);	// TODO: check pixel_order
	mbinfo->flags |= (1 << 11);

	printf("MULTIBOOT: loaded kernel %s\n", file);

	fclose(fp);

	return 0;
}

struct _module {
	uint32_t start;
	uint32_t end;
	char *name;
	struct _module *next;
};

struct _module *first_mod = (void*)0;
int mod_count = 0;

static void module_add(uint32_t start, uint32_t end, char *name)
{
	struct _module *m = (struct _module *)malloc(sizeof(struct module));
	m->start = start;
	m->end = end;
	m->name = name;
	m->next = first_mod;
	first_mod = m;
	mod_count++;
}

static void add_multiboot_modules()
{
	mbinfo->mods_count = mod_count;

	mbinfo->mods_addr = (uint32_t)malloc(16 * mod_count);
	struct _module *cur_mod = first_mod;
	for(int i = 0; i < mod_count; i++)
	{
		struct module *mmod = (struct module *)(mbinfo->mods_addr + i * 16);
		mmod->mod_start = cur_mod->start;
		mmod->mod_end = cur_mod->end;
		mmod->string = (uint32_t)cur_mod->name;
		mmod->reserved = 0;

		cur_mod = cur_mod->next;
	}
}

int method_module(char *args)
{
	// Load a module
	FILE *fp = fopen(args, "r");
	if(!fp)
	{
		printf("MODULE: cannot load file %s\n", args);
		return -1;
	}

	// Allocate a chunk for it
	uint32_t address = chunk_get_any_chunk((uint32_t)fp->len);
	if(!address)
	{
		printf("MODULE: unable to allocate a chunk of size %i for %s\n",
				fp->len, args);
		return -1;
	}

	// Load it
	size_t bytes_to_read = (size_t)fp->len;
	size_t bytes_read = fread((void*)address, 1, bytes_to_read, fp);
	fclose(fp);

	if(bytes_to_read != bytes_read)
	{
		printf("MODULE: error loading %s only %i out of %i bytes read\n",
				args, bytes_read, bytes_to_read);
		return -1;
	}

	module_add(address, address + (uint32_t)bytes_read, args);

	printf("MODULE: %s loaded\n", args);
	return 0;
}

int method_boot(char *args)
{
#ifdef DEBUG
	printf("Interpreting boot command\n");
#endif
	(void)args;

	if(entry_addr == 0)
	{
		printf("BOOT: no valid kernel loaded\n");
		return -1;
	}

	if(mbinfo)
	{
		add_multiboot_modules();

		// Do a multiboot load
		printf("BOOT: multiboot load\n");

		void (*e_point)(uint32_t, uint32_t, uint32_t, uint32_t) = 
			(void(*)(uint32_t, uint32_t, uint32_t, uint32_t))entry_addr;
		e_point(MULTIBOOT_BOOTLOADER_MAGIC, (uint32_t)mbinfo,
				_arm_m_type, (uint32_t)&funcs);
	}
	else
	{
		// Do a simple jump
		printf("BOOT: non-multiboot load\n");
		
		void (*e_point)() = (void(*)())entry_addr;
		e_point();
	}
	return 0;
}

void atag_cb(struct atag *tag)
{
	if(tag->hdr.tag == ATAG_MEM)
	{
		uint32_t start = tag->u.mem.start;
		uint32_t size = tag->u.mem.size;
		
		// mem_upper is number of kiB beyond 1 MiB
		if(start == 0)
			mbinfo->mem_upper = (size / 1024) - 1024;

		mbinfo->mmap_length += 24;
	}
}

void atag_cb2(struct atag *tag)
{
	if(tag->hdr.tag == ATAG_MEM)
	{
		uint32_t start = tag->u.mem.start;
		uint32_t size = tag->u.mem.size;

		mmap_ptr[0] = 24;	// size of the tag
		mmap_ptr[1] = start;	// base addr
		mmap_ptr[2] = 0;	// upper 32 bits of base addr
		mmap_ptr[3] = size;	// length
		mmap_ptr[4] = 0;	// upper 32 bits of length
		mmap_ptr[5] = 1;	// available

		mmap_ptr += 6;		// skip to next
	}
}

