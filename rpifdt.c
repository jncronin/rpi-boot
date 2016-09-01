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

static uint32_t get_adjusted_address(const char *dtb, int node, uint32_t addr);

void emmc_set_base(uint32_t base);
void uart_set_base(uint32_t base);
void gpio_set_base(uint32_t base);
void mbox_set_base(uint32_t base);
void timer_set_base(uint32_t base);

void uart_init();

static void parse_reg(const char *dtb, int node, int parent,
		void (*cb)(uint32_t addr, uint32_t length))
{
	// get property
	int plen;
	const void *regp = fdt_getprop(dtb, node, "reg", &plen);

	// try and get #address-cells and #size-cells of parent
	int ac_len, sc_len;
	const void *ac = fdt_getprop(dtb, parent, "#address-cells", &ac_len);
	const void *sc = fdt_getprop(dtb, parent, "#size-cells", &sc_len);

	if(ac && sc && regp)
	{
		int acs = read_wordbe(ac, 0);
		int scs = read_wordbe(sc, 0);

#ifdef DTB_DEBUG
		printf("DTB: valid address cells (%i: %i) and size cells (%i: %i)\n",
			acs, ac_len, scs, sc_len);
#endif

		int idx = 0;
		while(idx < plen)
		{
			uint32_t addr = 0;
			uint32_t len = 0;

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

			uint32_t adj_addr = get_adjusted_address(dtb, node, addr);
#ifdef DEBUG_DTB
			printf("DTB: address: %x (%x), length: %x\n", addr, adj_addr, len);
#endif
			cb(adj_addr, len);
		}
	}
}

static void parse_node(const char *dtb, int node,
		void (*mem_cb)(uint32_t addr, uint32_t length))
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
			parse_reg(dtb, node, parent, mem_cb);
		}

		parse_node(dtb, node, mem_cb);
		
		node = fdt_next_subnode(dtb, node);
	}
}

/* Find a device by matching its compatible property */
static void parse_dtb_compatible_helper(const char *dtb, const char *compat, int node,
		void (*cb)(const char *dtb, int node, int parent))
{
	int parent = node;
	node = fdt_first_subnode(dtb, node);
	while(node >= 0)
	{
		const char *haystack = fdt_getprop(dtb, node, "compatible", NULL);
		if(compat)
		{
			if(strstr(haystack, compat))
			{
				cb(dtb, node, parent);
			}
		}

		parse_dtb_compatible_helper(dtb, compat, node, cb);

		node = fdt_next_subnode(dtb, node);
	}
}

static void parse_dtb_compatible(const char *dtb, const char *compat,
		void (*cb)(const char *dtb, int node, int parent))
{
	int root_offset = fdt_path_offset(dtb, "/");
	parse_dtb_compatible_helper(dtb, compat, root_offset, cb);
}


struct soc_range
{
	int soc_node;
	uint32_t child_addr;
	uint32_t parent_addr;
	uint32_t length;
};

static struct soc_range *sr = NULL;
static int sr_items = 0;

static uint32_t get_adjusted_address_int(const char *dtb, int node, uint32_t addr)
{
	// iterate through the sr array, looking for a region where:
	// 	sr[n].soc_node is a parent of node
	// 	addr is within the required range
	// then call self with return addr - child_addr + parent_addr
	// or just addr if nothing found
	
	if(node == fdt_path_offset(dtb, "/"))
		return addr;

	for(int i = 0; i < sr_items; i++)
	{
		if(sr[i].soc_node == node)
		{
			if(addr >= sr[i].child_addr &&
					addr < (sr[i].child_addr + sr[i].length))
			{
				addr = addr - sr[i].child_addr + sr[i].parent_addr;
				break;
			}
		}
	}

	node = fdt_parent_offset(dtb, node);
	if(node >= 0)
		return get_adjusted_address_int(dtb, node, addr);
	else
		return addr;
}

