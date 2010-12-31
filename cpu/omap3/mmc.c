/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Syed Mohammed Khasim <khasim@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation's version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <config.h>
#include <common.h>
#include <mmc.h>
#include <part.h>
#include <asm/arch/cpu.h>

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
#include <i2c.h>
#endif

#ifdef CONFIG_MMC
#include "mmc_host_def.h"
#include "mmc_protocol.h"
#include "fat.h"

#define OMAP_MMC_MASTER_CLOCK   96000000

mmc_card_data cur_card_data[2];
mmc_controller_data cur_controller_data[2];

static block_dev_desc_t mmc_blk_dev[2];

block_dev_desc_t *mmc_get_dev(int dev)
{
	if ((dev == 0) || (dev == 1))
		return (block_dev_desc_t *) &mmc_blk_dev[dev];
	else
		return NULL;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
static void twl4030_mmc_config(unsigned int slot)
{
	unsigned char data;

	/* configure the LDO */
	if (slot == 0) {
		data = 0x20;
		i2c_write(0x4B, 0x82, 1, &data, 1);
		data = 0x2;
		i2c_write(0x4B, 0x85, 1, &data, 1);
	} else {
		data = 0x20;
		i2c_write(0x4B, 0x86, 1, &data, 1);
		data = 0xB;
		i2c_write(0x4B, 0x89, 1, &data, 1);
	}
	return;
}
#endif

unsigned char mmc_board_init(mmc_controller_data *mmc_cont_cur)
{
	unsigned char ret = 1;
	unsigned int value = 0;
	unsigned char data = 0;
	unsigned char tmp = 0;
	
#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
	twl4030_mmc_config(mmc_cont_cur->slot);
#endif

	if (mmc_cont_cur->slot == 0) {
		value = CONTROL_PBIAS_LITE;
		CONTROL_PBIAS_LITE = value | (1 << 2) | (1 << 1) | (1 << 9);

		value = CONTROL_DEV_CONF0;
		CONTROL_DEV_CONF0 = value | (1 << 24);

	} else if (mmc_cont_cur->slot == 1) {
		value = CONTROL_DEV_CONF1;
		CONTROL_DEV_CONF1 = value | (1 << 6);
		value = (*(volatile unsigned int *)CM_FCLKEN1_CORE);
		(*(volatile unsigned int *)CM_FCLKEN1_CORE) = value | (1 << 25);
		value = (*(volatile unsigned int *)CM_ICLKEN1_CORE);
		(*(volatile unsigned int *)CM_ICLKEN1_CORE) = value | (1 << 25);
	}
	return ret;
}

void mmc_init_stream(mmc_controller_data *mmc_cont_cur)
{
	OMAP_HSMMC_CON(mmc_cont_cur->base) |= INIT_INITSTREAM;

	OMAP_HSMMC_CMD(mmc_cont_cur->base) = MMC_CMD0;
	while (1) {
		if ((OMAP_HSMMC_STAT(mmc_cont_cur->base) & CC_MASK))
			break;
	}
	OMAP_HSMMC_STAT(mmc_cont_cur->base) = CC_MASK;

	OMAP_HSMMC_CMD(mmc_cont_cur->base) = MMC_CMD0;
	while (1) {
		if ((OMAP_HSMMC_STAT(mmc_cont_cur->base) & CC_MASK))
			break;
	}

	OMAP_HSMMC_STAT(mmc_cont_cur->base) =
				OMAP_HSMMC_STAT(mmc_cont_cur->base);
	OMAP_HSMMC_CON(mmc_cont_cur->base) &= ~INIT_INITSTREAM;
}

unsigned char mmc_clock_config(mmc_controller_data *mmc_cont_cur,
			unsigned int iclk, unsigned short clk_div)
{
	unsigned int val;

	mmc_reg_out(OMAP_HSMMC_SYSCTL(mmc_cont_cur->base),
			(ICE_MASK | DTO_MASK | CEN_MASK),
			(ICE_STOP | DTO_15THDTO | CEN_DISABLE));

	switch (iclk) {
	case CLK_INITSEQ:
		val = MMC_INIT_SEQ_CLK / 2;
		break;
	case CLK_400KHZ:
		val = MMC_400kHz_CLK;
		break;
	case CLK_MISC:
		val = clk_div;
		break;
	default:
		return 0;
	}
	mmc_reg_out(OMAP_HSMMC_SYSCTL(mmc_cont_cur->base),
		    ICE_MASK | CLKD_MASK, (val << CLKD_OFFSET) | ICE_OSCILLATE);

	while (1) {
		if ((OMAP_HSMMC_SYSCTL(mmc_cont_cur->base) & ICS_MASK)
							!= ICS_NOTREADY)
			break;
	}

	OMAP_HSMMC_SYSCTL(mmc_cont_cur->base) |= CEN_ENABLE;
	return 1;
}

unsigned char mmc_init_setup(mmc_controller_data *mmc_cont_cur)
{
	unsigned int reg_val;

	mmc_board_init(mmc_cont_cur);

	OMAP_HSMMC_SYSCONFIG(mmc_cont_cur->base) |= MMC_SOFTRESET;
	while (1) {
		if ((OMAP_HSMMC_SYSSTATUS(mmc_cont_cur->base)
						& RESETDONE) != 0)
		break;
	}

	OMAP_HSMMC_SYSCTL(mmc_cont_cur->base) |= SOFTRESETALL;
	while ((OMAP_HSMMC_SYSCTL(mmc_cont_cur->base) & SOFTRESETALL) != 0x0)
		;

	OMAP_HSMMC_HCTL(mmc_cont_cur->base) = DTW_1_BITMODE | SDBP_PWROFF | SDVS_3V0;
	OMAP_HSMMC_CAPA(mmc_cont_cur->base) |= VS30_3V0SUP | VS18_1V8SUP;

	reg_val = OMAP_HSMMC_CON(mmc_cont_cur->base) & RESERVED_MASK;

	OMAP_HSMMC_CON(mmc_cont_cur->base) = CTPL_MMC_SD | reg_val | WPP_ACTIVEHIGH |
	    CDP_ACTIVEHIGH | MIT_CTO | DW8_1_4BITMODE | MODE_FUNC |
	    STR_BLOCK | HR_NOHOSTRESP | INIT_NOINIT | NOOPENDRAIN;

	mmc_clock_config(mmc_cont_cur, CLK_INITSEQ, 0);
	OMAP_HSMMC_HCTL(mmc_cont_cur->base) |= SDBP_PWRON;

	OMAP_HSMMC_IE(mmc_cont_cur->base) = OMAP_HSMMC_STATUS_REQ;

	mmc_init_stream(mmc_cont_cur);
	return 1;
}

unsigned char mmc_send_cmd(unsigned int base, unsigned int cmd,
			unsigned int arg, unsigned int *response, unsigned int nb_block)
{
	unsigned int mmc_stat;

	while ((OMAP_HSMMC_PSTATE(base) & DATI_MASK) == DATI_CMDDIS)
		;

	OMAP_HSMMC_BLK(base) = BLEN_512BYTESLEN | (nb_block << 16);
	OMAP_HSMMC_STAT(base) = 0xFFFFFFFF;
	OMAP_HSMMC_ARG(base) = arg;
	OMAP_HSMMC_CMD(base) = cmd | CMD_TYPE_NORMAL | CICE_NOCHECK |
	    CCCE_NOCHECK |  ACEN_DISABLE | DE_DISABLE;

	while (1) {
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0)
			return (unsigned char)mmc_stat;


		if (mmc_stat & CC_MASK) {
			OMAP_HSMMC_STAT(base) = CC_MASK;
			response[0] = OMAP_HSMMC_RSP10(base);
			if ((cmd & RSP_TYPE_MASK) == RSP_TYPE_LGHT136) {
				response[1] = OMAP_HSMMC_RSP32(base);
				response[2] = OMAP_HSMMC_RSP54(base);
				response[3] = OMAP_HSMMC_RSP76(base);
			}
			break;
		}
	}
	return 1;
}

