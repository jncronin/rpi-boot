/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS 	- SD Group Physical Layer Simplified Specification
 */

#include <stdint.h>
#include <stdio.h>
#include "mmio.h"

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

static uint32_t card_supports_sdhc = 0;
static uint32_t card_supports_18v = 0;
static uint32_t card_ocr = 0;
static uint32_t card_cid_0, card_cid_1, card_cid_2, card_cid_3;
static uint32_t card_rca;
static uint32_t card_ready = 0;

int sd_card_init()
{
	card_ready = 0;

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
	printf("SD: sent CMD0\n");

	// Send CMD8 to the card
	// Voltage supplied = 0x1 = 2.7-3.6V (standard)
	// Check pattern = 10101010b (as per PLSS 4.3.13) = 0xAA
	mmio_write(EMMC_BASE + EMMC_INTERRUPT, 0);
	mmio_write(EMMC_BASE + EMMC_ARG1, 0x000001AA);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(8) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
	printf("SD: sent CMD8, ");
	// Wait for a response
	while(1)
	{
		uint32_t i = mmio_read(EMMC_BASE + EMMC_INTERRUPT);
		if(i & 0x1)
			break;
	}
	printf("received response: ");
	uint32_t cmd8_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	printf("%x %x %x %x\n", cmd8_resp, mmio_read(EMMC_BASE + EMMC_RESP1),
			mmio_read(EMMC_BASE + EMMC_RESP2),
			mmio_read(EMMC_BASE + EMMC_RESP3));

	// The CMD8 response is R7 (card interface condition) - PLSS 4.9.6
	// Bits 0-7 should be the check pattern
	// Bits 8-11 should be the accepted voltage (in this case 1 = 2.7-3.6V)
	if((cmd8_resp & 0xfff) != 0x1aa)
	{
		printf("SD: unusable card, exiting\n");
		return -1;
	}
	
	// The card is a valid ver2.0 or later SD memory card
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
		printf("SD: ACMD41 response: %x %x %x %x\n",
				acmd41_resp,
				mmio_read(EMMC_BASE + EMMC_RESP1),
				mmio_read(EMMC_BASE + EMMC_RESP2),
				mmio_read(EMMC_BASE + EMMC_RESP3));

		uint32_t card_ready = (acmd41_resp >> 31) & 0x1;
		if(!card_ready)
		{
			printf("SD: card not yet ready\n");
			for(int i = 0; i < 100000; i++);
			continue;
		}

		card_supports_sdhc = (acmd41_resp >> 30) & 0x1;
		card_supports_18v = (acmd41_resp >> 24) & 0x1;
		card_ocr = (acmd41_resp >> 8) & 0xffff;
		
		break;
	}

	// Switch to 1.8V mode if possible
	if(card_supports_18v)
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
	card_cid_0 = mmio_read(EMMC_BASE + EMMC_RESP0);
	card_cid_1 = mmio_read(EMMC_BASE + EMMC_RESP1);
	card_cid_2 = mmio_read(EMMC_BASE + EMMC_RESP2);
	card_cid_3 = mmio_read(EMMC_BASE + EMMC_RESP3);

	printf("SD: card CID: %x%x%x%x\n", card_cid_0, card_cid_1, card_cid_2, card_cid_3);

	// Send CMD3 to enter the data state
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(3) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48);
	uint32_t cmd3_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	printf("SD: CMD3 response: %x\n", cmd3_resp);

	card_rca = (cmd3_resp >> 16) & 0xffff;
	uint32_t crc_error = (cmd3_resp >> 15) & 0x1;
	uint32_t illegal_cmd = (cmd3_resp >> 14) & 0x1;
	uint32_t error = (cmd3_resp >> 13) & 0x1;
	uint32_t status = (cmd3_resp >> 9) & 0xf;
	uint32_t ready = (cmd3_resp >> 8) & 0x1;

	if(crc_error)
	{
		printf("SD: CRC error\n");
		return -1;
	}

	if(illegal_cmd)
	{
		printf("SD: illegal command\n");
		return -1;
	}

	if(error)
	{
		printf("SD: generic error\n");
		return -1;
	}

	if(!ready)
	{
		printf("SD: not ready for data\n");
		return -1;
	}

	printf("SD: RCA: %x\n", card_rca);

	// Now select the card (toggles it to transfer state)
	mmio_write(EMMC_BASE + EMMC_ARG1, card_rca << 16);
	mmio_write(EMMC_BASE + EMMC_CMDTM, SD_CMD_INDEX(7) | SD_CMD_CRCCHK_EN |
			SD_CMD_RSPNS_TYPE_48B);
	uint32_t cmd7_resp = mmio_read(EMMC_BASE + EMMC_RESP0);
	status = (cmd7_resp >> 9) & 0xf;


	printf("SD: setup successful (status %x)\n", status);

	card_ready = 1;
	return 0;
}

int sd_read_block(uint32_t block_no, uint8_t *buf)
{
	if(!card_ready)
	{
		printf("SD: sd_read_block() called prior to card initilisation\n");
		return -1;
	}

	// PLSS table 4.20 - SDSC cards use byte addresses rather than block addresses
	if(!card_supports_sdhc)
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

	printf("SD: read command complete, waiting for data (current INTERRUPT: %x)\n",
			mmio_read(EMMC_BASE + EMMC_INTERRUPT));

	// Read data
	int byte_no = 0;
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
		buf[byte_no++] = d1;
		buf[byte_no++] = d2;
		buf[byte_no++] = d3;
	}

	printf("SD: data read successful\n");

	return 0;
}

