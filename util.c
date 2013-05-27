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
#include "util.h"

// Support for unaligned data access

void write_word(uint32_t val, uint8_t *buf, int offset)
{
    buf[offset + 0] = val & 0xff;
    buf[offset + 1] = (val >> 8) & 0xff;
    buf[offset + 2] = (val >> 16) & 0xff;
    buf[offset + 3] = (val >> 24) & 0xff;
}

void write_halfword(uint16_t val, uint8_t *buf, int offset)
{
    buf[offset + 0] = val & 0xff;
    buf[offset + 1] = (val >> 8) & 0xff;
}

void write_byte(uint8_t byte, uint8_t *buf, int offset)
{
    buf[offset] = byte;
}

uint32_t read_word(uint8_t *buf, int offset)
{
	uint32_t b0 = buf[offset + 0] & 0xff;
	uint32_t b1 = buf[offset + 1] & 0xff;
	uint32_t b2 = buf[offset + 2] & 0xff;
	uint32_t b3 = buf[offset + 3] & 0xff;

	return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

uint16_t read_halfword(uint8_t *buf, int offset)
{
	uint16_t b0 = buf[offset + 0] & 0xff;
	uint16_t b1 = buf[offset + 1] & 0xff;

	return b0 | (b1 << 8);
}

uint8_t read_byte(uint8_t *buf, int offset)
{
	return buf[offset];
}

// Support for BE to LE conversion
uint32_t byte_swap(uint32_t in)
{
    uint32_t b0 = in & 0xff;
    uint32_t b1 = (in >> 8) & 0xff;
    uint32_t b2 = (in >> 16) & 0xff;
    uint32_t b3 = (in >> 24) & 0xff;
    uint32_t ret = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    return ret;
}