unsigned char mmc_read_data(unsigned int base, unsigned int *output_buf)
{
	unsigned int mmc_stat;
	unsigned int read_count = 0;

	/*
	 * Start Polled Read
	 */
	while (1) {
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0)
			return (unsigned char)mmc_stat;

		if (mmc_stat & BRR_MASK) {
			unsigned int k;

			OMAP_HSMMC_STAT(base) |= BRR_MASK;
			for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
				*output_buf = OMAP_HSMMC_DATA(base);
				output_buf++;
				read_count += 4;
			}
		}

		if (mmc_stat & BWR_MASK)
			OMAP_HSMMC_STAT(base) |= BWR_MASK;

		if (mmc_stat & TC_MASK) {
			OMAP_HSMMC_STAT(base) |= TC_MASK;
			break;
		}
	}
	return 1;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
unsigned char mmc_write_data(unsigned int base, unsigned int *input_buf)
{
	unsigned int mmc_stat;

	/*
	 * Start Polled Write
	 */
	while (1) {
		do {
			mmc_stat = OMAP_HSMMC_STAT(base);
		} while (mmc_stat == 0);

		if ((mmc_stat & ERRI_MASK) != 0)
			return (unsigned char)mmc_stat;

		if (mmc_stat & BWR_MASK) {
			unsigned int k;

			OMAP_HSMMC_STAT(base) |= BWR_MASK;
			for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
				OMAP_HSMMC_DATA(base) = *input_buf;
				input_buf++;
			}
		}

		if (mmc_stat & BRR_MASK)
			OMAP_HSMMC_STAT(base) |= BRR_MASK;

		if (mmc_stat & TC_MASK) {
			OMAP_HSMMC_STAT(base) |= TC_MASK;
			break;
		}

	}
	return 1;
}

