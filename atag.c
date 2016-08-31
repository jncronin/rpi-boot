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

#include <stdint.h>
#include "atag.h"
#include "rpifdt.h"

#define tag_next(t)		(struct atag *)((uint32_t *)(t) + (t)->hdr.size)

extern const char *atag_cmd_line;
extern int conf_source;
extern uint32_t _atags;

int check_atags(const void *atags)
{
	const struct atag *cur = (const struct atag *)atags;
	if(cur->hdr.tag == ATAG_CORE)
		return 0;
	return -1;
}

void parse_atags(uint32_t atags, void (*mem_cb)(uint32_t addr, uint32_t len))
{
	if(atags == 0)
		return;

	struct atag *cur = (struct atag *)atags;
	struct atag *prev;
	do
	{
		prev = cur;

		if(cur->hdr.tag == ATAG_MEM)
			mem_cb(cur->u.mem.start, cur->u.mem.size);
		else if(cur->hdr.tag == ATAG_CMDLINE)
			atag_cmd_line = &cur->u.cmdline.cmdline[0];

		cur = tag_next(cur);
	} while(prev->hdr.tag != ATAG_NONE);
}

void parse_atag_or_dtb(void (*mem_cb)(uint32_t addr, uint32_t len))
{
	switch(conf_source)
	{
		case 1:
			parse_atags(_atags, mem_cb);
			break;
		case 2:
			parse_atags(0, mem_cb);
			break;
		case 3:
			parse_dtb((const void *)_atags, mem_cb);
			break;
		case 4:
			parse_dtb((const void *)0, mem_cb);
			break;
	}
}

