// Functions for allocating chunks of large memory

#ifndef MEMCHUNK_H
#define MEMCHUNK_H

#include <stdint.h>

void chunk_register_free(uint32_t start, uint32_t length);
uint32_t chunk_get_any_chunk(uint32_t length);
uint32_t chunk_get_chunk(uint32_t start, uint32_t length);

#endif