unsigned char mmc_write_data_multiblk(unsigned int base, unsigned int *input_buf, unsigned int count, unsigned int address, unsigned int address_step)
{
	unsigned int mmc_stat;
	unsigned int remaining_block, current_block;
	unsigned int loop;
	unsigned int resp[4];

	/*
	 * Start Polled Write
	 */



	current_block   = 0;
	remaining_block = count;

	printf("writing block: % 8d / % 8d", current_block, count);

	while(remaining_block)
	{
		loop = (remaining_block > 0xFFFF) ? 0xFFFF : remaining_block;

		/* Send the number of block to be sent */
		mmc_send_cmd(base, MMC_CMD23, loop, resp, 0);

		/* send Multi-Block start command */
		mmc_send_cmd(base, MMC_CMD25, address, resp, loop);

		while (1) {
			do {
				mmc_stat = OMAP_HSMMC_STAT(base);
			} while (mmc_stat == 0);

			if ((mmc_stat & ERRI_MASK) != 0){
				printf("\n\n Error %08X\n\n", mmc_stat);
				return (unsigned char)mmc_stat;
			}

			if (loop && (mmc_stat & BWR_MASK)) {
				unsigned int k;

				OMAP_HSMMC_STAT(base) |= BWR_MASK;
				for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
					OMAP_HSMMC_DATA(base) = *input_buf;
					input_buf++;
				}

				current_block++;
				remaining_block--;
				address += address_step;
				loop--;
				if((remaining_block & 0x3FF) == 0){
					printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b% 8d / % 8d", current_block, count);
				}	
			}

			if (mmc_stat & BRR_MASK)
				OMAP_HSMMC_STAT(base) |= BRR_MASK;

			if (mmc_stat & TC_MASK) {
				OMAP_HSMMC_STAT(base) |= TC_MASK;
				break;
			}

		}

	}
	printf(" => Done\n");
	return 1;
}
#endif /* CFG_CMD_MMC - not to used x-loader build */

