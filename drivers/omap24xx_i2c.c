/*
 * Basic I2C functions
 *
 * Copyright (c) 2004 Texas Instruments
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Jian Zhang jzhang@ti.com, Texas Instruments
 *
 * Copyright (c) 2003 Wolfgang Denk, wd@denx.de
 * Rewritten to fit into the current U-Boot framework
 *
 * Adapted for OMAP2420 I2C, r-woodruff2@ti.com
 *
 * Copyright (c) 2010 Barnes & Noble, Inc.
 * Intrinsyc Software International, Inc. on behalf of Barnes & Noble, Inc.
 */

#include <common.h>

#if defined(CONFIG_DRIVER_OMAP24XX_I2C) || defined(CONFIG_DRIVER_OMAP34XX_I2C)

#include <asm/arch/i2c.h>
#include <asm/io.h>
#include <i2c.h>

static u32 i2c_base = I2C_DEFAULT_BASE;
static u32 i2c_speed = CFG_I2C_SPEED;

//#define DEBUG

#ifdef DEBUG

#define DBG(ARGS...) {printf ("[%d]",__LINE__);printf(ARGS);}
#define inb(a) ({u8 v=__raw_readb(i2c_base + (a));printf("%d:Rb[%x<=%x]\n",__LINE__,a,v);v;})
#define outb(v,a) {printf("%d:Wb[%x<=%x]\n",__LINE__,a,v);__raw_writeb((v), (i2c_base + (a)));}
#define inw(a) ({u16 v=__raw_readb(i2c_base + (a));printf("%d:Rw[%x<=%x]\n",__LINE__,a,v);v;})
#define outw(v,a) {printf("%d:Ww[%x<=%x]\n",__LINE__,a,v);__raw_writew((v), (i2c_base + (a)));}

#else
#define DBG(ARGS...)
#define inb(a) __raw_readb(i2c_base + (a))
#define outb(v,a) __raw_writeb((v), (i2c_base + (a)))
#define inw(a) __raw_readw(i2c_base +(a))
#define outw(v,a) __raw_writew((v), (i2c_base + (a)))
#endif

static void wait_for_bb(void);
static u16 wait_for_pin(void);
static void flush_fifo(void);

#ifdef CONFIG_OMAP34XX
#define I2C_NUM_IF 3
#else
#define I2C_NUM_IF 2
#endif

int select_bus(int bus, int speed)
{
	if ((bus < 0) || (bus >= I2C_NUM_IF)) {
		printf("Bad bus ID-%d\n", bus);
		return -1;
	}

#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
	/* Check speed */
	if ((speed != OMAP_I2C_STANDARD) && (speed != OMAP_I2C_FAST_MODE)
	    && (speed != OMAP_I2C_HIGH_SPEED)) {
		printf("Invalid Speed for i2c init-%d\n", speed);
		return -1;
	}
#else
	if ((speed != OMAP_I2C_STANDARD) && (speed != OMAP_I2C_FAST_MODE)) {
		printf("Invalid Speed for i2c init-%d\n", speed);
		return -1;
	}
#endif

#if defined(CONFIG_OMAP34XX)
	if (bus == 2)
		i2c_base = I2C_BASE3;
	else 
#endif
	if (bus == 1)
		i2c_base = I2C_BASE2;
	else
		i2c_base = I2C_BASE1;

	i2c_init(speed, CFG_I2C_SLAVE);
	return 0;
}

