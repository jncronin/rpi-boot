/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
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

#include "block.h"
#include "vfs.h"

#ifdef ENABLE_SD
int sd_card_init(struct block_device **dev);
#endif
#ifdef ENABLE_MBR
int read_mbr(struct block_device *, struct block_device ***, int *);
#endif
#ifdef ENABLE_USB
int dwc_usb_init(struct usb_hcd **dev, uint32_t base);
#endif
#ifdef ENABLE_RASPBOOTIN
int raspbootin_init(struct fs **fs);
#endif
#ifdef ENABLE_FAT
int fat_init(struct block_device *, struct fs **);
#endif
#ifdef ENABLE_EXT2
int ext2_init(struct block_device *, struct fs **);
#endif
#ifdef ENABLE_NOFS
int nofs_init(struct block_device *, struct fs **);
#endif

void libfs_init()
{
#ifdef ENABLE_USB
	struct usb_hcd *usb_hcd;
	dwc_usb_init(&usb_hcd, DWC_USB_BASE);
#endif

#ifdef ENABLE_SD
	struct block_device *sd_dev;
	if(sd_card_init(&sd_dev) == 0)
	{
#ifdef ENABLE_MBR
		read_mbr(sd_dev, (void*)0, (void*)0);
#endif
	}
#endif

#ifdef ENABLE_RASPBOOTIN
    struct fs *raspbootin_fs;
    if(raspbootin_init(&raspbootin_fs) == 0)
        vfs_register(raspbootin_fs);
#endif
}

int register_fs(struct block_device *dev, int part_id)
{
	switch(part_id)
	{
		case 0:
			// Try reading it as an mbr, then try all known filesystems
#ifdef ENABLE_MBR
			if(read_mbr(dev, NULL, NULL) == 0)
				break;
#endif
#ifdef ENABLE_FAT
			if(fat_init(dev, &dev->fs) == 0)
				break;
#endif
#ifdef ENABLE_EXT2
			if(ext2_init(dev, &dev->fs) == 0)
				break;
#endif
#ifdef ENABLE_NOFS
			if(nofs_init(dev, &dev->fs) == 0)
				break;
#endif
			break;

		case 1:
		case 4:
		case 6:
		case 0xb:
		case 0xc:
		case 0xe:
		case 0x11:
		case 0x14:
		case 0x1b:
		case 0x1c:
		case 0x1e:
#ifdef ENABLE_FAT
			fat_init(dev, &dev->fs);
#endif
			break;

		case 0x83:
#ifdef ENABLE_EXT2
			ext2_init(dev, &dev->fs);
#endif
			break;

		case 0xda:
#ifdef ENABLE_NOFS
			nofs_init(dev, &dev->fs);
#endif
			break;
	}

	if(dev->fs)
	{
		vfs_register(dev->fs);
		return 0;
	}
	else
		return -1;
}


