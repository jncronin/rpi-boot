#include <stdint.h>
#include "atag.h"

#define tag_next(t)		(struct atag *)((uint32_t *)(t) + (t)->hdr.size)

void parse_atags(uint32_t atags, void (*cb)(struct atag *))
{
	if(atags == 0)
		return;

	struct atag *cur = (struct atag *)atags;
	struct atag *prev;
	do
	{
		prev = cur;

		cb(cur);
		cur = tag_next(cur);
	} while(prev->hdr.tag != ATAG_NONE);
}

