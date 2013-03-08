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

/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS 	- SD Group Physical Layer Simplified Specification ver 3.00
 * HCSS		- SD Group Host Controller Simplified Specification ver 3.00
 * 
 * Broadcom BCM2835 Peripherals Guide
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mmio.h"
#include "block.h"
#include "timer.h"

#ifdef DEBUG2
#define EMMC_DEBUG
#endif

static char driver_name[] = "emmc";
static char device_name[] = "emmc0";	// We use a single device name as there is only
					// one card slot in the RPi

static uint32_t hci_ver = 0;
static uint32_t capabilities_0 = 0;
static uint32_t capabilities_1 = 0;

struct emmc_block_dev
{
	struct block_device bd;
	uint32_t card_supports_sdhc;
	uint32_t card_supports_18v;
	uint32_t card_ocr;
	uint32_t card_rca;
	uint32_t last_interrupt;
	uint32_t last_error;
};

#define EMMC_BASE		0x20300000
#define	EMMC_ARG2		0
#define EMMC_BLKSIZECNT		4
#define EMMC_ARG1		8
#define EMMC_CMDTM		0xC
#define EMMC_RESP0		0x10
#define EMMC_RESP1		0x14
#define EMMC_RESP2		0x18
#define EMMC_RESP3		0x1C
#define EMMC_DATA		0x20
#define EMMC_STATUS		0x24
#define EMMC_CONTROL0		0x28
#define EMMC_CONTROL1		0x2C
#define EMMC_INTERRUPT		0x30
#define EMMC_IRPT_MASK		0x34
#define EMMC_IRPT_EN		0x38
#define EMMC_CONTROL2		0x3C
#define EMMC_CAPABILITIES_0	0x40
#define EMMC_CAPABILITIES_1	0x44
#define EMMC_FORCE_IRPT		0x50
#define EMMC_BOOT_TIMEOUT	0x70
#define EMMC_DBG_SEL		0x74
#define EMMC_EXRDFIFO_CFG	0x80
#define EMMC_EXRDFIFO_EN	0x84
#define EMMC_TUNE_STEP		0x88
#define EMMC_TUNE_STEPS_STD	0x8C
#define EMMC_TUNE_STEPS_DDR	0x90
#define EMMC_SPI_INT_SPT	0xF0
#define EMMC_SLOTISR_VER	0xFC

#define SD_CMD_INDEX(a)		((a) << 24)
#define SD_CMD_TYPE_NORMAL	0x0
#define SD_CMD_TYPE_SUSPEND	(1 << 22)
#define SD_CMD_TYPE_RESUME	(2 << 22)
#define SD_CMD_TYPE_ABORT	(3 << 22)
#define SD_CMD_ISDATA		(1 << 21)
#define SD_CMD_IXCHK_EN		(1 << 20)
#define SD_CMD_CRCCHK_EN	(1 << 19)
#define SD_CMD_RSPNS_TYPE_NONE	0			// For no response
#define SD_CMD_RSPNS_TYPE_136	(1 << 16)		// For response R2 (with CRC), R3,4 (no CRC)
#define SD_CMD_RSPNS_TYPE_48	(2 << 16)		// For responses R1, R5, R6, R7 (with CRC)
#define SD_CMD_RSPNS_TYPE_48B	(3 << 16)		// For responses R1b, R5b (with CRC)
#define SD_CMD_MULTI_BLOCK	(1 << 5)
#define SD_CMD_DAT_DIR_HC	0
#define SD_CMD_DAT_DIR_CH	(1 << 4)
#define SD_AUTO_CMD_EN_NONE	0
#define SD_AUTO_CMD_EN_CMD12	(1 << 2)
#define SD_AUTO_CMD_EN_CMD23	(2 << 2)
#define SD_BLKCNT_EN		(1 << 1)

#define SD_ERR_CMD_TIMEOUT	0
#define SD_ERR_CMD_CRC		1
#define SD_ERR_CMD_END_BIT	2
#define SD_ERR_CMD_INDEX	3
#define SD_ERR_DATA_TIMEOUT	4
#define SD_ERR_DATA_CRC		5
#define SD_ERR_DATA_END_BIT	6
#define SD_ERR_CURRENT_LIMIT	7
#define SD_ERR_AUTO_CMD12	8
#define SD_ERR_ADMA		9
#define SD_ERR_TUNING		10
#define SD_ERR_RSVD		11