unsigned char mmc_detect_card(mmc_card_data *mmc_card_cur,
				mmc_controller_data *mmc_contr_cur)
{
	unsigned char err;
	unsigned int argument = 0;
	unsigned int ocr_value = 0;
	unsigned int ocr_recvd = 0;
	unsigned int ret_cmd41 = 0;
	unsigned int hcs_val = 0;
	unsigned int resp[4];
	unsigned short retry_cnt = 2000;

	/* Set to Initialization Clock */
	err = mmc_clock_config(mmc_contr_cur, CLK_400KHZ, 0);
	if (err != 1)
		return err;

	mmc_card_cur->RCA = MMC_RELATIVE_CARD_ADDRESS;
	argument = 0x00000000;

	switch (mmc_contr_cur->slot) {
	case 0:
		ocr_value = (0x1FF << 15);
		break;
	case 1:
		ocr_value = 0x80;
		break;
	default:
		printf("mmc_detect_card:Invalid Slot\n");
	}
	err = mmc_send_cmd(mmc_contr_cur->base, MMC_CMD0, argument, resp, 0);
	if (err != 1)
		return err;

	argument = SD_CMD8_CHECK_PATTERN | SD_CMD8_2_7_3_6_V_RANGE;
	err = mmc_send_cmd(mmc_contr_cur->base, MMC_SDCMD8, argument, resp, 0);
	hcs_val = (err == 1) ?
	    MMC_OCR_REG_HOST_CAPACITY_SUPPORT_SECTOR :
	    MMC_OCR_REG_HOST_CAPACITY_SUPPORT_BYTE;

	argument = 0x0000 << 16;
	err = mmc_send_cmd(mmc_contr_cur->base, MMC_CMD55, argument, resp, 0);
	if (err == 1) {
		mmc_card_cur->card_type = SD_CARD;
		ocr_value |= hcs_val;
		ret_cmd41 = MMC_ACMD41;
	} else {
		mmc_card_cur->card_type = MMC_CARD;
		ocr_value |= MMC_OCR_REG_ACCESS_MODE_SECTOR;
		ret_cmd41 = MMC_CMD1;
		OMAP_HSMMC_CON(mmc_contr_cur->base) &= ~OD;
		OMAP_HSMMC_CON(mmc_contr_cur->base) |= OPENDRAIN;
	}

	argument = ocr_value;
	err = mmc_send_cmd(mmc_contr_cur->base, ret_cmd41, argument, resp, 0);
	if (err != 1)
		return err;

	ocr_recvd = ((mmc_resp_r3 *) resp)->ocr;

	while (!(ocr_recvd & (0x1 << 31)) && (retry_cnt > 0)) {
		retry_cnt--;
		if (mmc_card_cur->card_type == SD_CARD) {
			argument = 0x0000 << 16;
			err = mmc_send_cmd(mmc_contr_cur->base, MMC_CMD55,
								argument, resp, 0);
		}

		argument = ocr_value;
		err = mmc_send_cmd(mmc_contr_cur->base, ret_cmd41,
								argument, resp, 0);
		if (err != 1)
			return err;
		ocr_recvd = ((mmc_resp_r3 *) resp)->ocr;
	}

	if (!(ocr_recvd & (0x1 << 31)))
		return 0;

	if (mmc_card_cur->card_type == MMC_CARD) {
		if ((ocr_recvd & MMC_OCR_REG_ACCESS_MODE_MASK) ==
		    MMC_OCR_REG_ACCESS_MODE_SECTOR) {
			mmc_card_cur->mode = SECTOR_MODE;
		} else {
			mmc_card_cur->mode = BYTE_MODE;
		}

		ocr_recvd &= ~MMC_OCR_REG_ACCESS_MODE_MASK;
	} else {
		if ((ocr_recvd & MMC_OCR_REG_HOST_CAPACITY_SUPPORT_MASK)
		    == MMC_OCR_REG_HOST_CAPACITY_SUPPORT_SECTOR) {
			mmc_card_cur->mode = SECTOR_MODE;
		} else {
			mmc_card_cur->mode = BYTE_MODE;
		}
		ocr_recvd &= ~MMC_OCR_REG_HOST_CAPACITY_SUPPORT_MASK;
	}

	ocr_recvd &= ~(0x1 << 31);
	if (!(ocr_recvd & ocr_value))
		return 0;

	err = mmc_send_cmd(mmc_contr_cur->base, MMC_CMD2, argument, resp, 0);
	if (err != 1)
		return err;

	if (mmc_card_cur->card_type == MMC_CARD) {
		argument = mmc_card_cur->RCA << 16;
		err = mmc_send_cmd(mmc_contr_cur->base, MMC_CMD3,
							argument, resp, 0);
		if (err != 1)
			return err;
	} else {
		argument = 0x00000000;
		err = mmc_send_cmd(mmc_contr_cur->base, MMC_SDCMD3,
							argument, resp, 0);
		if (err != 1)
			return err;

		mmc_card_cur->RCA = ((mmc_resp_r6 *) resp)->newpublishedrca;
	}

	OMAP_HSMMC_CON(mmc_contr_cur->base) &= ~OD;
	OMAP_HSMMC_CON(mmc_contr_cur->base) |= NOOPENDRAIN;
	return 1;
}

