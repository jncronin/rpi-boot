/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
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

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "fb.h"
#include "console.h"
#include "util.h"

extern uint8_t vgafont8[];

extern void memory_barrier();
extern uint32_t read_sctlr();

#define CHAR_W		8
#define CHAR_H		8

#define DEF_FORE	0xffffffff
#define DEF_BACK	0x00000000

static int cur_x = 0;
static int cur_y = 0;

static uint32_t cur_fore = DEF_FORE;
static uint32_t cur_back = DEF_BACK;

void clear()
{
	int height = fb_get_height();
	int pitch = fb_get_pitch();
	int line_byte_width = fb_get_width() * (fb_get_bpp() >> 3);
	uint8_t *fb = (uint8_t *)fb_get_framebuffer();

	for(int line = 0; line < height; line++)
		memset(&fb[line * pitch], 0, line_byte_width);

	cur_x = 0;
	cur_y = 0;
}

void newline()
{
	cur_y++;
	cur_x = 0;

	// Scroll up if necessary
	if(cur_y == fb_get_height() / CHAR_H)
	{
		uint8_t *fb = (uint8_t *)fb_get_framebuffer();
		int line_byte_width = fb_get_width() * (fb_get_bpp() >> 3);
		int pitch = fb_get_pitch();
		int height = fb_get_height();

		for(int line = 0; line < (height - CHAR_H); line++)
			quick_memcpy(&fb[line * pitch], &fb[(line + CHAR_H) * pitch], line_byte_width);
		for(int line = height - CHAR_H; line < height; line++)
			memset(&fb[line * pitch], 0, line_byte_width);

		cur_y--;
	}
}

int console_putc(int c)
{
	int line_w = fb_get_width() / CHAR_W;

	if(c == '\n')
		newline();
	else
	{
		draw_char((char)c, cur_x, cur_y, cur_fore, cur_back);
		cur_x++;
		if(cur_x == line_w)
			newline();
	}

	return c;
}

void draw_char(char c, int x, int y, uint32_t fore, uint32_t back)
{
	volatile uint8_t *fb = (uint8_t *)fb_get_framebuffer();
	int bpp = fb_get_bpp();
	int bytes_per_pixel = bpp >> 3;

	int d_offset = y * CHAR_H * fb_get_pitch() + x * bytes_per_pixel * CHAR_W;
	int line_d_offset = d_offset;
	int s_offset = (int)c * CHAR_W * CHAR_H;

	for(int c_y = 0; c_y < CHAR_H; c_y++)
	{
		d_offset = line_d_offset;

		for(int c_x = 0; c_x < CHAR_W; c_x++)
		{
			int s_byte_no = s_offset / 8;
			int s_bit_no = s_offset % 8;

			uint8_t s_byte = vgafont8[s_byte_no];
			uint32_t colour = back;
			if((s_byte >> (7 - s_bit_no)) & 0x1)
				colour = fore;

			for(int i = 0; i < bytes_per_pixel; i++)
			{
				fb[d_offset + i] = (uint8_t)(colour & 0xff);
				colour >>= 8;
			}	

			d_offset += bytes_per_pixel;
			s_offset++;
		}

		line_d_offset += fb_get_pitch();
	}
}

int fb_test(uint32_t fb_addr)
{
	volatile uint8_t *bb = (uint8_t *)fb_addr;

	// dump fb location
	puts("FB addr:");
	puthex(fb_addr);
	puts("");
	puts("");

	// clear to white
	//memset((uint8_t *)bb, 0x0f, fb_get_byte_size());
	//memory_barrier();

	bb[0] = 0x12;
	//bb[1] = 0x23;
	//bb[2] = 0x34;
	//bb[3] = 0x45;
	//bb[1000] = 0x78;

	uint8_t colour = 0;
	for(int i = 0; i < fb_get_byte_size(); i++)
		bb[i] = colour++;

	return 0;
}

int console_test()
{
	uint32_t fb_addr = (uint32_t)fb_get_framebuffer();

	puts("Console test, fb_addr =");
	puthex(fb_addr);
	puts("");

	fb_test(fb_addr);
	//fb_test(fb_addr + 0x40000000);
	//fb_test(fb_addr + 0x80000000);
	//fb_test(fb_addr + 0xc0000000);

	return 0;
}