#define SD_ERR_MASK_CMD_TIMEOUT		(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC		(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT		(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX		(1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT	(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC		(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT	(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT	(1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12		(1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA		(1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING		(1 << (16 + SD_ERR_CMD_TUNING))

static char *err_irpts[] = { "CMD_TIMEOUT", "CMC_CRC", "CMD_END_BIT", "CMD_INDEX",
	"DATA_TIMEOUT", "DATA_CRC", "DATA_END_BIT", "CURRENT_LIMIT",
	"AUTO_CMD12", "ADMA", "TUNING", "RSVD" };

int sd_read(struct block_device *, uint8_t *, size_t buf_size, uint32_t);

static void sd_send_command(uint32_t command)
{
	// Wait for the CMD inhibit bit to clear
	while(mmio_read(EMMC_BASE + EMMC_STATUS) & 0x1)
		usleep(1000);

	// Send the command
	mmio_write(EMMC_BASE + EMMC_CMDTM, command);
}

static int sd_wait_response(useconds_t usec, struct emmc_block_dev *dev)
{
	int response = 0;
	uint32_t error = 0;

	if(dev)
	{
		dev->last_error = 0;
		dev->last_interrupt = 0;
	}

	if(usec == 0)
	{
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0)
		{
			error = mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8000;
			if(error)
				break;
		}
		if(!error)
			response = 1;
	}
	else
	{
		TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1, usec);
		response = mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1;
		error = mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x8000;
#ifdef EMMC_DEBUG
		if(!response)
			printf("SD: no command result received, interrupt: %08x\n",
					mmio_read(EMMC_BASE + EMMC_INTERRUPT));
#endif
	}

	if(error)
	{
		uint32_t irpt = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
		if(dev)
		{
			dev->last_error = error;
			dev->last_interrupt = irpt;
		}
		printf("SD: received error interrupt: %08x ", irpt);
		for(int i = 0; i < SD_ERR_RSVD; i++)
		{
			if(irpt & (1 << (i + 16)))
				printf(err_irpts[i]);
		}
		printf("\n");

		if(irpt & 0x10000)
		{
			// Command timeout, dump status register
			printf("SD: timeout error, status %08x\n",
					mmio_read(EMMC_BASE + EMMC_STATUS));
		}
	}

	// Reset the command done and error interrupt
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffff0001);

	return response;
}

/* Execute a R1 command
 *
 * If no error, returns 0 else the value of the interrupt register & 1
 *
 * dev - 	emmc structure
 * cmd - 	command to execute (+ control bits)
 * arg1 - 	value of ARG1
 * response - 	if not null, copy response to here
 * timeout -	how long to wait for a response
 * tries -	how many times to attempt retry
 * retry_mask - if the interrupt errors includes this mask then do a retry, else don't
 * 		we use bit 0 (value 1) to represent a timeout
 */
static uint32_t sd_do_r1_cmd(struct emmc_block_dev *dev, uint32_t cmd, uint32_t arg1,
		uint32_t *response, useconds_t timeout, int tries, uint32_t retry_mask)
{
	int cur_try = 0;

	while(cur_try < tries)
	{
		mmio_write(EMMC_BASE + EMMC_ARG1, arg1);
		sd_send_command(cmd);

		int wait_response = sd_wait_response(timeout, dev);
		uint32_t err_val = dev->last_error;
		if(wait_response)
		{
			if(response != (void *)0)
				*response = mmio_read(EMMC_BASE + EMMC_RESP0);
			return 0;
		}
		
		err_val |= 1;
		if(retry_mask & err_val)
			tries++;
		else
			break;		
	};

	return dev->last_interrupt & 1;
}

