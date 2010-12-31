/*
 * board/omap3621_boxer/tps65921_pm.c
 *
 * Copyright (C) 2010 Barnes & Noble, Inc.
 * Intrinsyc Software International, Inc. on behalf of Barnes & Noble, Inc.
 *
 * TPS PMIC PM config routines for u-boot
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


#define TPS65921_CHIP_PM		0x4b

#define PM_MASTER_REG_OFFSET		0x36

#define DEBUG(x...)	 printf(x)

/* Register Offsets */
#define CFG_P123_TRANSITION 		0x03
#define PWR_P1_SW_EVENTS 		0x10

/* CFG_P123_TRANSITION Fields */
#define SEQ_OFFSYNC (1<<0)

/* PWR_P1_SW_EVENTS Fields */
#define PWR_STOPON_POWERON		(1<<6)
#define PWR_DEVOFF			    (1<<0)
#define PWR_ENABLE_WARMRESET    (1<<4)


/*****************************************************
 * Functions to read and write from tps65921 PM Master
 *****************************************************/
static inline int tps65921_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg + PM_MASTER_REG_OFFSET, 1, &val, 1);
}

static inline int tps65921_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg + PM_MASTER_REG_OFFSET, 1, val, 1);
}

void tps65921_poweroff(void)
{
    int ret = 0;
    u8 ival = 0;
    u8 nval = 0;

    /* Make sure SEQ_OFFSYNC is set so that all the res go to wait-on */
    ret = tps65921_i2c_read_u8(TPS65921_CHIP_PM, &ival, CFG_P123_TRANSITION);
    if ( ret ) {
	DEBUG("tps65921_pm: error reading CFG_P123_TRANSITION\n");
	return;
    }

    nval = (ival | SEQ_OFFSYNC);
    ret = tps65921_i2c_write_u8(TPS65921_CHIP_PM, nval, CFG_P123_TRANSITION);
    if ( ret )  {
	DEBUG("tps65921_pm: error writing CFG_P123_TRANSITION\n");
	return;
    }

    tps65921_i2c_read_u8(TPS65921_CHIP_PM, &nval, CFG_P123_TRANSITION);
    DEBUG("tps65921_pm: CFG_P123_TRANSITION: %02x changed to %02x\n", ival, nval);

    ret = tps65921_i2c_read_u8(TPS65921_CHIP_PM, &ival, PWR_P1_SW_EVENTS);
    if ( ret ) {
	DEBUG("tps65921_pm: error reading PWR_P1_SW_EVENTS\n");
	return;
    }

    nval = ival | (PWR_STOPON_POWERON | PWR_DEVOFF | PWR_ENABLE_WARMRESET);
    ret = tps65921_i2c_write_u8(TPS65921_CHIP_PM, nval, PWR_P1_SW_EVENTS);
    if ( ret )  {
	DEBUG("tps65921_pm: error writing PWR_P1_SW_EVENTS\n");
	return;
    }

    tps65921_i2c_read_u8(TPS65921_CHIP_PM, &nval, PWR_P1_SW_EVENTS);
    DEBUG("tps65921_pm: PWR_P1_SW_EVENTS : %02x changed to %02x\n", ival, nval);
}
