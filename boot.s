.section ".text.boot"

.globl Start

Start:
	mov sp, #0x8000

	// Clear out bss
	ldr r4, =_bss_start
	ldr r9, =_bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0

	b .test

.loop:
	// this does 4x4 = 16 byte stores at once
	stmia r4!, {r5-r8}	// the '!' increments r4 but only after ('ia') the store
.test:
	cmp r4, r9
	blo .loop

	// branch and link to kernel_main
	ldr r3, =kernel_main
	blx r3		// blx may switch to Thumb mode, depending on the target address

halt:
	wfe		// equivalent of x86 HLT instruction
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