int sd_card_init(struct block_device **dev)
{
	// Read the controller version
	uint32_t ver = mmio_read(EMMC_BASE + EMMC_SLOTISR_VER);
	uint32_t vendor = ver >> 24;
	uint32_t sdversion = (ver >> 16) & 0xff;
	uint32_t slot_status = ver & 0xff;
	printf("EMMC: vendor %x, sdversion %x, slot_status %x\n", vendor, sdversion, slot_status);
	hci_ver = sdversion;

	if(hci_ver < 2)
	{
		printf("EMMC: only SDHCI versions > 3.0 are supported\n");
		return -1;
	}

	// Reset the controller
#ifdef EMMC_DEBUG
	printf("EMMC: resetting controller\n");
#endif
	uint32_t control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
	control1 |= (1 << 24);
	mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT((mmio_read(EMMC_BASE + EMMC_CONTROL1) & (0x7 << 24)) == 0, 1000000);
	if((mmio_read(EMMC_BASE + EMMC_CONTROL1) & (0x7 << 24)) != 0)
	{
		printf("EMMC: controller did not reset properly\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: control0: %08x, control1: %08x\n",
			mmio_read(EMMC_BASE + EMMC_CONTROL0),
			mmio_read(EMMC_BASE + EMMC_CONTROL1));
#endif

	// Read the capabilities registers
	capabilities_0 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_0);
	capabilities_1 = mmio_read(EMMC_BASE + EMMC_CAPABILITIES_1);
#ifdef EMMC_DEBUG
	printf("EMMC: capabilities: %08x%08x\n", capabilities_1, capabilities_0);
#endif

	// Check for a valid card
#ifdef EMMC_DEBUG
	printf("EMMC: checking for an inserted card\n");
#endif
	uint32_t status_reg = mmio_read(EMMC_BASE + EMMC_STATUS);
	if((status_reg & (1 << 16)) == 0)
	{
		printf("EMMC: no card inserted\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: status: %08x\n", status_reg);
#endif

	// Clear control2
	mmio_write(EMMC_BASE + EMMC_CONTROL2, 0);

	// Set clock rate to something slow
#ifdef EMMC_DEBUG
	printf("EMMC: setting clock rate\n");
#endif
	control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
	control1 |= 1;			// enable clock
	control1 |= 0x20;		// programmable clock mode
	control1 |= (1 << 8);		// base clock * M/2
	control1 |= (7 << 16);		// data timeout = TMCLK * 2^10
	mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
	TIMEOUT_WAIT(mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2, 0x1000000);
	if((mmio_read(EMMC_BASE + EMMC_CONTROL1) & 0x2) == 0)
	{
		printf("EMMC: controller's clock did not stabilise within 1 second\n");
		return -1;
	}
#ifdef EMMC_DEBUG
	printf("EMMC: control0: %08x, control1: %08x\n",
			mmio_read(EMMC_BASE + EMMC_CONTROL0),
			mmio_read(EMMC_BASE + EMMC_CONTROL1));
#endif

	// Enable the SD clock
#ifdef EMMC_DEBUG
	printf("EMMC: enabling SD clock\n");
#endif
	usleep(2000);
	control1 = mmio_read(EMMC_BASE + EMMC_CONTROL1);
	control1 |= 4;
	mmio_write(EMMC_BASE + EMMC_CONTROL1, control1);
	usleep(2000);

	// Mask off sending interrupts to the ARM
	mmio_write(EMMC_BASE + EMMC_IRPT_EN, 0);
	// Reset interrupts
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);
	// Have all interrupts sent to the INTERRUPT register
	mmio_write(EMMC_BASE + EMMC_IRPT_MASK, 0xffffffff);

	usleep(2000);

	// Send CMD0 to the card (reset to idle state)
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(0));
#ifdef EMMC_DEBUG
	printf("SD: sent CMD0 ");
#endif

	if(!sd_wait_response(500000, (void*)0))
	{
#ifdef EMMC_DEBUG
		printf("- no response\n");
#endif
		printf("EMMC: no SD card detected\n");
		return -1;
	}

#ifdef EMMC_DEBUG
	printf("done\n");
#endif

	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 1);
	mmio_write(EMMC_BASE + EMMC_ARG1, 0x000001AA);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(8) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
#ifdef EMMC_DEBUG
	printf("SD: sent CMD8, ");
