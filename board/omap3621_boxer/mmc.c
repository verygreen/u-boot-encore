/*
 * (C) Copyright 2004-2010 Texas Instruments, <www.ti.com>
 * Sebastien Griffoul <s-griffoul@ti.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/mem.h>

#if (CONFIG_FASTBOOT)
#include <fastboot.h>
#endif

void board_mmc_init(void)
{
#if (CONFIG_FASTBOOT)

	/* Partitons on EMMC preasent on OMAP4SDP required for Fastboot*/
	fastboot_ptentry ptn[] = {
		/* Leaving Sector0- Sector -256 [128KB]for MBR */
		{
                        .name   = "mbr",
                        .start  = 0x000, /*Sector Start */
                        .length = 0x0000200, /*512B */
                        .flags  = 0,
                },
		
		{
			.name   = "xloader",
			.start  = 0x001, 
			.length = 0x0060000, /*384KB */
			.flags  = 0,
		},
		{
			.name   = "bootloader",
			.start  = 0x400, 
			.length = 0x0180000, /* 1.5 M */
			.flags  = 0,
		},		
         	{
			.name   = "environment",
			.start  = 0x700,      /* should be equals to CFG_ENV_ADDR (set in the config file) */
			.length = 0x0020000,  /* should be equals to CFG_ENV_SIZE (set in the config file) */
			.flags  = FASTBOOT_PTENTRY_FLAGS_WRITE_ENV,
		},
		{
			.name   = "kernel",
			.start  = 0x800,
			.length = 0x300000, /* 3M */
			.flags  = 0,
		},
		{
			.name   = "misc",
			.start  = 0x2000,    /* Should be equals to CFG_BOOTCMDBLK_ADDR (set in the config file) */
			.length = 0x200,   /* 512B */
			.flags  = 0,
		},
		{
			.name   = "recovery",
			.start  = 0x2100,
			.length = 0x1000000, /* 16M */
			.flags  = 0,
		},
		{
			.name   = "rootfs",
			.start  = 0xA100,    /* Sector Start */
			.length = 0xC800000, /*200MB */
			.flags  = 0,
		},
		{
			.name   = "user",
			.start  = 0x6E100,   /* Sector Start */
			.length = 0xC800000, /* 200MB */
			.flags  = 0,
		},
	};
	int i;
	for (i = 0; i < 9; i++) {
		printf("Fastboot: adding partition %s \tat %08X size %08X\n", ptn[i].name, ptn[i].start, ptn[i].length);
		fastboot_flash_add_ptn(&ptn[i]);
	}
#endif
}

