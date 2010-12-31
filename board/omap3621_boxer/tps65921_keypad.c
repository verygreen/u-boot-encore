/*
 * board/omap3621_boxer/tps65921_keypad.c
 *
 * Copyright (C) 2010 Barnes & Noble, Inc.
 * Intrinsyc Software International, Inc. on behalf of Barnes & Noble, Inc.
 *
 * Keypad read for u-boot
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <asm/mach-types.h>

#include <tps65921.h>

extern u32 get_board_rev(void);
extern int gpio_pin_read(u32 gpio_pin);

#define TPS65921_CHIP_KEYPAD		0x4a

// We only scan 3 columns
#define MAX_COL 3

// We only scan 3 rows
#define MAX_ROW 3

/********************************************
 * Functions to read and write from tps65921
 ********************************************/
static inline int tps65921_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg + 0xD2, 1, &val, 1);
}

static inline int tps65921_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg + 0xD2, 1, val, 1);
}

/**************
 * Keypad Init
 **************/
int tps65921_keypad_init(void)
{
	int ret = 0;
	u8 ctrl;
	ret = tps65921_i2c_read_u8(TPS65921_CHIP_KEYPAD, &ctrl, KEYP_CTRL);
	if (!ret) {
		ctrl |= KEYP_CTRL_KBD_ON | KEYP_CTRL_SOFT_NRST;
		ctrl &= ~KEYP_CTRL_SOFTMODEN;
		ret = tps65921_i2c_write_u8(TPS65921_CHIP_KEYPAD, ctrl, KEYP_CTRL);
	}
	return ret;
}

/***************
 * Keypad Reset
 ***************/
int tps65921_keypad_reset(void)
{
	int ret = 0;
	u8 ctrl;
	ret = tps65921_i2c_read_u8(TPS65921_CHIP_KEYPAD, &ctrl, KEYP_CTRL);
	if (!ret) {
		ctrl &= ~KEYP_CTRL_SOFT_NRST;
		ret = tps65921_i2c_write_u8(TPS65921_CHIP_KEYPAD, ctrl, KEYP_CTRL);
	}
	return ret;
}

/**************************************************************
 * Read the keypad and return the number of keys pressed while 
 * setting the corresponding bit in the (key) argument
 **************************************************************/
int tps65921_keypad_keys_pressed(unsigned char *key)
{
	int count = 0;
	u8 col, row;

	*key = 0;
	switch(get_board_rev()) {
		case BOARD_ENCORE_REV_EVT1A:
		case BOARD_ENCORE_REV_EVT1B:
			{
				u8 cidx;

				for (cidx = 0; cidx < MAX_COL; cidx++) {
					col = ~(1 << cidx) & 0xff;
					tps65921_i2c_write_u8(TPS65921_CHIP_KEYPAD, col, KEYP_KBC);
					tps65921_i2c_read_u8(TPS65921_CHIP_KEYPAD, &row, KEYP_KBR);
					// All keys return a value on row 1
					if (!(row & 1)) {
						*key |= (1 << cidx);
						count++;
					}
				}
			}
			break;

		case BOARD_ENCORE_REV_EVT2:
			{
				u8 ridx;

				// All keys triggered by the same col (1)
				col = ~(1) & 0xff;
				tps65921_i2c_write_u8(TPS65921_CHIP_KEYPAD, col, KEYP_KBC);
				tps65921_i2c_read_u8(TPS65921_CHIP_KEYPAD, &row, KEYP_KBR);
				// All keys on their own row
				for (ridx = 0; ridx < MAX_ROW; ridx++) {
					if (!(row & (1 << ridx))) {
						switch (ridx)
						{
							case 0: *key |= HOME_KEY;
									count++;
									break;
							case 1: *key |= VOLUP_KEY;
									count++;
									break;
							case 2: *key |= VOLDN_KEY;
									count++;
									break;
							default:
									break;
						}
					}
				}
			}
			break;

		case BOARD_ENCORE_REV_DVT:
		case BOARD_ENCORE_REV_PVT:
			{
				u8 ridx;

				// All keys triggered by the same col (1)
				col = ~(1) & 0xff;
				tps65921_i2c_write_u8(TPS65921_CHIP_KEYPAD, col, KEYP_KBC);
				tps65921_i2c_read_u8(TPS65921_CHIP_KEYPAD, &row, KEYP_KBR);
				// All keys on their own row
				for (ridx = 0; ridx < MAX_ROW; ridx++) {
					if (!(row & (1 << ridx))) {
						switch (ridx)
						{
							case 0: *key |= HOME_KEY;
									count++;
									break;
							case 1: *key |= VOLUP_KEY;
									count++;
									break;
							case 2: *key |= VOLDN_KEY;
									count++;
									break;
							default:
									break;
						}
					}
				}
				// Check for HOME Key on gpio 48, moved from keypad to gpio in EVT2B
				if (!(*key & HOME_KEY)) {
					if (gpio_pin_read(48) == 0) {
						*key |= HOME_KEY;
						count++;
					}
				}
			}
			break;

		default:
			count = 0;
	}
	return count;
}