static uint32_t get_adjusted_address(const char *dtb, int node, uint32_t addr)
{
	node = fdt_parent_offset(dtb, node);
	if(node >= 0)
		return get_adjusted_address_int(dtb, node, addr);
	else
		return addr;
}

static void load_ranges(const char *dtb)
{
	// First iterate through to count the total number of simple-bus
	//  items
	int soc = fdt_node_offset_by_compatible(dtb, -1, "simple-bus");
	sr_items = 0;
	while(soc != -FDT_ERR_NOTFOUND)
	{
		int ac = fdt_address_cells(dtb, soc);
		int sc = fdt_size_cells(dtb, soc);
		int ranges_plen;
		const void *ranges = fdt_getprop(dtb, soc, "ranges", &ranges_plen);
		if(ranges)
			sr_items += ranges_plen / ((2 * ac + sc) * 4);
		soc = fdt_node_offset_by_compatible(dtb, soc, "simple-bus");
	}

	if(sr)
		free(sr);
	sr = (struct soc_range *)malloc(sr_items * sizeof(struct soc_range));

	// Now re-iterate to get the actual values
	int cur_sr_item = 0;
	soc = fdt_node_offset_by_compatible(dtb, -1, "simple-bus");
	while(soc != -FDT_ERR_NOTFOUND)
	{
		int ac = fdt_address_cells(dtb, soc);
		int sc = fdt_size_cells(dtb, soc);
		int ranges_plen;
		const void *ranges = fdt_getprop(dtb, soc, "ranges", &ranges_plen);
		if(ranges)
		{
#ifdef DEBUG	
			printf("DTB: soc %i, ac %i, sc %i, plen %i\n", soc, ac, sc, ranges_plen);
#endif

			int num_ranges = ranges_plen / ((2 * ac + sc) * 4);

			int offset = 0;
			// if more than one 4-byte word is specified for address
			// we take the last one only (DTB is big-endian)
			for(int i = 0; i < num_ranges; i++, cur_sr_item++)
			{
				uint32_t cur_child = 0;
				uint32_t cur_parent = 0;
				uint32_t cur_len = 0;
				for(int j = 0; j < ac; j++)
				{
					cur_child = read_wordbe(ranges, offset);
					offset += 4;
				}
				for(int j = 0; j < ac; j++)
				{
					cur_parent = read_wordbe(ranges, offset);
					offset += 4;
				}
				for(int j = 0; j < sc; j++)
				{
					cur_len = read_wordbe(ranges, offset);
					offset += 4;
				}

#ifdef DEBUG
				printf("DTB: ranges: child: %x, parent: %x; len: %x\n",
					cur_child, cur_parent, cur_len);
#endif

				sr[cur_sr_item].soc_node = soc;
				sr[cur_sr_item].child_addr = cur_child;
				sr[cur_sr_item].parent_addr = cur_parent;
				sr[cur_sr_item].length = cur_len;
			}
		}
		soc = fdt_node_offset_by_compatible(dtb, soc, "simple-bus");
	}
}

static void dump_cb(uint32_t addr, uint32_t len)
{
	printf("addr: %x, len: %x  ", addr, len);
}

static void mmc_reg_cb(uint32_t addr, uint32_t len)
{
	(void)len;
	emmc_set_base(addr);
}

static void uart_reg_cb(uint32_t addr, uint32_t len)
{
	(void)len;
	uart_set_base(addr);
}

static void gpio_reg_cb(uint32_t addr, uint32_t len)
{
	(void)len;
	gpio_set_base(addr);
}

static void mbox_reg_cb(uint32_t addr, uint32_t len)
{
	(void)len;
	mbox_set_base(addr);
}

static void timer_reg_cb(uint32_t addr, uint32_t len)
{
	(void)len;
	timer_set_base(addr);
}

