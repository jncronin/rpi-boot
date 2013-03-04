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
#include <stdio.h>

#include "dwc_usb.h"

#define MAX_BRK DWC_USB_FIFO_START

extern char _end;

uint32_t cur_brk = 0;

void *sbrk(uint32_t increment)
{
	// sbrk returns the previous brk value
	
	// First set up cur_brk if not done already
	if(cur_brk == 0)
	{
//		printf("HEAP: initializing at %x\n", (uint32_t)&_end);

		cur_brk = (uint32_t)&_end;
		if(cur_brk & 0xfff)
		{
			cur_brk &= 0xfffff000;
			cur_brk += 0x1000;
		}
	}

	uint32_t old_brk = cur_brk;

	cur_brk += increment;
	if(cur_brk >= MAX_BRK)
	{
		cur_brk = old_brk;
		return (void*)-1;
	}
	return (void*)old_brk;	
}

