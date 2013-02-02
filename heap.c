#include <stdint.h>
#include <stdio.h>

#define MAX_BRK 0x100000

extern char _end;

uint32_t cur_brk = 0;

void *sbrk(uint32_t increment)
{
	// sbrk returns the previous brk value
	
	// First set up cur_brk if not done already
	if(cur_brk == 0)
	{
		printf("HEAP: initializing at %x\n", (uint32_t)&_end);

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

