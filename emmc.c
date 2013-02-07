/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS 	- SD Group Physical Layer Simplified Specification
 * 
 * Broadcom BCM2835 Peripherals Guide
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mmio.h"
#include "block.h"

static char driver_name[] = "emmc";
static char device_name[] = "emmc0";	// We use a single device name as there is only
					// one card slot in the RPi

struct emmc_block_dev
{
	struct block_device bd;
	uint32_t card_supports_sdhc;
	uint32_t card_supports_18v;
	uint32_t card_ocr;
	uint32_t card_rca;
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

int sd_read(struct block_device *, uint8_t *, size_t buf_size, uint32_t);

int sd_card_init(struct block_device **dev)
{
	// Read the controller version
	uint32_t ver = mmio_read(EMMC_BASE + EMMC_SLOTISR_VER);
	uint32_t vendor = ver >> 24;
	uint32_t sdversion = (ver >> 16) & 0xff;
	uint32_t slot_status = ver & 0xff;
	printf("EMMC: vendor %x, sdversion %x, slot_status %x\n", vendor, sdversion, slot_status);


	// Mask off sending interrupts to the ARM
	mmio_write(EMMC_BASE + EMMC_IRPT_EN, 0);
	// Reset interrupts
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
	// Have all interrupts sent to the INTERRUPT register
	mmio_write(EMMC_BASE + EMMC_IRPT_MASK, 0xffffffff);

	// Send CMD0 to the card (reset to idle state)
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(0));
#ifdef DEBUG
	printf("SD: sent CMD0\n");
#endif

	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
	mmio_write(EMMC_BASE + EMMC_ARG1, 0x000001AA);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(8) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
#ifdef DEBUG
	printf("SD: sent CMD8, ");
#endif
	// Wait for a response
	while(1)
	{
		uint32_t i = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
		if(i & 0x1)
			break;
	}
#ifdef DEBUG
	printf("received response: ");
#endif
	uint32_t cmd8_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef DEBUG
	printf("%x %x %x %x\n", cmd8_resp, mmio_read(EMMC_BASE + EMMC_RESP1),
			mmio_read(EMMC_BASE + EMMC_RESP2),
			mmio_read(EMMC_BASE + EMMC_RESP3));
#endif

	// The CMD8 response is R7 (card interface condition) - PLSS 4.9.6
	// Bits 0-7 should be the check pattern
	// Bits 8-11 should be the accepted voltage (in this case 1 = 2.7-3.6V)
	if((cmd8_resp & 0xfff) != 0x1aa)
	{
		printf("SD: unusable card, exiting\n");
		return -1;
	}
	
	// The card is a valid ver2.0 or later SD memory card
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
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(55) | SD_CMD_CRCCHK_EN |
				SD_CMD_RSPNS_TYPE_48);

		// Request SDHC mode, 1.8V voltage
		mmio_write(EMMC_BASE + EMMC_ARG1, (1 << 30) | (1 << 24));
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(41) | SD_CMD_RSPNS_TYPE_136);

		// Read the response
		uint32_t acmd41_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef DEBUG
		printf("SD: ACMD41 response: %x %x %x %x\n",
				acmd41_resp,
				mmio_read(EMMC_BASE + EMMC_RESP1),
				mmio_read(EMMC_BASE + EMMC_RESP2),
				mmio_read(EMMC_BASE + EMMC_RESP3));
#endif

		uint32_t card_ready = (acmd41_resp >> 31) & 0x1;
		if(!card_ready)
		{
			printf("SD: card not yet ready\n");
			for(int i = 0; i < 100000; i++);
			continue;
		}

		ret->card_supports_sdhc = (acmd41_resp >> 30) & 0x1;
		ret->card_supports_18v = (acmd41_resp >> 24) & 0x1;
		ret->card_ocr = (acmd41_resp >> 8) & 0xffff;
		
		break;
	}

	// Switch to 1.8V mode if possible
	if(ret->card_supports_18v)
	{
		printf("SD: switching to 1.8V mode: ");

		mmio_write(EMMC_BASE + EMMC_ARG1, 0);
		mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(11) | SD_CMD_CRCCHK_EN |
				SD_CMD_RSPNS_TYPE_48);

		// Wait for completion
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);

		printf("done\n");
	}

	// Send CMD2 to get the cards CID
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(2) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_136);
	uint32_t card_cid_0 = mmio_read(EMMC_BASE + EMMC_RESP0);
	uint32_t card_cid_1 = mmio_read(EMMC_BASE + EMMC_RESP1);
	uint32_t card_cid_2 = mmio_read(EMMC_BASE + EMMC_RESP2);
	uint32_t card_cid_3 = mmio_read(EMMC_BASE + EMMC_RESP3);

