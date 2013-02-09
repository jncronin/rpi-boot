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
	while((mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTCONNDET)
			== 0);

	// Initiate a port reset
	printf("USB: resetting port\n");
	hprt = mmio_read(USB_HOST_PORT_CTRL_STATUS);
	hprt |= USB_HPCS_PRTRST;
	mmio_write(USB_HOST_PORT_CTRL_STATUS, hprt);

	// Wait 10 ms - TODO use timer
	for(int i = 0; i < 1000000; i++);

	// Clear port reset bit
	hprt = mmio_read(USB_HOST_PORT_CTRL_STATUS);
	hprt &= ~USB_HPCS_PRTRST;
	mmio_write(USB_HOST_PORT_CTRL_STATUS, hprt);

	// Wait for the port enable disable change bit to be set
	printf("USB: waiting for port enable disable change bit\n");
	while((mmio_read(USB_HOST_PORT_CTRL_STATUS) & USB_HPCS_PRTENCHNG)
			== 0);

	// Read the port speed
	printf("USB: reading port speed\n");
	uint32_t port_speed = (mmio_read(USB_HOST_PORT_CTRL_STATUS) >> 17)
		& 0x3;

	printf("USB: port speed %i\n", port_speed);
	while(1);

	return 0;
}

