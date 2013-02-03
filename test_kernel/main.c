#include <stdint.h>
#include "../multiboot.h"

void kmain(uint32_t magic, multiboot_header_t *mbd, uint32_t m_type,
		struct multiboot_arm_functions *funcs)
{
/*	extern uint32_t magic;
	extern multiboot_header_t *mbd;
	extern struct multiboot_arm_functions *funcs;
	extern uint32_t m_type;*/

	funcs->clear();
	funcs->printf("Welcome to the test kernel\n");
	funcs->printf("Multiboot magic: %x\n", magic);
	funcs->printf("Running on machine type: %x\n", m_type);
}

