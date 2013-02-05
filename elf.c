/* Copyright (C) 2013 by John Cronin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

int elf32_read_ehdr(FILE *fp, Elf32_Ehdr **ehdr)
{
	*ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
	fseek(fp, 0, SEEK_SET);
	size_t bytes_to_read = sizeof(Elf32_Ehdr);
	size_t bytes_read = fread(*ehdr, 1, sizeof(Elf32_Ehdr), fp);
	if(bytes_to_read != bytes_read)
	{
		free(*ehdr);
		return ELF_FILE_LOAD_ERROR;
	}

	// Confirm its an ELF file
	if(((*ehdr)->e_ident[0] != 0x7f) || ((*ehdr)->e_ident[1] != 'E') ||
			((*ehdr)->e_ident[2] != 'L') ||
			((*ehdr)->e_ident[3] != 'F'))
	{
		free(*ehdr);
		return ELF_NOT_ELF;
	}

	// Confirm its a 32 bit file
	if((*ehdr)->e_ident[EI_CLASS] != ELFCLASS32)
	{
		free(*ehdr);
		return ELF_NOT_32_BIT;
	}

	// Confirm its a little-endian file
	if((*ehdr)->e_ident[EI_DATA] != ELFDATA2LSB)
	{
		free(*ehdr);
		return ELF_NOT_LITTLE_ENDIAN;
	}

	// Confirm its an executable file
	if((*ehdr)->e_type != ET_EXEC)
	{
		free(*ehdr);
		return ELF_NOT_EXEC;
	}

	// Confirm its for the ARM architecture
	if((*ehdr)->e_machine != EM_ARM)
	{
		free(*ehdr);
		return ELF_NOT_ARM;
	}
	return ELF_OK;
}

int elf32_read_shdrs(FILE *fp, Elf32_Ehdr *ehdr, uint8_t **shdrs)
{
	size_t bytes_to_load = (size_t)(ehdr->e_shentsize * ehdr->e_shnum);
	fseek(fp, (long)ehdr->e_shoff, SEEK_SET);
	*shdrs = (uint8_t *)malloc(bytes_to_load);
	size_t bytes_read = fread(*shdrs, 1, bytes_to_load, fp);
	if(bytes_read != bytes_to_load)
	{
		free(*shdrs);
		return ELF_FILE_LOAD_ERROR;
	}
	return ELF_OK;
}

int elf32_load_section(FILE *fp, Elf32_Shdr *shdr)
{
	if(shdr->sh_type == SHT_NOBITS)
		memset((void*)shdr->sh_addr, 0, shdr->sh_size);
	else
	{
		if(!shdr->sh_offset)
			return ELF_NO_OFFSET;

		fseek(fp, (long)shdr->sh_offset, SEEK_SET);
		size_t bytes_to_read = (size_t)shdr->sh_size;
		size_t bytes_read = fread((void *)shdr->sh_addr,
				1, bytes_to_read, fp);
		if(bytes_to_read != bytes_read)
			return ELF_FILE_LOAD_ERROR;
	}
	return ELF_OK;
}

int elf32_read_phdrs(FILE *fp, Elf32_Ehdr *ehdr, uint8_t **phdrs)
{
	(void)fp;
	(void)ehdr;
	(void)phdrs;
	return -1;
}

int elf32_load_segment(FILE *fp, Elf32_Phdr *phdr)
{
	(void)fp;
	(void)phdr;
	return -1;
}

