.global loader

.set ALIGN,	1<<0
.set MEMINFO,	1<<1
.set FLAGS,	ALIGN | MEMINFO
.set MAGIC,	0x1BADB002
.set CHECKSUM,	-(MAGIC + FLAGS)

mboot_header:
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text

stack_bottom:
.skip 1024
stack_top:
.comm mbd, 4
.comm magic, 4
.comm m_type, 4
.comm funcs, 4

loader:
//	ldr	r5, =magic
//	str	r0, [r5]

//	ldr	r5, =mbd
//	str	r1, [r5]

//	ldr	r5, =m_type
//	str	r2, [r5]

//	ldr	r5, =m_type
//	str	r3, [r5]

	ldr	sp, =stack_top

	ldr	r4, =kmain
	blx	r4

halt:
	wfe
	b halt