void i2c_init(int speed, int slaveadd)
{
	int psc, fsscll, fssclh;
	int hsscll = 0, hssclh = 0;
	u32 scll, sclh, scl;
	int reset_timeout = 10;
	unsigned long internal_clk;

	/* Only handle standard, fast and high speeds */
	if ((speed != OMAP_I2C_STANDARD) &&
	    (speed != OMAP_I2C_FAST_MODE) &&
	    (speed != OMAP_I2C_HIGH_SPEED)) {
		printf("Error : I2C unsupported speed %d\n", speed);
		return;
	}

	/*
	 * Set the internal sampling clock to what the
	 * board requires if it is defined.  Else use
	 * the values in the v2.6.31 kernel.
	 */
#if defined(I2C_INTERNAL_SAMPLING_CLK)
	internal_clk = I2C_INTERNAL_SAMPLING_CLK;
#else
	/* standard */
	internal_clk = 4000;
	if (speed == OMAP_I2C_HIGH_SPEED)
		internal_clk = 19200;
	else if (speed == OMAP_I2C_FAST_MODE)
		internal_clk = 9600;
	else /* standard */
		internal_clk = 4000;
#endif

	psc = I2C_IP_CLK / internal_clk;
	psc -= 1;
	if (psc < I2C_PSC_MIN) {
		printf("Error : I2C unsupported prescalar %d\n", psc);
		return;
	}

	if (speed == OMAP_I2C_HIGH_SPEED) {
		/* High speed */

		/* For first phase of HS mode */
		scl = internal_clk / 400;
		fsscll = scl - (scl / 3) - I2C_HIGHSPEED_PHASE_ONE_SCLL_TRIM;
		fssclh = (scl / 3) - I2C_HIGHSPEED_PHASE_ONE_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing clock\n");
			return;
		}

		/* For second phase of HS mode */
		scl = I2C_IP_CLK / speed;
		hsscll = scl - (scl / 3) - I2C_HIGHSPEED_PHASE_TWO_SCLL_TRIM;
		hssclh = (scl / 3) - I2C_HIGHSPEED_PHASE_TWO_SCLH_TRIM;

		if (((hsscll < 0) || (hssclh < 0)) ||
		    ((hsscll > 255) || (hssclh > 255))) {
			printf("Error : I2C initializing second phase clock\n");
			return;
		}

		scll = (unsigned int)hsscll << 8 | (unsigned int)fsscll;
		sclh = (unsigned int)hssclh << 8 | (unsigned int)fssclh;

	} else if (speed == OMAP_I2C_FAST_MODE) {
		/* Standard speed */
		scl = internal_clk / speed;
		fsscll = scl - (scl / 3) - I2C_FASTSPEED_SCLL_TRIM;
		fssclh = (scl / 3) - I2C_FASTSPEED_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;

	} else {
		/* Standard speed */
		fsscll = fssclh = internal_clk / (2 * speed);

		fsscll -= I2C_FASTSPEED_SCLL_TRIM;
		fssclh -= I2C_FASTSPEED_SCLH_TRIM;

		if (((fsscll < 0) || (fssclh < 0)) ||
		    ((fsscll > 255) || (fssclh > 255))) {
			printf("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;
	}

	/* Execute Soft-reset sequence for I2C controller */
	reset_timeout = 100;
	while ((inw(I2C_CON) & I2C_CON_EN) && reset_timeout--) {
		/* Ensure that the module is disabled */
		outw(0, I2C_CON);
	}
	if (reset_timeout <= 0)
		printf("ERROR: Timeout to Disable the Module\n");

	outw(I2C_SYSC_SRST, I2C_SYSC);  /* Set the I2Ci.I2C_SYSC[1] SRST bit to 1 */
	udelay(1000);
	outw(I2C_CON_EN, I2C_CON);  /* Enable the module */

	reset_timeout = 100;
	while (!(inw(I2C_SYSS) & I2C_SYSS_RDONE) && reset_timeout--) {
		if (reset_timeout <= 0)
			printf("ERROR: Timeout while waiting for soft-reset to complete\n");
	}

	outw(0, I2C_CON);  /* Disable I2C controller before writing
                                        to PSC and SCL registers */
	outw(psc, I2C_PSC);
	outw(scll, I2C_SCLL);
	outw(sclh, I2C_SCLH);

	/* own address */
	outw(slaveadd, I2C_OA);
	outw(I2C_CON_EN, I2C_CON);

	/* have to enable intrrupts or OMAP i2c module doesn't work */
	outw(I2C_IE_XRDY_IE | I2C_IE_RRDY_IE | I2C_IE_ARDY_IE |
	     I2C_IE_NACK_IE | I2C_IE_AL_IE, I2C_IE);
	udelay(1000);
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
}

static int i2c_read_byte(u8 devaddr, u8 regoffset, u8 * value)
{
	int err;
	int i2c_error = 0;
	u16 status;

	/* wait until bus not busy */
	wait_for_bb();

	/* one byte only */
	outw(1, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* no stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX, I2C_CON);

	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
		/* Important: have to use byte access */
		outb(regoffset, I2C_DATA);

		/* Important: wait for ARDY bit to set */
		err = 2000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0)
			i2c_error = 1;

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		err = 2000;
		outw(I2C_CON_EN, I2C_CON);
		while (inw(I2C_STAT) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* Have to clear pending interrupt to clear I2C_STAT */
			outw(0xFFFF, I2C_STAT);
			if (!err--) {
				break;
			}
		}

		/* set slave address */
		outw(devaddr, I2C_SA);
		/* read one byte from slave */
		outw(1, I2C_CNT);
		/* need stop bit here */
		outw(I2C_CON_EN |
		     ((i2c_speed ==
		       OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) | I2C_CON_MST |
		     I2C_CON_STT | I2C_CON_STP, I2C_CON);

		status = wait_for_pin();
		if (status & I2C_STAT_RRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
			*value = inb(I2C_DATA);
#else
			*value = inw(I2C_DATA);
#endif
		/* Important: wait for ARDY bit to set */
		err = 20000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0){
printf("i2c_read_byte -- I2C_STAT_ARDY error\n");
			i2c_error = 1;
		}
		} else {
			i2c_error = 1;
		}

		if (!i2c_error) {
			int err = 1000;
			outw(I2C_CON_EN, I2C_CON);
			while (inw(I2C_STAT)
			       || (inw(I2C_CON) & I2C_CON_MST)) {
				outw(0xFFFF, I2C_STAT);
				if (!err--) {
					break;
				}
			}
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

int i2c_read_2_byte(u8 devaddr, u8 regoffset, u8 * value)
{
	int err;
	int i2c_error = 0;
	u16 status;

	if (!value) return 1;
	
	/* wait until bus not busy */
	wait_for_bb();

	/* one byte only */
	outw(1, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* no stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX, I2C_CON);

	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
		/* Important: have to use byte access */
		outb(regoffset, I2C_DATA);

		/* Important: wait for ARDY bit to set */
		err = 2000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0)
			i2c_error = 1;

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		err = 2000;
		outw(I2C_CON_EN, I2C_CON);
		while (inw(I2C_STAT) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* Have to clear pending interrupt to clear I2C_STAT */
			outw(0xFFFF, I2C_STAT);
			if (!err--) {
				break;
			}
		}

		/* set slave address */
		outw(devaddr, I2C_SA);
		/* read two bytes from slave */
		outw(2, I2C_CNT);
		/* need stop bit here */
		outw(I2C_CON_EN |
		     ((i2c_speed ==
		       OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) | I2C_CON_MST |
		     I2C_CON_STT | I2C_CON_STP, I2C_CON);

		status = wait_for_pin();
		if (status & I2C_STAT_RRDY) {
		int i =0;
		for (i=0; i<2; i++) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
				*value++ = inb(I2C_DATA);
#else
				*value = inw(I2C_DATA);
#endif
			/* Important: wait for ARDY bit to set */
			err = 20000;
			while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
				;
		}	
		if (err <= 0){
printf("i2c_read_byte -- I2C_STAT_ARDY error\n");
			i2c_error = 1;
		}
		} else {
			i2c_error = 1;
		}

		if (!i2c_error) {
			int err = 1000;
			outw(I2C_CON_EN, I2C_CON);
			while (inw(I2C_STAT)
			       || (inw(I2C_CON) & I2C_CON_MST)) {
				outw(0xFFFF, I2C_STAT);
				if (!err--) {
					break;
				}
			}
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

int i2c_read_16_byte(u8 devaddr, u8 regoffset, u8 * value)
{
	int err;
	int i2c_error = 0;
	u16 status;

	if (!value) return 1;
	
	/* wait until bus not busy */
	wait_for_bb();

	/* one byte only */
	outw(1, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* no stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX, I2C_CON);

	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
		/* Important: have to use byte access */
		outb(regoffset, I2C_DATA);

		/* Important: wait for ARDY bit to set */
		err = 2000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
			;
		if (err <= 0)
			i2c_error = 1;

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}

	if (!i2c_error) {
		err = 2000;
		outw(I2C_CON_EN, I2C_CON);
		while (inw(I2C_STAT) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* Have to clear pending interrupt to clear I2C_STAT */
			outw(0xFFFF, I2C_STAT);
			if (!err--) {
				break;
			}
		}

		/* set slave address */
		outw(devaddr, I2C_SA);
		/* read 16 bytes from slave */
		outw(16, I2C_CNT);
		/* need stop bit here */
		outw(I2C_CON_EN |
		     ((i2c_speed ==
		       OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) | I2C_CON_MST |
		     I2C_CON_STT | I2C_CON_STP, I2C_CON);

		status = wait_for_pin();
		if (status & I2C_STAT_RRDY) {
		int i =0;
		for (i=0; i<16; i++) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
				*value++ = inb(I2C_DATA);
#else
				*value = inw(I2C_DATA);
#endif
			/* Important: wait for ARDY bit to set */
			err = 20000;
			while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && err--)
				;
		}	
		if (err <= 0){
printf("i2c_read_byte -- I2C_STAT_ARDY error\n");
			i2c_error = 1;
		}
		} else {
			i2c_error = 1;
		}

		if (!i2c_error) {
			int err = 1000;
			outw(I2C_CON_EN, I2C_CON);
			while (inw(I2C_STAT)
			       || (inw(I2C_CON) & I2C_CON_MST)) {
				outw(0xFFFF, I2C_STAT);
				if (!err--) {
					break;
				}
			}
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

static int i2c_write_byte(u8 devaddr, u8 regoffset, u8 value)
{
	int eout;
	int i2c_error = 0;
	u16 status, stat;

	/* wait until bus not busy */
	wait_for_bb();

	/* two bytes */
	outw(2, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP, I2C_CON);

	/* wait until state change */
	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
		/* send out 1 byte */
		outb(regoffset, I2C_DATA);
		outw(I2C_STAT_XRDY, I2C_STAT);
		status = wait_for_pin();
		if ((status & I2C_STAT_XRDY)) {
			/* send out next 1 byte */
			outb(value, I2C_DATA);
			outw(I2C_STAT_XRDY, I2C_STAT);
		} else {
			i2c_error = 1;
		}
#else
		/* send out 2 bytes */
		outw((value << 8) | regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		eout= 20000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && eout--)
			;
		if (eout <= 0)
			printf("timed out in i2c_write_byte: I2C_STAT=%x\n",
			       inw(I2C_STAT));

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}
	if (!i2c_error) {
		eout = 2000;

		outw(I2C_CON_EN, I2C_CON);
		while ((stat = inw(I2C_STAT)) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* have to read to clear intrrupt */
			outw(0xFFFF, I2C_STAT);
			if (--eout == 0)	/* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

#if 0
int i2c_write_2_byte(u8 devaddr, u8 regoffset, u8* value)
{
	int eout;
	int i2c_error = 0;
	u16 status, stat;
	
	if (!value)	return 1;

	/* wait until bus not busy */
	wait_for_bb();

	/* two bytes */
	outw(3, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP, I2C_CON);

	/* wait until state change */
	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
		/* send out 1 byte */
		outb(regoffset, I2C_DATA);
		outw(I2C_STAT_XRDY, I2C_STAT);
		status = wait_for_pin();
		if ((status & I2C_STAT_XRDY)) {
			/* send out next 1 byte */
			outb(value[0], I2C_DATA);
			outw(I2C_STAT_XRDY, I2C_STAT);
			outb(value[1], I2C_DATA);
			outw(I2C_STAT_XRDY, I2C_STAT);
		} else {
			i2c_error = 1;
		}
#else
		/* send out 2 bytes */
		outw((value << 8) | regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		eout= 20000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && eout--)
			;
		if (eout <= 0)
			printf("timed out in i2c_write_byte: I2C_STAT=%x\n",
			       inw(I2C_STAT));

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}
	if (!i2c_error) {
		eout = 2000;

		outw(I2C_CON_EN, I2C_CON);
		while ((stat = inw(I2C_STAT)) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* have to read to clear intrrupt */
			outw(0xFFFF, I2C_STAT);
			if (--eout == 0)	/* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

int i2c_write_16_byte(u8 devaddr, u8 regoffset, u8* value)
{
	int eout;
	int i2c_error = 0;
	u16 status, stat;
	
	if (!value)	return 1;

	/* wait until bus not busy */
	wait_for_bb();

	/* 16 bytes */
	outw(17, I2C_CNT);
	/* set slave address */
	outw(devaddr, I2C_SA);
	/* stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP, I2C_CON);

	/* wait until state change */
	status = wait_for_pin();

	if (status & I2C_STAT_XRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
		/* send out 1 byte */
		outb(regoffset, I2C_DATA);
		outw(I2C_STAT_XRDY, I2C_STAT);
		status = wait_for_pin();
		if ((status & I2C_STAT_XRDY)) {
			int i;
			for ( i = 0; i <16; i++) {
			/* send out next 1 byte */
			outb(value[i], I2C_DATA);
			outw(I2C_STAT_XRDY, I2C_STAT);
			}
		} else {
			i2c_error = 1;
		}
#else
		/* send out 2 bytes */
		outw((value << 8) | regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		eout= 20000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && eout--)
			;
		if (eout <= 0)
			printf("timed out in i2c_write_byte: I2C_STAT=%x\n",
			       inw(I2C_STAT));

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}
	if (!i2c_error) {
		eout = 2000;

		outw(I2C_CON_EN, I2C_CON);
		while ((stat = inw(I2C_STAT)) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* have to read to clear intrrupt */
			outw(0xFFFF, I2C_STAT);
			if (--eout == 0)	/* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}
#endif

static void flush_fifo(void)
{
	u16 stat;

	/* note: if you try and read data when its not there or ready
	 * you get a bus error
	 */
	while (1) {
		stat = inw(I2C_STAT);
		if (stat == I2C_STAT_RRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
			inb(I2C_DATA);
#else
			inw(I2C_DATA);
#endif
			outw(I2C_STAT_RRDY, I2C_STAT);
		} else
			break;
	}
}

int i2c_probe(uchar chip)
{
	int res = 1;		/* default = fail */

	if (chip == inw(I2C_OA)) {
		return res;
	}

	/* wait until bus not busy */
	wait_for_bb();

	/* try to read one byte */
	outw(1, I2C_CNT);
	/* set slave address */
	outw(chip, I2C_SA);
	/* stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_STP, I2C_CON);
	/* enough delay for the NACK bit set */
	udelay(50000);

	if (!(inw(I2C_STAT) & I2C_STAT_NACK)) {
		res = 0;	/* success case */
		flush_fifo();
		outw(0xFFFF, I2C_STAT);
	} else {
		outw(0xFFFF, I2C_STAT);	/* failue, clear sources */
		outw(inw(I2C_CON) | I2C_CON_STP, I2C_CON);	/* finish up xfer */
		udelay(20000);
		wait_for_bb();
	}
	flush_fifo();
	outw(0, I2C_CNT);	/* don't allow any more data in...we don't want it. */
	outw(0xFFFF, I2C_STAT);
	return res;
}

int i2c_read(uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	int i;

	if (alen > 1) {
		printf("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_read_byte(chip, addr + i, &buffer[i])) {
			printf("I2C read: I/O error\n");
			i2c_init(i2c_speed, CFG_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	int i;

	if (alen > 1) {
		printf("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf("I2C read: address out of range\n");
		return 1;
	}

	for (i = 0; i < len; i++) {
		if (i2c_write_byte(chip, addr + i, buffer[i])) {
			printf("I2C read: I/O error\n");
			i2c_init(i2c_speed, CFG_I2C_SLAVE);
			return 1;
		}
	}

	return 0;
}

static int i2c_multidata_write_byte(u8 devaddr, u8 regoffset, u8 *values, int len)
{
	int eout;
	int i2c_error = 0;
	u16 status, stat;
        int i=0;
        int count=0;

	/* wait until bus not busy */
	wait_for_bb();
        //printf("I2C DEBUG: Bus not busy complete\n");

        count = len + 1;
	/* length+1 bytes */
	outw(count, I2C_CNT);
        //printf("I2C DEBUG: Count set to %x\n", count);

	/* set slave address */
	outw(devaddr, I2C_SA);
        //printf("I2C DEBUG: Slave Address set to %x\n", devaddr);

	/* stop bit needed here */
	outw(I2C_CON_EN | ((i2c_speed == OMAP_I2C_HIGH_SPEED) ? 0x1 << 12 : 0) |
	     I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP, I2C_CON);
        //printf("I2C DEBUG: Configuration set\n");

	/* wait until state change */
	status = wait_for_pin();
        //printf("I2C DEBUG: Wait pin status change\n");

	if (status & I2C_STAT_XRDY) {
#if defined(CONFIG_OMAP243X) || defined(CONFIG_OMAP34XX)
		/* send out 1 byte */
                //printf("I2C DEBUG: Transmit ready status\n");

		outb(regoffset, I2C_DATA);
                //printf("I2C DEBUG: Register Offset set to %x\n", regoffset);

		outw(I2C_STAT_XRDY, I2C_STAT);
                //printf("I2C DEBUG: Clearing transmit ready\n");

               	for (i = 0; i < len; i++) {
		  status = wait_for_pin();
                  //printf("I2C DEBUG: Wait pin status change\n");

		  if ((status & I2C_STAT_XRDY)) {
                        //printf("I2C DEBUG: Data output iteration %x\n", i);
			/* send out next 1 byte */
			outb(values[i], I2C_DATA);
                        //printf("I2C DEBUG: Data output value written %x\n", values[i]);
			outw(I2C_STAT_XRDY, I2C_STAT);
                        //printf("I2C DEBUG: Clearing transmit ready \n");
		  } else {
                        printf("I2C error\n");
			i2c_error = 1;
		  }
                }
                //printf("I2C DEBUG: Multidata byte write transfer complete \n");
#else
		/* send out 2 bytes */
		outw((value << 8) | regoffset, I2C_DATA);
#endif
		/* must have enough delay to allow BB bit to go low */
		eout= 20000;
		while (!(inw(I2C_STAT) & I2C_STAT_ARDY) && eout--)
			;
		if (eout <= 0)
			printf("timed out in i2c_write_byte: I2C_STAT=%x\n",
			       inw(I2C_STAT));

		if (inw(I2C_STAT) & I2C_STAT_NACK) {
			i2c_error = 1;
		}
	} else {
		i2c_error = 1;
	}
	if (!i2c_error) {
		eout = 2000;

		outw(I2C_CON_EN, I2C_CON);
		while ((stat = inw(I2C_STAT)) || (inw(I2C_CON) & I2C_CON_MST)) {
			/* have to read to clear intrrupt */
			outw(0xFFFF, I2C_STAT);
			if (--eout == 0)	/* better leave with error than hang */
				break;
		}
	}
	flush_fifo();
	outw(0xFFFF, I2C_STAT);
	outw(0, I2C_CNT);
	return i2c_error;
}

int i2c_multidata_write(uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	int i;

	if (alen > 1) {
		printf("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (addr + len > 256) {
		printf("I2C read: address out of range\n");
		return 1;
	}

	if (i2c_multidata_write_byte(chip, addr, &buffer[0], len)) {
		printf("I2C read: I/O error\n");
		i2c_init(i2c_speed, CFG_I2C_SLAVE);
		return 1;
	}

	return 0;
}

static void wait_for_bb(void)
{
	int timeout = 5000;
	u16 stat;

	outw(0xFFFF, I2C_STAT);	/* clear current interruts... */
	while ((stat = inw(I2C_STAT) & I2C_STAT_BB) && timeout--) {
		outw(stat, I2C_STAT);
	}

	if (timeout <= 0) {
		printf("timed out in wait_for_bb: I2C_STAT=%x\n",
		       inw(I2C_STAT));
	}
	outw(0xFFFF, I2C_STAT);	/* clear delayed stuff */
}

static u16 wait_for_pin(void)
{
	u16 status;
	int timeout = 9000;

	do {
		status = inw(I2C_STAT);
	} while (!(status &
		   (I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
		    I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
		    I2C_STAT_AL)) && timeout--);

	if (timeout <= 0) {
		printf("timed out in wait_for_pin: I2C_STAT=%x\n",
		       inw(I2C_STAT));
		outw(0xFFFF, I2C_STAT);
	}
	return status;
}

#endif				/* CONFIG_DRIVER_OMAP24XX_I2C */
