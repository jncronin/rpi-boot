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

.section ".text.boot"

.globl Start

Start:
	adr	x20, stack + (1 << 15)
	mov	sp, x20

	/* Clear out bss */
	ldr	x4, =_bss_start
	ldr	x9, =_bss_end
	mov	x5, #0
	mov	x6, #0

	b	.test

.loop:
	/* this does 2x8 = 16 byte stores at once */
	stp	x5, x6, [x4, #0]
	add	x4, x4, #16
.test:
	cmp	x4, x9
	blo	.loop

	/* branch and link to kernel_main */
	bl	kernel_main

halt:
	wfe		/* equivalent of x86 HLT instruction */
	b	halt

.globl flush_cache
flush_cache:
	mov 	x0, #0
	/* XXX, not used in QEMU */
	ret

.globl memory_barrier
memory_barrier:
	mov	x0, #0
	dmb	sy
	ret

.globl read_sctlr
read_sctlr:
	mrs	x0, sctlr_el2
	ret

.globl quick_memcpy
quick_memcpy:
	mov	x10, x0
	mov	x11, x1

.loopb:
	ldp	x12, x13, [x11, #0]
	add	x11, x11, #16
	stp	x12, x13, [x10, #0]
	add	x10, x10, #16
	sub	x2, x2, #16
	cmp	x2, #0
	bhi	.loopb

	ret

.section ".bss"
	.align 4		/* Align on 128bit boundary */
	stack:
	.align 15		/* Reserve 32kb of stack */

