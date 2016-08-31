/* Copyright (C) 2016 by John Cronin <jncronin@tysos.org>
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

#include <libfdt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "util.h"

extern const char *atag_cmd_line;
extern uint32_t base_adjust;

static void parse_node(const char *dtb, int node, void (*mem_cb)(uint32_t addr, uint32_t length))
{
	int parent = node;
	node = fdt_first_subnode(dtb, node);
	while(node >= 0)
	{
		const void *dt;
		int plen;

		dt = fdt_getprop(dtb, node, "device_type", &plen);

#ifdef DEBUG_DTB
		const char *nn;
		nn = fdt_get_name(dtb, node, NULL);
		if(dt != NULL && nn != NULL)
		{
			printf("DTB: found device_type property for %s: %s\n", nn, (const char *)dt);
		}
		else if(nn != NULL)
			printf("DTB: found node %s\n", nn);
		else
			printf("DTB: unknown node\n");
#endif
		if(dt != NULL && !strcmp((const char *)dt, "memory"))
		{
			const struct fdt_property *regp;
			regp = fdt_getprop(dtb, node, "reg", &plen);
	
			if(regp != NULL)
			{
#ifdef DEBUG_DTB
				printf("DTB: reg length %i\n", plen);
#endif
	
				// try and get #address-cells and #size-cells of parent
				int ac_len, sc_len;
				const void *ac = fdt_getprop(dtb, parent, "#address-cells", &ac_len);
				const void *sc = fdt_getprop(dtb, parent, "#size-cells", &sc_len);

				if(ac != NULL && sc != NULL)
				{
					int acs = read_wordbe(ac, 0);
					int scs = read_wordbe(sc, 0);

#ifdef DTB_DEBUG
					printf("DTB: valid address cells (%i: %i) and size cells (%i: %i)\n",
						acs, ac_len, scs, sc_len);
#endif

					int idx = 0;
					uint32_t addr = 0;
					uint32_t len = 0;
					while(idx < plen)
					{
						// always take the most significant 4 bytes
						for(int i = 0; i < acs; i++)
						{
							addr = read_wordbe((const void *)regp, idx);
							idx += 4;
						}
						for(int i = 0; i < scs; i++)
						{
							len = read_wordbe((const void *)regp, idx);
							idx += 4;
						}
					}
#ifdef DEBUG_DTB
					printf("DTB: address: %x, length: %x\n", addr, len);
#endif
					mem_cb(addr, len);
				}
			}
		}

		parse_node(dtb, node, mem_cb);
		
		node = fdt_next_subnode(dtb, node);
	}
}

void parse_dtb(const char *dtb, void (*mem_cb)(uint32_t start, uint32_t len))
{
#ifdef DEBUG
	static int debug_disp = 0;
#endif

	int root_offset = fdt_path_offset(dtb, "/");

	const char *model = fdt_getprop(dtb, root_offset, "model", NULL);
	const char *compatible = fdt_getprop(dtb, root_offset, "compatible", NULL);
	
	int chosen_offset = fdt_path_offset(dtb, "/chosen");
	const char *bootargs = fdt_getprop(dtb, chosen_offset, "bootargs", NULL);

#ifdef DEBUG
	if(debug_disp == 0)
	{
		printf("DTB: model: %s, compatible: %s\n", model, compatible);
		printf("DTB: bootargs: %s\n", bootargs);
	}
#endif

	if(strstr(compatible, "bcm2709"))
	{
		base_adjust = BASE_ADJUST_V2;
#ifdef DEBUG
		if(debug_disp == 0)
		{
			printf("DTB: detected model 2 - using %x for base_adjust\n",
					base_adjust);
		}
#endif
	}
	// TODO need base_adjust for v3


	atag_cmd_line = bootargs;

	parse_node(dtb, root_offset, mem_cb);

#ifdef DEBUG
	debug_disp = 1;
#endif
}