unsigned char mmc_read_cardsize(unsigned int base, mmc_card_data *mmc_dev_data,
				mmc_csd_reg_t *cur_csd)
{
	mmc_extended_csd_reg_t ext_csd;
	unsigned int size, count, blk_len, blk_no, card_size, argument;
	unsigned char err;
	unsigned int resp[4];

	if (mmc_dev_data->mode == SECTOR_MODE) {
		if (mmc_dev_data->card_type == SD_CARD) {
			card_size =
			    (((mmc_sd2_csd_reg_t *) cur_csd)->
			     c_size_lsb & MMC_SD2_CSD_C_SIZE_LSB_MASK) |
			    ((((mmc_sd2_csd_reg_t *) cur_csd)->
			      c_size_msb & MMC_SD2_CSD_C_SIZE_MSB_MASK)
			     << MMC_SD2_CSD_C_SIZE_MSB_OFFSET);
			mmc_dev_data->size = card_size * 1024;
			if (mmc_dev_data->size == 0)
				return 0;
		} else {
			argument = 0x00000000;
			err = mmc_send_cmd(base, MMC_CMD8, argument, resp, 0);
			if (err != 1)
				return err;
			err = mmc_read_data(base, (unsigned int *)&ext_csd);
			if (err != 1)
				return err;
			mmc_dev_data->size = ext_csd.sectorcount;

			if (mmc_dev_data->size == 0)
				mmc_dev_data->size = 8388608;
		}
	} else {
		if (cur_csd->c_size_mult >= 8)
			return 0;

		if (cur_csd->read_bl_len >= 12)
			return 0;

		/* Compute size */
		count = 1 << (cur_csd->c_size_mult + 2);
		card_size = (cur_csd->c_size_lsb & MMC_CSD_C_SIZE_LSB_MASK) |
		    ((cur_csd->c_size_msb & MMC_CSD_C_SIZE_MSB_MASK)
		     << MMC_CSD_C_SIZE_MSB_OFFSET);
		blk_no = (card_size + 1) * count;
		blk_len = 1 << cur_csd->read_bl_len;
		size = blk_no * blk_len;
		mmc_dev_data->size = size / MMCSD_SECTOR_SIZE;
		if (mmc_dev_data->size == 0)
			return 0;
	}
	return 1;
}

