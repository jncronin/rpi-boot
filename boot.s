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

// The following is designed to ensure a v2 pi is not in hyp mode
// It does nothing on v1
Start:
    mrs     r3,cpsr                     // get the current program status register
    and     r3,#0x1f                    // and mask out the mode bits
    cmp     r3,#0x1a                    // are we in hyp mode?
    beq     hyp                         // if we are in hyp mode, go to that section
    cpsid   iaf,#0x13                   // if not switch to svc mode, ensure we have a stack for the kernel; no ints
    b       cont                        // and then jump to set up the stack

// -- from here we are in hyp mode so we need to exception return to the svc mode
hyp:
    mrs     r3,cpsr                     // get the cpsr again
    and     r3,#~0x1f                   // clear the mode bits
    orr     r3,#0x013                   // set the mode for svc
    orr     r3,#1<<6|1<<7|1<<8          // disable interrupts as well
    msr     spsr_cxsf,r3                // and save that in the spsr

    ldr     r3,=cont                    // get the address where we continue
    msr     elr_hyp,r3                  // store that in the elr register

    eret                                // this is an exception return

// -- everyone continues from here
cont:
	mov sp, #0x8000

	/* Clear out bss */
	ldr r4, =_bss_start
	ldr r9, =_bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0

	b .test

.loop:
	/* this does 4x4 = 16 byte stores at once */
	stmia r4!, {r5-r8}	/* the '!' increments r4 but only after ('ia') the store */
.test:
	cmp r4, r9
	blo .loop

	/* branch and link to kernel_main */
	ldr r3, =kernel_main
	blx r3		/* blx may switch to Thumb mode, depending on the target address */

halt:
	wfe		/* equivalent of x86 HLT instruction */
	b halt

.globl flush_cache
flush_cache:
	mov 	r0, #0
	mcr	p15, #0, r0, c7, c14, #0
	mov	pc, lr

.globl memory_barrier
memory_barrier:
	mov	r0, #0
	mcr	p15, #0, r0, c7, c10, #5
	mov	pc, lr

.globl read_sctlr
read_sctlr:
	mrc	p15, #0, r0, c1, c0, #0
	mov	pc, lr

.globl quick_memcpy
quick_memcpy:
	push 	{r4-r9}
	mov	r4, r0
	mov	r5, r1

.loopb:
	ldmia	r5!, {r6-r9}
	stmia	r4!, {r6-r9}
	subs	r2, #16
	bhi	.loopb

	pop	{r4-r9}
	mov	pc, lr

