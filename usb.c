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

/* RPi USB host controller driver
 *
 * The controller is a Synopsys DWC USB 2 OTG
 *
 * References:
 * 	Altera cv_54018-1.2, chapter 12, USB 2.0 OTG controller
 * 	Raspberry Pi - USB Controller ver 1.03 by Luke Robertson
 * 	BCM2835 Peripherals Guide
 */

#include <stdint.h>
#include <stdio.h>
#include "mmio.h"
#include "usb.h"
#include "timer.h"

/* According to the Altera documents, the usb controller is set up as a host
 * by the following sequence
 *
 * 1)	Set Port Power bit in Host Port Control and Status Register to 1
 * 2)	When a USB device connects, an interrupt is generated and the
 * 	Port Connect Detected bit in the Host Port Control register is
 * 	set to 1
 * 3)	The driver then initiates a port reset by setting the Port Reset
 * 	bit to 1
 * 4)	Wait a minimum of 10 ms
 * 5) 	Set the Port Reset bit back to 0
 * 6)	Another interrupt is generated, the Port Enable Disable Change
 * 	and Port Speed bits are set to reflect the speed of the device
 * 	that attached.
 * 7)	The Host Frame Interval Register is updated with the correct
 * 	PHY clock settings (by who?)
 * 8)	The driver then programs the following global registers
 * 		- Receive FIFO size register
 * 		- Non periodic transmit FIFO size register
 * 			(selects size and start address)
 * 		- Host periodic transmit FIFO size register
 * 			(selects size and start address)
 * 9)	The driver initializes and enables at least one channel to communicate
 * 	with the USB device
 */

int usb_init()
{
	// Set Port Power bit to 1
	printf("USB: setting port power bit\n");
	uint32_t hprt = mmio_read(USB_HOST_PORT_CTRL_STATUS);
	hprt |= USB_HPCS_PRTPWR;
	mmio_write(USB_HOST_PORT_CTRL_STATUS, hprt);

	// Wait for the port connected bit to be set
	printf("USB: waiting for port connected bit\n");
	TIMEOUT_WAIT(mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTCONNDET, 500000);
	if(!(mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTCONNDET))
		return -1;

	// Initiate a port reset
	printf("USB: resetting port\n");
	hprt = mmio_read(USB_HOST_PORT_CTRL_STATUS);
	hprt |= USB_HPCS_PRTRST;
	mmio_write(USB_HOST_PORT_CTRL_STATUS, hprt);

	// Wait 10 ms
	usleep(10000);

	// Clear port reset bit
	hprt = mmio_read(USB_HOST_PORT_CTRL_STATUS);
	hprt &= ~USB_HPCS_PRTRST;
	mmio_write(USB_HOST_PORT_CTRL_STATUS, hprt);

	// Wait for the port enable disable change bit to be set
	printf("USB: waiting for port enable disable change bit\n");
	TIMEOUT_WAIT(mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTENCHNG, 500000);
	if(!(mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTENCHNG))
		return -1;

	// Read the port speed
	printf("USB: reading port speed\n");
	uint32_t port_speed = (mmio_read(USB_HOST_PORT_CTRL_STATUS) >> 17)
		& 0x3;

	printf("USB: port speed %i\n", port_speed);

	return 0;
}