unsigned char omap_mmc_read_sect(unsigned int start_sec, unsigned int num_bytes,
			mmc_controller_data *mmc_cont_cur,
			mmc_card_data *mmc_c, unsigned int *output_buf)
{
	unsigned char err;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int num_sec_val =
	    (num_bytes + (MMCSD_SECTOR_SIZE - 1)) / MMCSD_SECTOR_SIZE;
	unsigned int sec_inc_val;

	if (num_sec_val == 0) {
		return 1;
	}
	if (mmc_c->mode == SECTOR_MODE) {
		argument = start_sec;
		sec_inc_val = 1;
	} else {
		argument = start_sec * MMCSD_SECTOR_SIZE;
		sec_inc_val = MMCSD_SECTOR_SIZE;
	}
	while (num_sec_val) {
		err = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD17,
							argument, resp, 0);
		if (err != 1)
			return err;


		err = mmc_read_data(mmc_cont_cur->base, output_buf);
		if (err != 1)
			return err;

		output_buf += (MMCSD_SECTOR_SIZE / 4);
		argument += sec_inc_val;
		num_sec_val--;
	}
	return 1;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
unsigned char omap_mmc_write_sect(unsigned int *input_buf,
		unsigned int num_bytes,
		mmc_controller_data *mmc_cont_cur,
		mmc_card_data *mmc_c, unsigned long start_sec)
{
	unsigned char err;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int num_sec_val =
		(num_bytes + (MMCSD_SECTOR_SIZE - 1)) / MMCSD_SECTOR_SIZE;
	unsigned int sec_inc_val;

	if (num_sec_val == 0) {
		printf("mmc write: Invalid size\n");
		return 1;
	}

	if (mmc_c->mode == SECTOR_MODE) {
		argument = start_sec;
		sec_inc_val = 1;
	} else {
		argument = start_sec * MMCSD_SECTOR_SIZE;
		sec_inc_val = MMCSD_SECTOR_SIZE;
	}

	if(num_sec_val < 20)
	{
		/* use single block writing */
		while(num_sec_val) {
			err = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD24,
								argument, resp, 0);
			if (err != 1)
				return err;

			err = mmc_write_data(mmc_cont_cur->base, input_buf);
			if (err != 1)
				return err;

			input_buf += (MMCSD_SECTOR_SIZE / 4);
			argument += sec_inc_val;
			num_sec_val--;
		}
	}
	else
	{
		/* For huge download use multiblock writing */
		err = mmc_write_data_multiblk(mmc_cont_cur->base, input_buf, num_sec_val, argument, sec_inc_val);

		if (err != 1)
			return err;
	}
	return 1;
}

unsigned char omap_mmc_erase_sect(unsigned int start,
	mmc_controller_data *mmc_cont_cur, mmc_card_data *mmc_c, int size)
{
	unsigned char err;
	unsigned int argument;
	unsigned int num_sec_val;
	unsigned int sec_inc_val;
	unsigned int resp[4];
	unsigned int mmc_stat;

	//if (start > mmc_c->size || (start + size) > mmc_c->size) {
	//	printf("mmc erase: erase to Sector is\n"
	//		"out of card range\n");
	//	return 1;
	//}

	num_sec_val = (size + (MMCSD_SECTOR_SIZE - 1)) / MMCSD_SECTOR_SIZE;
	if (mmc_c->mode == SECTOR_MODE) {
		argument = start;
		sec_inc_val = 1;
	} else {
		argument = start * MMCSD_SECTOR_SIZE;
		sec_inc_val = MMCSD_SECTOR_SIZE;
	}
	while (num_sec_val) {
		err = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD24,
							argument, resp, 0);
		if (err != 1)
			return err;
		while (1) {
			do {
				mmc_stat = OMAP_HSMMC_STAT(mmc_cont_cur->base);
			} while (mmc_stat == 0);

			if ((mmc_stat & ERRI_MASK) != 0)
				return (unsigned char)mmc_stat;

			if (mmc_stat & BWR_MASK) {
				unsigned int k;

				OMAP_HSMMC_STAT(mmc_cont_cur->base) |= BWR_MASK;
				for (k = 0; k < MMCSD_SECTOR_SIZE / 4; k++) {
					OMAP_HSMMC_DATA(mmc_cont_cur->base) =
								0XFFFFFFFF;
				}
			}

			if (mmc_stat & BRR_MASK)
				OMAP_HSMMC_STAT(mmc_cont_cur->base) |= BRR_MASK;

			if (mmc_stat & TC_MASK) {
				OMAP_HSMMC_STAT(mmc_cont_cur->base) |= TC_MASK;
				break;
			}
		}
		argument += sec_inc_val;
		num_sec_val--;
	}
	return 1;
}
#endif

