#include <stdint.h>
#include "mmio.h"

extern void memory_barrier();

inline void mmio_write(uint32_t reg, uint32_t data)
{
	memory_barrier();
	*(volatile uint32_t *)(reg) = data;
	memory_barrier();
}

inline uint32_t mmio_read(uint32_t reg)
{
	memory_barrier();
	return *(volatile uint32_t *)(reg);
	memory_barrier();
}

