#ifndef ATAG_H
#define ATAG_H

#include <stdint.h>

struct atag_header {
	uint32_t size;
	uint32_t tag;
};

#define ATAG_NONE		0
#define ATAG_CORE		0x54410001
#define ATAG_MEM		0x54410002
#define ATAG_VIDEOTEXT		0x54410003
#define ATAG_RAMDISK		0x54410004
#define ATAG_INITRD2		0x54420005
#define ATAG_SERIAL		0x54410006
#define ATAG_REVISION		0x54410007
#define ATAG_VIDEOLFB		0x54410008
#define ATAG_CMDLINE		0x54410009

struct atag_core {
        uint32_t flags;              /* bit 0 = read-only */
        uint32_t pagesize;           /* systems page size (usually 4k) */
        uint32_t rootdev;            /* root device number */
};

struct atag_mem {
        uint32_t     size;   /* size of the area */
        uint32_t     start;  /* physical start address */
};

struct atag_videotext {
        uint8_t              x;           /* width of display */
        uint8_t              y;           /* height of display */
        uint16_t             video_page;
        uint8_t              video_mode;
        uint8_t              video_cols;
        uint16_t             video_ega_bx;
        uint8_t              video_lines;
        uint8_t              video_isvga;
        uint16_t             video_points;
};

struct atag_ramdisk {
        uint32_t flags;      /* bit 0 = load, bit 1 = prompt */
        uint32_t size;       /* decompressed ramdisk size in _kilo_ bytes */
        uint32_t start;      /* starting block of floppy-based RAM disk image */
};

struct atag_initrd2 {
        uint32_t start;      /* physical start address */
        uint32_t size;       /* size of compressed ramdisk image in bytes */
};

struct atag_serialnr {
        uint32_t low;
        uint32_t high;
};

struct atag_revision {
        uint32_t rev;
};

struct atag_videolfb {
        uint16_t             lfb_width;
        uint16_t             lfb_height;
        uint16_t             lfb_depth;
        uint16_t             lfb_linelength;
        uint32_t             lfb_base;
        uint32_t             lfb_size;
        uint8_t              red_size;
        uint8_t              red_pos;
        uint8_t              green_size;
        uint8_t              green_pos;
        uint8_t              blue_size;
        uint8_t              blue_pos;
        uint8_t              rsvd_size;
        uint8_t              rsvd_pos;
};

struct atag_cmdline {
        char    cmdline[1];     /* this is the minimum size */
};

struct atag
{
	struct atag_header hdr;
	union {
		struct atag_core		core;
		struct atag_mem			mem;
		struct atag_videotext		videotext;
		struct atag_ramdisk		ramdisk;
		struct atag_initrd2		initrd2;
		struct atag_serialnr		serialnr;
		struct atag_revision		revision;
		struct atag_videolfb		videolfb;
		struct atag_cmdline		cmdline;
	} u;
};

void parse_atags(uint32_t atags, void (*callback_f)(struct atag *));

#endif