#endif
	// Wait for a response
	int can_set_sdhc = sd_wait_response(500000, (void*)0);
	uint32_t sdhc_flag = 0;
	if(can_set_sdhc)
	{
		uint32_t cmd8_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef EMMC_DEBUG
		printf("received response: %08x\n", cmd8_resp);
#else
		(void)cmd8_resp;
#endif
		// The CMD8 response is R7 (card interface condition) - PLSS 4.9.6
		// Bits 0-7 should be the check pattern
		// Bits 8-11 should be the accepted voltage (in this case 1 = 2.7-3.6V)
		if((cmd8_resp & 0xfff) != 0x1aa)
		{
			printf("SD: unusable card, exiting\n");
			return -1;
		}
		sdhc_flag = 0x40000000;
	}
#ifdef EMMC_DEBUG
	else
		printf("no response received\n");
#endif

	// Prepare the device structure
	struct emmc_block_dev *ret;
	if(*dev == 0)
		ret = (struct emmc_block_dev *)malloc(sizeof(struct emmc_block_dev));
	else
		ret = (struct emmc_block_dev *)*dev;

	memset(ret, 0, sizeof(struct emmc_block_dev));
	ret->bd.driver_name = driver_name;
	ret->bd.device_name = device_name;
	ret->bd.block_size = 512;
	ret->bd.read = sd_read;

	// Check ACMD41
	while(1)
	{
		usleep(500000);
#ifdef EMMC_DEBUG
		printf("SD: sending ACMD41: CMD55: ");
#endif
		mmio_write(EMMC_BASE + EMMC_ARG1, 0);
		sd_send_command(SD_CMD_INDEX(55) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
		if(!sd_wait_response(500000, ret))
		{
#ifdef EMMC_DEBUG
			printf("SD: no CMD55 response, retrying\n");
#endif
			if(ret->last_interrupt & 0x10000)
			{
				// timeout occured
				usleep(20000);
				free(ret);
				return sd_card_init(dev);
			}

			continue;
		}
#ifdef EMMC_DEBUG
		printf("done, response %08x, ACMD41: ", mmio_read(EMMC_BASE + EMMC_RESP0));
#endif

		usleep(2000);

		// Request SDHC mode if available
//		mmio_write(EMMC_BASE + EMMC_ARG1, (1 << 30) | (1 << 24) | 0x00FF8000);
		mmio_write(EMMC_BASE + EMMC_ARG1, 0x00FF8000 | sdhc_flag);
		sd_send_command(SD_CMD_INDEX(41) | SD_CMD_RSPNS_TYPE_48);
		usleep(500000);

		// Await the response
		if(!sd_wait_response(500000, ret))
		{
			printf("SD: unusable card\n");
			free(ret);
			return -1;
		}

#ifdef EMMC_DEBUG
		printf("done\n");
#endif

		// Read the response
		usleep(250000);
		uint32_t acmd41_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef EMMC_DEBUG
		printf("SD: ACMD41 response: %08x\n",
				acmd41_resp);
#endif

		uint32_t card_ready = (acmd41_resp >> 31) & 0x1;
		if(!card_ready)
		{
			printf("SD: card not yet ready\n");
			usleep(1000000);
			continue;
		}

		ret->card_supports_sdhc = (acmd41_resp >> 30) & 0x1;
#ifdef EMMC_18V
		ret->card_supports_18v = (acmd41_resp >> 24) & 0x1;
#else
		ret->card_supports_18v = 0;
#endif
		ret->card_ocr = (acmd41_resp >> 8) & 0xffff;
	
		break;
	}

#ifdef EMMC_DEBUG
	printf("SD: card identified: OCR: %04x, 1.8v support: %i, SDHC support: %i\n",
			ret->card_ocr, ret->card_supports_18v, ret->card_supports_sdhc);
#endif

	// Switch to 1.8V mode if possible
	if(ret->card_supports_18v)
	{
#ifdef EMMC_DEBUG
		printf("SD: switching to 1.8V mode: ");
#endif

		mmio_write(EMMC_BASE + EMMC_ARG1, 0);
		mmio_write(EMMC_BASE + EMMC_INTERRUPT, 1);
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(11) | SD_CMD_CRCCHK_EN |
				SD_CMD_RSPNS_TYPE_48);

		// Wait for completion
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);

		printf("done\n");
	}

	// Send CMD2 to get the cards CID
	sd_send_command(SD_CMD_INDEX(2) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_136);
	if(!sd_wait_response(500000, ret))
	{
		printf("SD: no CMD2 response\n");
		free(ret);
		return -1;
	}
	uint32_t card_cid_0 = mmio_read(EMMC_BASE + EMMC_RESP0);
	uint32_t card_cid_1 = mmio_read(EMMC_BASE + EMMC_RESP1);
	uint32_t card_cid_2 = mmio_read(EMMC_BASE + EMMC_RESP2);
	uint32_t card_cid_3 = mmio_read(EMMC_BASE + EMMC_RESP3);