unsigned char configure_controller(mmc_controller_data *cur_controller_data,
								int slot)
{
	int ret = 0;

	cur_controller_data->slot = slot;
	switch (slot) {
	case 0:
		cur_controller_data->base = OMAP_HSMMC1_BASE;
		break;
	case 1:
		cur_controller_data->base = OMAP_HSMMC2_BASE;
		break;
	default:
		printf("MMC on SLOT=%d not Supported\n", slot);
		ret = 1;
	}
	return ret;
}

unsigned char configure_mmc(mmc_card_data *mmc_card_cur,
				mmc_controller_data *mmc_cont_cur)
{
	unsigned char ret_val;
	unsigned int argument;
	unsigned int resp[4];
	unsigned int trans_fact, trans_unit, retries = 2;
	unsigned int max_dtr;
	int dsor;
	mmc_csd_reg_t Card_CSD;
	unsigned char trans_speed;

	ret_val = mmc_init_setup(mmc_cont_cur);
	if (ret_val != 1)
		return ret_val;


	do {
		ret_val = mmc_detect_card(mmc_card_cur, mmc_cont_cur);
		retries--;
	} while ((retries > 0) && (ret_val != 1));

	argument = mmc_card_cur->RCA << 16;
	ret_val = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD9, argument, resp, 0);
	if (ret_val != 1)
		return ret_val;

	((unsigned int *)&Card_CSD)[3] = resp[3];
	((unsigned int *)&Card_CSD)[2] = resp[2];
	((unsigned int *)&Card_CSD)[1] = resp[1];
	((unsigned int *)&Card_CSD)[0] = resp[0];

	if (mmc_card_cur->card_type == MMC_CARD)
		mmc_card_cur->version = Card_CSD.spec_vers;

	trans_speed = Card_CSD.tran_speed;
	ret_val = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD4,
					MMC_DSR_DEFAULT << 16, resp, 0);
	if (ret_val != 1)
		return ret_val;

	trans_unit = trans_speed & MMC_CSD_TRAN_SPEED_UNIT_MASK;
	trans_fact = trans_speed & MMC_CSD_TRAN_SPEED_FACTOR_MASK;

	if (trans_unit > MMC_CSD_TRAN_SPEED_UNIT_100MHZ)
		return 0;

	if ((trans_fact < MMC_CSD_TRAN_SPEED_FACTOR_1_0) ||
	    (trans_fact > MMC_CSD_TRAN_SPEED_FACTOR_8_0))
		return 0;

	trans_unit >>= 0;
	trans_fact >>= 3;

	max_dtr = tran_exp[trans_unit] * tran_mant[trans_fact];
	dsor = OMAP_MMC_MASTER_CLOCK / max_dtr;

/* Following lines commented to build in x-loader; otherwise its including
 * division library and creating a linking error.
	if (OMAP_MMC_MASTER_CLOCK / dsor > max_dtr)
		dsor++;
*/
	if (dsor == 4)
		dsor = 5;
	else if (dsor == 3)
		dsor = 4;
	else
		return 1;
	
	ret_val = mmc_clock_config(mmc_cont_cur, CLK_MISC, dsor);
	if (ret_val != 1)
		return ret_val;

	argument = mmc_card_cur->RCA << 16;
	ret_val = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD7_SELECT,
							argument, resp, 0);
	if (ret_val != 1)
		return ret_val;

	/* Configure the block length to 512 bytes */
	argument = MMCSD_SECTOR_SIZE;
	ret_val = mmc_send_cmd(mmc_cont_cur->base, MMC_CMD16, argument, resp, 0);
	if (ret_val != 1)
		return ret_val;

	/* get the card size in sectors */
	ret_val = mmc_read_cardsize(mmc_cont_cur->base,
					mmc_card_cur, &Card_CSD);
	if (ret_val != 1)
		return ret_val;

	return 1;
}