static void mmc_cb(const char *dtb, int node, int parent)
{
#ifdef DEBUG
	printf("DTB: found MMC device: ");
	parse_reg(dtb, node, parent, dump_cb);
	printf("\n");
#endif
	parse_reg(dtb, node, parent, mmc_reg_cb);
}

static void uart_cb(const char *dtb, int node, int parent)
{
#ifdef DEBUG
	printf("DTB: found UART device: ");
	parse_reg(dtb, node, parent, dump_cb);
	printf("\n");
#endif
	parse_reg(dtb, node, parent, uart_reg_cb);
}

static void gpio_cb(const char *dtb, int node, int parent)
{
#ifdef DEBUG
	printf("DTB: found GPIO device: ");
	parse_reg(dtb, node, parent, dump_cb);
	printf("\n");
#endif
	parse_reg(dtb, node, parent, gpio_reg_cb);
}

static void mbox_cb(const char *dtb, int node, int parent)
{
#ifdef DEBUG
	printf("DTB: found mbox device: ");
	parse_reg(dtb, node, parent, dump_cb);
	printf("\n");
#endif
	parse_reg(dtb, node, parent, mbox_reg_cb);
}

static void timer_cb(const char *dtb, int node, int parent)
{
#ifdef DEBUG
	printf("DTB: found timer device: ");
	parse_reg(dtb, node, parent, dump_cb);
	printf("\n");
#endif
	parse_reg(dtb, node, parent, timer_reg_cb);
}

void parse_dtb_devices(const char *dtb)
{
	/*int root_offset = fdt_path_offset(dtb, "/");

	int soc = fdt_path_offset(dtb, "/soc");
	int mmc = fdt_path_offset(dtb, "/soc/mmc");
	int mbox = fdt_path_offset(dtb, "/soc/mbox");
	int uart = fdt_path_offset(dtb, "/soc/uart"); */

	parse_dtb_compatible(dtb, "pl011", uart_cb);
	parse_dtb_compatible(dtb, "bcm2835-gpio", gpio_cb);
	parse_dtb_compatible(dtb, "bcm2835-system-timer", timer_cb);
	uart_init();
	parse_dtb_compatible(dtb, "bcm2835-mmc", mmc_cb);
	parse_dtb_compatible(dtb, "bcm2835-mbox", mbox_cb);
}

void dump_tree(const char *dtb, int node, int parent, int indent)
{
	const char *node_name = fdt_get_name(dtb, node, NULL);
	const char *compat = fdt_getprop(dtb, node, "compatible", NULL);
	
	if(node_name)
	{
		for(int i = 0; i < indent; i++)
			printf("  ");
		printf(node_name);
		if(compat)
			printf("(%s)", compat);
		printf(" ");
		parse_reg(dtb, node, parent, dump_cb);

		printf("\n");
	}

	int subnode;
	fdt_for_each_subnode(subnode, dtb, node)
		dump_tree(dtb, subnode, node, indent + 1);
}


void parse_dtb(const char *dtb, void (*mem_cb)(uint32_t start, uint32_t len))
{
	static int runonce = 0;

	if(runonce == 0)
	{
		load_ranges(dtb);
		parse_dtb_devices(dtb);
	}

	int root_offset = fdt_path_offset(dtb, "/");

	int chosen_offset = fdt_path_offset(dtb, "/chosen");
	const char *bootargs = fdt_getprop(dtb, chosen_offset, "bootargs", NULL);

#ifdef DEBUG
	if(runonce == 0)
	{
		const char *model = fdt_getprop(dtb, root_offset, "model", NULL);
		const char *compatible = fdt_getprop(dtb, root_offset, "compatible", NULL);
	
		printf("DTB: model: %s, compatible: %s\n", model, compatible);
		printf("DTB: bootargs: %s\n", bootargs);
		printf("DTB: Tree:\n");
		dump_tree(dtb, root_offset, 0, 0);
	}
#endif

	atag_cmd_line = bootargs;

	parse_node(dtb, root_offset, mem_cb);

	runonce = 1;
}

