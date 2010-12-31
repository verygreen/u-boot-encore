/*
 * Board specific setup info
 *
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
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

/* DPLL(1-4) PARAM TABLES */
/* Each of the tables has M, N, FREQSEL, M2 values defined for nominal
 * OPP (1.2V). The fields are defined according to dpll_param struct(clock.c).
 * The values are defined for all possible sysclk and for ES1 and ES2.
 */

/* WARNING : Only the 26MHz timing has been tested */

/* Vdd1 = 1.20 for mpu and iva default */

mpu_dpll_param:
/* 12MHz */
.word 50, 0, 0, 1
/* 13MHz */
.word 600, 12, 0, 1
/* 19.2MHz */
.word 125, 3, 0, 1
/* 26MHz */
.word 300, 12, 0, 1
/* 38.4MHz */
.word 125, 7, 0, 1

iva_dpll_param:
/* 12MHz */
.word 130, 2, 0, 1
/* 13MHz */
.word 40, 0, 0, 1
/* 19.2MHz */
.word 325, 11, 0, 1
/* 26MHz */
.word 20, 0, 0, 1
/* 38.4MHz */
.word 325, 23, 0, 1

#if defined(PRCM_CLK_CFG2_400MHZ)

core_dpll_param:
/* 12MHz */
.word 100, 2, 0, 1
/* 13MHz */
.word 400, 12, 0, 1
/* 19.2MHz */
.word 375, 17, 0, 1
/* 26MHz */
.word 200, 12, 0, 1
/* 38.4MHz */
.word 375, 35, 0, 1

/* for 3621 running @ 165Mhz as OMAP34xx processors */
#elif defined(PRCM_CLK_CFG2_332MHZ)

core_dpll_param:
/* 12MHz */
.word M_12,N_12,FSEL_12,M2_12

/* 13MHz */
.word M_13,N_13,FSEL_13,M2_13

/* 19.2MHz */
.word M_19p2,N_19p2,FSEL_19p2,M2_19p2

/* 26MHz */
.word M_26,N_26,FSEL_26,M2_26

/* 38.4MHz */
.word M_38p4,N_38p4,FSEL_38p4,M2_38p4

#else
#error "Undefined memory speed"
#endif /* PRCM_CLK_CFG2_* */


per_dpll_param:
#if defined(CONFIG_PER_M2_192)
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	720,	4,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 13000,	864,	0,	1,	7,	4,	9,	32,	10,	8,	6,	1
.word 19200,	720,	7,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 26000,	864,	12,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 38400,	720,	15,	0,	7,	4,	9,	32,	10,	8,	6,	1
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0
#elif defined(CONFIG_PER_SGX_192)
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	720,	4,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 13000,	864,	0,	1,	7,	4,	9,	32,	10,	8,	6,	2
.word 19200,	720,	7,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 26000,	864,	12,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 38400,	720,	15,	0,	7,	4,	9,	32,	10,	8,	6,	2
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	
#else /* Default to 96 MHz M2 */
/*	sys(kHz), m,	n,	clkin,	sd,	dco,	m2,	m3,	m4,	m5,	m6,	m2div */
.word 12000,	360,	4,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 13000,	432,	0,	1,	4,	2,	9,	16,	5,	4,	3,	1
.word 19200,	360,	7,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 26000,	432,	12,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 38400,	360,	15,	0,	4,	2,	9,	16,	5,	4,	3,	1
.word 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	
#endif