#ifdef DEBUG
	printf("SD: card CID: %x%x%x%x\n", card_cid_0, card_cid_1, card_cid_2, card_cid_3);
#endif
	uint32_t *dev_id = (uint32_t *)malloc(4 * sizeof(uint32_t));
	dev_id[0] = card_cid_0;
	dev_id[1] = card_cid_1;
	dev_id[2] = card_cid_2;
	dev_id[3] = card_cid_3;
	ret->bd.device_id = (uint8_t *)dev_id;
	ret->bd.dev_id_len = 4 * sizeof(uint32_t);

	// Send CMD3 to enter the data state
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(3) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
	uint32_t cmd3_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
#ifdef DEBUG
	printf("SD: CMD3 response: %x\n", cmd3_resp);
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

#ifdef DEBUG
	printf("SD: RCA: %x\n", ret->card_rca);
#endif

	// Now select the card (toggles it to transfer state)
	mmio_write(EMMC_BASE + EMMC_ARG1, ret->card_rca << 16);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(7) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48B);
	uint32_t cmd7_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	status = (cmd7_resp >> 9) & 0xf;

	if((status != 3) && (status != 4))
	{
		printf("SD: invalid status (%i)\n", status);
		free(ret);
		free(dev_id);
		return -1;
	}

	printf("SD: found a valid SD card\n");
#ifdef DEBUG
	printf("SD: setup successful (status %x)\n", status);
#endif

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

	mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(13) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
	uint32_t status = mmio_read(EMMC_BASE + EMMC_RESP0);
	uint32_t cur_state = (status >> 9) & 0xf;
	if(cur_state == 3)
	{
		// Currently in the stand-by state - select it
		mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
		mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(7) | SD_CMD_CRCCHK_EN |
				SD_CMD_RSPNS_TYPE_48B);
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);
	}
	else if(cur_state == 5)
	{
		// In the data transfer state - cancel the transmission
		mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(12) |
					SD_CMD_CRCCHK_EN |
					SD_CMD_RSPNS_TYPE_48B);
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);
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
		mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
		mmio_write(EMMC_BASE + EMMC_ARG1, edev->card_rca << 16);
		mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(13) |
				SD_CMD_CRCCHK_EN |
				SD_CMD_RSPNS_TYPE_48);
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);
		status = mmio_read(EMMC_BASE + EMMC_RESP0);
		cur_state = (status >> 9) & 0xf;

		if(cur_state != 4)
		{
			printf("SD: unable to initialise SD card for "
					"reading (state %i)\n", cur_state);
			return -1;
		}
	}

#ifdef DEBUG
	printf("SD: read() card ready, reading from block %i\n", block_no);
#endif

	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!edev->card_supports_sdhc)
		block_no *= 512;

	mmio_write(EMMC_BASE + EMMC_ARG1, block_no);
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(17) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);

	// Wait for the command to complete
	while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x1) == 0);
	uint32_t cmd17_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	if(cmd17_resp != 0x900)	// STATE = transfer, READY_FOR_DATA = set
	{
		printf("SD: CMD17 response: %x\n", cmd17_resp);
		return -1;
	}

#ifdef DEBUG
	printf("SD: read command complete, waiting for data (current INTERRUPT: %x)\n",
			mmio_read(EMMC_BASE + EMMC_INTERRUPT));
#endif

	// Read data
	int byte_no = 0;
	int bytes_to_read = (int)buf_size;
	if(bytes_to_read > 512)
		bytes_to_read = 512;		// read a maximum of 512 bytes
	while(byte_no < 512)
	{
		// Wait for data to become available
		while((mmio_read(EMMC_BASE + EMMC_INTERRUPT) & 0x20) == 0);

		uint32_t data = mmio_read(EMMC_BASE + EMMC_DATA);
		uint8_t d0 = (uint8_t)(data & 0xff);
		uint8_t d1 = (uint8_t)((data >> 8) & 0xff);
		uint8_t d2 = (uint8_t)((data >> 16) & 0xff);
		uint8_t d3 = (uint8_t)((data >> 24) & 0xff);

		buf[byte_no++] = d0;
		if(byte_no >= bytes_to_read)
			break;
		buf[byte_no++] = d1;
		if(byte_no >= bytes_to_read)
			break;
		buf[byte_no++] = d2;
		if(byte_no >= bytes_to_read)
			break;
		buf[byte_no++] = d3;
		if(byte_no >= bytes_to_read)
			break;
	}

#ifdef DEBUG
	printf("SD: data read successful\n");
#endif

	return byte_no;
}