unsigned long mmc_bread(int dev_num, ulong blknr, ulong blkcnt, ulong *dst)
{
	unsigned long ret;

	ret = (unsigned long)omap_mmc_read_sect(blknr,
				(blkcnt * MMCSD_SECTOR_SIZE),
			&cur_controller_data[dev_num], &cur_card_data[dev_num],
					(unsigned int *)dst);
	return ret;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
static unsigned long mmc_bwrite(int dev_num, unsigned long blknr,
                       lbaint_t blkcnt, const ulong *src)
{
        return mmc_write(dev_num, (unsigned char*)src, blknr,
                         blkcnt * MMCSD_SECTOR_SIZE);
}
#endif

int mmc_init(int slot)
{
	switch (slot) {
	case 0:
		configure_controller(&cur_controller_data[slot], slot);
		configure_mmc(&cur_card_data[slot], &cur_controller_data[slot]);
		mmc_blk_dev[slot].if_type = IF_TYPE_MMC;
		mmc_blk_dev[slot].part_type = PART_TYPE_DOS;
		mmc_blk_dev[slot].dev = cur_controller_data[slot].slot;
		mmc_blk_dev[slot].lun = 0;
		mmc_blk_dev[slot].type = 0;

		/* FIXME fill in the correct size (is set to 32MByte) */
		mmc_blk_dev[slot].blksz = MMCSD_SECTOR_SIZE;
		mmc_blk_dev[slot].lba = 0x10000;
		mmc_blk_dev[slot].removable = 0;
		mmc_blk_dev[slot].block_read = mmc_bread;
#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
		mmc_blk_dev[slot].block_write = mmc_bwrite;
#endif
		//fat_register_device(&mmc_blk_dev[slot], 1);
		break;
	case 1:
		configure_controller(&cur_controller_data[slot], slot);
		configure_mmc(&cur_card_data[slot], &cur_controller_data[slot]);
		mmc_blk_dev[slot].if_type = IF_TYPE_MMC;
		mmc_blk_dev[slot].part_type = PART_TYPE_DOS;
		mmc_blk_dev[slot].dev = cur_controller_data[slot].slot;
		mmc_blk_dev[slot].lun = 0;
		mmc_blk_dev[slot].type = 0;

		/* FIXME fill in the correct size (is set to 32MByte) */
		mmc_blk_dev[slot].blksz = MMCSD_SECTOR_SIZE;
		mmc_blk_dev[slot].lba = 0x10000;
		mmc_blk_dev[slot].removable = 0;
		mmc_blk_dev[slot].block_read = mmc_bread;
#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
		mmc_blk_dev[slot].block_write = mmc_bwrite;
#endif
		fat_register_device(&mmc_blk_dev[slot], 1);
		break;
	default:
		printf("mmc_init:mmc slot is not supported%d\n", slot);
	}
  	return 0;
}

int mmc_read(int mmc_cont, unsigned int src, unsigned char *dst, int size)
{
	int ret;

	ret = omap_mmc_read_sect(src, size, &cur_controller_data[mmc_cont],
	                        &cur_card_data[mmc_cont], (unsigned int *)dst);
	return ret;
}

#if CONFIG_DRIVER_OMAP34XX_I2C /* don't compile for x-loader */
int mmc_write(int mmc_cont, unsigned char *src, unsigned long dst, int size)
{
	int ret;

	ret = omap_mmc_write_sect((unsigned int *)src, size,
	                          &cur_controller_data[mmc_cont], &cur_card_data[mmc_cont], dst);
	return ret;
}

int mmc_erase(int mmc_cont, unsigned int start, int size)
{
	int ret;

	ret = omap_mmc_erase_sect(start, &cur_controller_data[mmc_cont],
	                          &cur_card_data[mmc_cont], size);
	return ret;
}
#endif

#endif
