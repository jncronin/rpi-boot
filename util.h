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

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h>

uint32_t read_word(uint8_t *buf, int offset);
uint16_t read_halfword(uint8_t *buf, int offset);
uint8_t read_byte(uint8_t *buf, int offset);
void write_word(uint32_t val, uint8_t *buf, int offset);
void write_halfword(uint16_t val, uint8_t *buf, int offset);
void write_byte(uint8_t val, uint8_t *buf, int offset);

void *quick_memcpy(void *dest, void *src, size_t n);
void *qmemcpy(void *dest, void *src, size_t n);

uintptr_t alloc_buf(size_t size);

static inline uint32_t byte_swap(uint32_t v) {
  uint32_t result;
#ifdef __arm__
	__asm__ __volatile__ ("rev %0, %1" : "=r" (result) : "r" (v));
#else
	result = (v >> 24) | (v << 24) | ((v&0xff00) << 8) | ((v&0xff0000) >> 8);
#endif
	return result;
}

#endif