#ifdef EMMC_DEBUG
	printf("SD: card CID: %08x%08x%08x%08x\n", card_cid_3, card_cid_2, card_cid_1, card_cid_0);
#endif
	uint32_t *dev_id = (uint32_t *)malloc(4 * sizeof(uint32_t));
	dev_id[0] = card_cid_0;
	dev_id[1] = card_cid_1;
	dev_id[2] = card_cid_2;
	dev_id[3] = card_cid_3;
	ret->bd.device_id = (uint8_t *)dev_id;
	ret->bd.dev_id_len = 4 * sizeof(uint32_t);

	// Send CMD3 to enter the data state
	sd_send_command(SD_CMD_INDEX(3) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
	if(!sd_wait_response(500000, ret))
	{
		printf("SD: no CMD3 response\n");
		free(ret);
		return -1;
	}
	uint32_t cmd3_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef EMMC_DEBUG
	printf("SD: CMD3 response: %08x\n", cmd3_resp);
#endif

	ret->card_rca = (cmd3_resp >> 16) & 0xffff;
	uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
	uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
	uint32_t error = (cmd3_resp >> 13) & 0x1;
	uint32_t status = (cmd3_resp >> 9) & 0xf;
	uint32_t ready = (cmd3_resp >> 8) & 0x1;

	if(crc_error)
	{
		printf("SD: CRC error\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(illegal_cmd)
	{
		printf("SD: illegal command\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(error)
	{
		printf("SD: generic error\n");
		free(ret);
		free(dev_id);
		return -1;
	}

	if(!ready)
	{
		printf("SD: not ready for data\n");
		free(ret);
		free(dev_id);
		return -1;
	}

#ifdef EMMC_DEBUG
	printf("SD: RCA: %04x\n", ret->card_rca);
#endif

	// Now select the card (toggles it to transfer state)
	mmio_write(EMMC_BASE + EMMC_ARG1, ret->card_rca << 16);
	sd_send_command(SD_CMD_INDEX(7) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48B);
	if(!sd_wait_response(500000, ret))
	{
		printf("SD: no CMD7 response\n");
		free(ret);
		return -1;
	}
	uint32_t cmd7_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	status = (cmd7_resp >> 9) & 0xf;

	if((status != 3) && (status != 4))
	{
		printf("SD: invalid status (%i)\n", status);
		free(ret);
		free(dev_id);
		return -1;
	}

	// If not an SDHC card, ensure BLOCKLEN is 512 bytes
	if(!ret->card_supports_sdhc)
	{
		mmio_write(EMMC_BASE + EMMC_ARG1, 512);
		sd_send_command(SD_CMD_INDEX(16) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
		if(!sd_wait_response(500000, ret))
		{
			printf("SD: no CMD16 response\n");
			free(ret);
			return -1;
		}
	}
	uint32_t controller_block_size = mmio_read(EMMC_BASE + 0x4);
	controller_block_size &= (~0xfff);
	controller_block_size |= 0x200;
	mmio_write(EMMC_BASE + 0x4, controller_block_size);

	// Set 4-bit transfer mode (ACMD6)
	// All SD cards should support this (PLSS 5.6)
	// See HCSS 3.4 for the algorithm
#ifdef EMMC_4_BIT
	uint32_t irpt_enable = 0xffffffff & (~0x100);
	mmio_write(EMMC_BASE + EMMC_IRPT_MASK, irpt_enable);
	mmio_write(EMMC_BASE + EMMC_ARG1, ret->card_rca << 16);
	sd_send_command(SD_CMD_INDEX(55) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
	if(!sd_wait_response(500000, ret))
	{
		printf("SD: no CMD55 response\n");
		free(ret);
		return -1;
	}
	usleep(2000);
	mmio_write(EMMC_BASE + EMMC_ARG1, 0x2);
	sd_send_command(SD_CMD_INDEX(6) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
	if(!sd_wait_response(500000, ret))
	{
		printf("SD: no ACMD6 response\n");
		free(ret);
		return -1;
	}
	usleep(2000);
	uint32_t control0 = mmio_read(EMMC_BASE + EMMC_CONTROL0);
	control0 |= (1 << 1);
	mmio_write(EMMC_BASE + EMMC_CONTROL0, control0);
	mmio_write(EMMC_BASE + EMMC_IRPT_MASK, 0xffffffff);
#endif

	printf("SD: found a valid SD card\n");
#ifdef EMMC_DEBUG
	printf("SD: setup successful (status %i)\n", status);
#endif

	// Reset interrupt register
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0xffffffff);

	*dev = (struct block_device *)ret;
	return 0;
}

int sd_read(struct block_device *dev, uint8_t *buf, size_t buf_size, uint32_t block_no)
{
	// Check the status of the card
	struct emmc_block_dev *edev = (struct emmc_block_dev *)dev;
	if(edev->card_rca == 0)
	{
		// Try again to initialise the card
		int ret = sd_card_init(&dev);
		if(ret != 0)
			return ret;
	}

#ifdef EMMC_DEBUG
	printf("SD: read() obtaining status register: ");
#endif

	mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
	sd_send_command(SD_CMD_INDEX(13) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
	if(!sd_wait_response(500000, edev))
	{
		printf("SD: read() no response from CMD13\n");
		edev->card_rca = 0;
		return -1;
	}
	uint32_t status = mmio_read(EMMC_BASE + EMMC_RESP0);
	uint32_t cur_state = (status >> 9) & 0xf;
#ifdef EMMC_DEBUG
	printf("status %i\n", cur_state);
#endif
	if(cur_state == 3)
	{
		// Currently in the stand-by state - select it
		mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
		sd_send_command(SD_CMD_INDEX(7) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48B);
		if(!sd_wait_response(500000, edev))
		{
			printf("SD: read() no response from CMD17\n");
			edev->card_rca = 0;
			return -1;
		}
	}
	else if(cur_state == 5)
	{
		// In the data transfer state - cancel the transmission
		sd_send_command(SD_CMD_INDEX(12) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48B);
		if(!sd_wait_response(500000, edev))
		{
			printf("SD: read() no response from CMD12\n");
			edev->card_rca = 0;
			return -1;
		}
	}
	else if(cur_state != 4)
	{
		// Not in the transfer state - re-initialise
		int ret = sd_card_init(&dev);
		if(ret != 0)
			return ret;
	}

	// Check again that we're now in the correct mode
	if(cur_state != 4)
	{
#ifdef EMMC_DEBUG
		printf("SD: read() rechecking status: ");
#endif
		mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
		sd_send_command(SD_CMD_INDEX(13) | SD_CMD_CRCCHK_EN | SD_CMD_RSPNS_TYPE_48);
		if(!sd_wait_response(500000, edev))
		{
			printf("SD: read() no response from CMD13\n");
			edev->card_rca = 0;
			return -1;
		}
		status = mmio_read(EMMC_BASE + EMMC_RESP0);
		cur_state = (status >> 9) & 0xf;

#ifdef EMMC_DEBUG
		printf("%i\n", cur_state);
#endif

		if(cur_state != 4)
		{
			printf("SD: unable to initialise SD card for "
					"reading (state %i)\n", cur_state);
			edev->card_rca = 0;
			return -1;
		}
	}

#ifdef EMMC_DEBUG
	printf("SD: read() card ready, reading from block %u\n", block_no);
#endif

	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!edev->card_supports_sdhc)
		block_no *= 512;

	// This is as per HCSS 3.7.2.1
	// Note that block size and block count are already set

	// Send the read single block command
	uint32_t cmd_17_resp;
	uint32_t cmd_17_ctrl = sd_do_r1_cmd(edev, SD_CMD_INDEX(17) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48 | SD_CMD_DAT_DIR_CH | SD_CMD_ISDATA,
			block_no, &cmd_17_resp, 500000, 3, SD_ERR_MASK_CMD_CRC |
			SD_ERR_MASK_DATA_CRC);

	if(cmd_17_ctrl)
	{
		printf("SD: error from CMD17 command: %08x\n", cmd_17_ctrl);
		return -1;
	}

	// Get response data
	uint32_t cmd17_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	if(cmd17_resp != 0x900)	// STATE = transfer, READY_FOR_DATA = set
	{
		printf("SD: error invalid CMD17 response: %x\n", cmd17_resp);
		return -1;
	}

#ifdef EMMC_DEBUG
	printf("SD: read command complete, waiting for data (current INTERRUPT: %08x, "
			"STATUS: %08x)\n",
			mmio_read(EMMC_BASE + EMMC_INTERRUPT),
			mmio_read(EMMC_BASE + EMMC_STATUS));
#endif
	
	// Read data
	int byte_no = 0;
	int bytes_to_read = (int)buf_size;
	if(bytes_to_read > 512)
		bytes_to_read = 512;		// read a maximum of 512 bytes

	// Wait for buffer read ready interrupt
#ifdef EMMC_DEBUG
	printf("SD: awaiting buffer read ready interrupt ");
	int card_interrupt_displayed = 0;
	uint32_t old_interrupt = 0;
#endif
	while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x20) == 0)
	{
#ifdef EMMC_DEBUG
		uint32_t cur_irpt = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
		if(cur_irpt != old_interrupt)
		{
			printf("SD: interrupt %08x\n", cur_irpt);
			old_interrupt = cur_irpt;
		}
		if(cur_irpt & 0x100)
		{
			if(!card_interrupt_displayed)
			{
				// Get card status (CMD13)
				mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
				sd_send_command(SD_CMD_INDEX(13) | SD_CMD_CRCCHK_EN |
						SD_CMD_RSPNS_TYPE_48);
				if(!sd_wait_response(500000, edev))
				{
					printf("SD: read() no response from CMD13\n");
					edev->card_rca = 0;
					return -1;
				}
				
				uint32_t card_status = mmio_read(EMMC_BASE + EMMC_RESP0);
				printf("SD: interrupt: %08x, controller status %08x, "
						"card status %08x\n",
						cur_irpt,
						mmio_read(EMMC_BASE + EMMC_STATUS),
						card_status);
				card_interrupt_displayed = 1;
			}
		}
#endif
		usleep(1000);
	}
#ifdef EMMC_DEBUG
	printf("done\n");
#endif
	// Clear buffer read ready interrupt
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0x20);

	// Get data
	while(byte_no < 512)
	{
		uint32_t data = mmio_read(EMMC_BASE + EMMC_DATA);
		uint8_t d0 = (uint8_t)(data & 0xff);
		uint8_t d1 = (uint8_t)((data >> 8) & 0xff);
		uint8_t d2 = (uint8_t)((data >> 16) & 0xff);
		uint8_t d3 = (uint8_t)((data >> 24) & 0xff);

		if(byte_no < bytes_to_read)
			buf[byte_no++] = d0;
		else
			byte_no++;
		if(byte_no < bytes_to_read)
			buf[byte_no++] = d1;
		else
			byte_no++;
		if(byte_no < bytes_to_read)
			buf[byte_no++] = d2;
		else
			byte_no++;
		if(byte_no < bytes_to_read)
			buf[byte_no++] = d3;
		else
			byte_no++;
	}

	// Wait for transfer complete interrupt
#ifdef EMMC_DEBUG
	printf("SD: awaiting transfer complete interrupt ");
#endif
	while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x2) == 0)
		usleep(1000);
#ifdef EMMC_DEBUG
	printf("done\n");
#endif
	// Clear transfer complete interrupt
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0x2);

#ifdef EMMC_DEBUG
	printf("SD: data read successful\n");
#endif

	return byte_no;
}

