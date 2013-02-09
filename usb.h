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

#ifndef USB_H
#define USB_H

#define USB_BASE			0x20980000
#define USB_OTG_CTRL			(USB_BASE + 0)
#define USB_OTG_IRPT			(USB_BASE + 4)
#define USB_AHB_CONF			(USB_BASE + 8)
#define USB_CORE_CONF			(USB_BASE + 0xC)
#define USB_CORE_RESET			(USB_BASE + 0x10)
#define USB_CORE_IRPT			(USB_BASE + 0x14)
#define USB_CORE_IRPT_MASK		(USB_BASE + 0x18)
#define USB_RECV_STATUS_DBG		(USB_BASE + 0x1C)
#define USB_STATUS_READ_POP		(USB_BASE + 0x1C)
#define USB_DEVICE_STATUS_READ_POP	(USB_BASE + 0x20)
#define USB_RECV_FIFO_SIZE		(USB_BASE + 0x24)
#define USB_NON_PERIODIC_FIFO_SIZE	(USB_BASE + 0x28)
#define USB_BON_PERIODIC_FIFO		(USB_BASE + 0x2C)
#define USB_I2C_ACCESS			(USB_BASE + 0x30)
#define USB_PHY_VENDOR_CONTROL		(USB_BASE + 0x34)
#define USB_GPIO			(USB_BASE + 0x38)
#define USB_USER_ID			(USB_BASE + 0x3C)
#define USB_SYNOPSYS_ID			(USB_BASE + 0x40)

#define USB_HOST_CONF			(USB_BASE + 0x400)
#define USB_HOST_FRAME_INTERVAL		(USB_BASE + 0x404)
#define USB_HOST_FRAME_NUMBER		(USB_BASE + 0x408)
#define USB_HOST_PERIODIC_FIFO		(USB_BASE + 0x410)
#define USB_HOST_ALL_CHAN_IRPT		(USB_BASE + 0x414)
#define USB_HOST_IPRT_MASK		(USB_BASE + 0x418)
#define USB_HOST_FRAME_LIST		(USB_BASE + 0x41C)
#define USB_HOST_PORT_CTRL_STATUS	(USB_BASE + 0x440)

#define USB_HOST_CHAN_CHAR		(USB_BASE + 0x500)
#define USB_HOST_CHAN_SPLIT_CTRL	(USB_BASE + 0x504)
#define USB_HOST_CHAN_IRPT		(USB_BASE + 0x508)
#define USB_HOST_CHAN_IRPT_MASK		(USB_BASE + 0x50C)
#define USB_HOST_CHAN_TFER_SIZE		(USB_BASE + 0x510)
#define USB_HOST_CHAN_DMA_ADDR		(USB_BASE + 0x514)

// USB_HOST_PORT_CTRL_STATUS bits
#define USB_HPCS_PRTCONNSTS		(1 << 0)
#define USB_HPCS_PRTCONNDET		(1 << 1)
#define USB_HPCS_PRTENA			(1 << 2)
#define USB_HPCS_PRTENCHNG		(1 << 3)
#define USB_HPCS_PRTOVRCURACT		(1 << 4)
#define USB_HPCS_PRTOVRCURCHNG		(1 << 5)
#define USB_HPCS_PRTRES			(1 << 6)
#define USB_HPCS_PRTSUSP		(1 << 7)
#define USB_HPCS_PRTRST			(1 << 8)
#define USB_HPCS_PRTLNSTS		(3 << 10)
#define USB_HPCS_PRTPWR			(1 << 12)
#define USB_HPCS_PRTTSTCTL		(15 << 13)
#define USB_HPCS_PRTSPD			(3 << 17)

#endif

