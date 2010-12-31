/*
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
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <asm/mach-types.h>
#include <i2c.h>
#include <linux/mtd/nand_ecc.h>
#include <twl4030.h>
#include <tps65921.h>

int get_boot_type(void);
void v7_flush_dcache_all(int, int);
void l2cache_enable(void);
void setup_auxcr(int, int);
void eth_init(void *);
u32 get_board_rev(void);
extern int gpio_pin_read(u32 gpio_pin);

enum hw_board_id {
	NONE,
	EVT1B = 0x2,
	EVT2,
	DVT,
	PVT
};

#define MAKE_HW_BOARD_ID(id1, id2, id3) (id1 | (id2 << 1 ) | (id3 << 2 ))

/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}

static const char * const arch_string(u32 mtype) 
{
    switch (mtype) {
    case MACH_TYPE_OMAP3621_BOXER:
        return "BOXER";
    case MACH_TYPE_OMAP3621_EVT1A:
        return "Encore";
    default:
        return "Unknown";
    }
}

static const char * const rev_string(u32 btype)
{
    switch (btype) {
    case BOARD_ENCORE_REV_EVT1A:
        return "EVT1A";
    case BOARD_ENCORE_REV_EVT1B:
        return "EVT1B";
    case BOARD_ENCORE_REV_EVT2:
        return "EVT2";
    case BOARD_ENCORE_REV_DVT:
        return "DVT";
    case BOARD_ENCORE_REV_PVT:
        return "PVT";
    default:
        return "Unknown";
    }
}

static inline enum hw_board_id read_board_id(void)
{
    enum hw_board_id bid;

    bid = MAKE_HW_BOARD_ID(gpio_pin_read(40), 
                            gpio_pin_read(41),
                            gpio_pin_read(42));
    return bid;
}

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
    enum hw_board_id bid;

	gpmc_init();		/* in SRAM or SDRAM, finish GPMC */
#if defined(CONFIG_3621EVT1A)
    gd->bd->bi_arch_number = MACH_TYPE_OMAP3621_EVT1A; /* Linux mach id */
    bid = read_board_id(); 

    switch(bid) {
        case NONE:
            gd->bd->bi_board_revision = BOARD_ENCORE_REV_EVT1A;
	        break;
        case EVT1B:
            gd->bd->bi_board_revision = BOARD_ENCORE_REV_EVT1B;
            break;
        case EVT2:
            gd->bd->bi_board_revision = BOARD_ENCORE_REV_EVT2; 
            break;
        case DVT:
            gd->bd->bi_board_revision = BOARD_ENCORE_REV_DVT; 
            break;
        case PVT:
            gd->bd->bi_board_revision = BOARD_ENCORE_REV_PVT; 
            break;
        default:   
            gd->bd->bi_arch_number = BOARD_ENCORE_REV_UNKNOWN;
        }

#else
	gd->bd->bi_arch_number = MACH_TYPE_OMAP3621_BOXER; /* Linux mach id*/
#endif
	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100); /* boot param addr */

	return 0;
}

/*****************************************
 * Routine: secure_unlock
 * Description: Setup security registers for access
 * (GP Device only)
 *****************************************/
void secure_unlock_mem(void)
{
	/* Permission values for registers -Full fledged permissions to all */
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	/* Protection Module Register Target APE (PM_RT)*/
	__raw_writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_1, RT_READ_PERMISSION_0);
	__raw_writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, RT_ADDR_MATCH_1);

	__raw_writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	/* IVA Changes */
	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_1);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_2);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_3);

	__raw_writel(UNLOCK_1, SMS_RG_ATT0); /* SDRC region 0 public */
}


/**********************************************************
 * Routine: secureworld_exit()
 * Description: If chip is EMU and boot type is external
 *		configure secure registers and exit secure world
 *  general use.
 ***********************************************************/
void secureworld_exit(void)
{
	unsigned long i;

	/* configrue non-secure access control register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 2":"=r" (i));
	/* enabling co-processor CP10 and CP11 accesses in NS world */
	__asm__ __volatile__("orr %0, %0, #0xC00":"=r"(i));
	/* allow allocation of locked TLBs and L2 lines in NS world */
	/* allow use of PLE registers in NS world also */
	__asm__ __volatile__("orr %0, %0, #0x70000":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 2":"=r" (i));

	/* Enable ASA and IBE in ACR register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x50":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1":"=r" (i));

	/* Exiting secure world */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 0":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x31":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 0":"=r" (i));
}

/**********************************************************
 * Routine: try_unlock_sram()
 * Description: If chip is GP/EMU(special) type, unlock the SRAM for
 *  general use.
 ***********************************************************/
void try_unlock_memory(void)
{
	int mode;
	int in_sdram = running_in_sdram();

	/* if GP device unlock device SRAM for general use */
	/* secure code breaks for Secure/Emulation device - HS/E/T*/
	mode = get_device_type();
	if (mode == GP_DEVICE) {
		secure_unlock_mem();
	}
	/* If device is EMU and boot is XIP external booting
	 * Unlock firewalls and disable L2 and put chip
	 * out of secure world
	 */
	/* Assuming memories are unlocked by the demon who put us in SDRAM */
	if ((mode <= EMU_DEVICE) && (get_boot_type() == 0x1F)
		&& (!in_sdram)) {
		secure_unlock_mem();
		secureworld_exit();
	}

	return;
}

/**********************************************************
 * Routine: s_init
 * Description: Does early system init of muxing and clocks.
 * - Called path is with SRAM stack.
 **********************************************************/
void s_init(void)
{
	int i;
	int external_boot = 0;

	watchdog_init();

	external_boot = (get_boot_type() == 0x1F) ? 1 : 0;
	/* Right now flushing at low MPU speed. Need to move after clock init */
	v7_flush_dcache_all(get_device_type(), external_boot);

	try_unlock_memory();

	if (cpu_is_3410()) {
		/* Lock down 6-ways in L2 cache so that effective size of L2 is 64K */
		__asm__ __volatile__("mov %0, #0xFC":"=r" (i));
		__asm__ __volatile__("mcr p15, 1, %0, c9, c0, 0":"=r" (i));
	}

#ifndef CONFIG_ICACHE_OFF
	icache_enable();
#endif

	l2cache_enable();

	set_muxconf_regs();
	/*identify the HW REV and other IDs to do the board differentiation without
	  ifdefs */
	
	delay(100);

	/* Writing to AuxCR in U-boot using SMI for GP/EMU DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	setup_auxcr(get_device_type(), external_boot);

	prcm_init();

	per_clocks_enable();
}

/*******************************************************
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 ********************************************************/
#if defined(CONFIG_3621EVT1A)
typedef enum {
    GPIO_OUTPUT = 0,
    GPIO_INPUT  = 1
} gpio_dir_t;

/* ========================================================================== */
/**
*  gpio_level_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    GPIO_LOW  = 0,
    GPIO_HIGH = 1
} gpio_level_t;

extern int gpio_pin_init(u32, gpio_dir_t, gpio_level_t);
#endif

#if defined(CONFIG_DRV_BOARD_INIT)
void drv_board_init (void)
{
#ifdef CONFIG_DRIVER_OMAP34XX_I2C
#if defined(CONFIG_3621EVT1A)
	/* turn on long pwr button press reset*/
	unsigned char data;
	data = 0x40;
	i2c_write(0x4b, 0x46, 1, &data, 1);
#endif
#endif
}
#endif

void encore_button_rtc_ack(void)
{
#define RTC_STATUS_DUMP (RTC_STATUS_POWER_UP|RTC_STATUS_ALARM|RTC_STATUS_DAY_EVENT|RTC_STATUS_HOUR_EVENT|RTC_STATUS_MIN_EVENT|RTC_STATUS_SEC_EVENT)

#define TWL4030_DUMPRTC_REG(X) 	    \
	if (i2c_read(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + X, 1, &val, 1))  \
	{	printf("%s: I/O error\n",#X);} else printf("%s=0x%x\n",#X,val);

	unsigned char val,data;
	int err;

	err=i2c_read(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_INTERRUPTS_REG, 1, &data, 1);
	if (data) {
		printf("warning-rtc irq enabled = 0x%x\n",data);
	}
	err|=i2c_read(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_STATUS_REG, 1, &val, 1);
	if (data||(val&RTC_STATUS_DUMP)) {
		printf("warning-rtc status = 0x%x\n",val);
		TWL4030_DUMPRTC_REG(RTC_ALARM_SECONDS_REG);
		TWL4030_DUMPRTC_REG(RTC_ALARM_MINUTES_REG);   
		TWL4030_DUMPRTC_REG(RTC_ALARM_HOURS_REG);     
		TWL4030_DUMPRTC_REG(RTC_ALARM_DAYS_REG);      
		TWL4030_DUMPRTC_REG(RTC_ALARM_MONTHS_REG);    
		TWL4030_DUMPRTC_REG(RTC_ALARM_YEARS_REG);     
		/* get the time*/
		i2c_read(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_CTRL_REG, 1, &val, 1);
		val|=0x40;
		i2c_write(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_CTRL_REG, 1, &val, 1);
		TWL4030_DUMPRTC_REG(RTC_SECONDS_REG);
		TWL4030_DUMPRTC_REG(RTC_MINUTES_REG);
		TWL4030_DUMPRTC_REG(RTC_HOURS_REG);
		TWL4030_DUMPRTC_REG(RTC_DAYS_REG);
		TWL4030_DUMPRTC_REG(RTC_MONTHS_REG);
		TWL4030_DUMPRTC_REG(RTC_YEARS_REG);
		TWL4030_DUMPRTC_REG(RTC_WEEKS_REG);

		/* acknowledge any alarm*/
		val = RTC_STATUS_RTC_ALARM_ACK;
		val = i2c_write(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_STATUS_REG, 1, &val, 1);
	}
	if (data) {
		printf("warning-clearing rtc irq\n");
		data = 0;
		val = i2c_write(TWL4030_CHIP_RTC, TWL4030_BASEADD_RTC + RTC_INTERRUPTS_REG, 1, &val, 1);
	}
}

int misc_init_r(void)
{
    DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_DRIVER_OMAP34XX_I2C
	unsigned char data;
   
    printf("Hardware arch: %s rev: %s\n", 
            arch_string(gd->bd->bi_arch_number), 
            rev_string(gd->bd->bi_board_revision));
   	
	extern int twl4030_init_battery_charging(void);

	i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);
	(void) twl4030_usb_init();
	twl4030_power_reset_init();
	/* see if we need to activate the power button startup */
#if defined(CONFIG_3621EVT1A)

	/* turn on long pwr button press reset*/
	data = 0x40;
	i2c_write(0x4b, 0x46, 1, &data, 1);
	printf("Power Button Active\n");



	printf("Keep TP in reset \n");
	gpio_pin_init(46, GPIO_OUTPUT, GPIO_HIGH);

        printf("Keep Audio codec under reset \n");
        gpio_pin_init(37, GPIO_OUTPUT, GPIO_LOW);

        printf("Power on Audio codec\n");
        gpio_pin_init(103, GPIO_OUTPUT, GPIO_HIGH);
      
	printf("Take TP out of reset \n");
	gpio_pin_init(46, GPIO_OUTPUT, GPIO_LOW);

#endif
	char *s = getenv("pbboot");
	if (s) {
		/* figure out why we have booted */
		i2c_read(0x4b, 0x3a, 1, &data, 1);

		/* if status is non-zero, we didn't transition
		 * from WAIT_ON state
		 */
		if (data) {
			printf("Transitioning to Wait State (%x)\n", data);

			/* clear status */
			data = 0;
			i2c_write(0x4b, 0x3a, 1, &data, 1);

			/* put PM into WAIT_ON state */
			data = 0x01;
			i2c_write(0x4b, 0x46, 1, &data, 1);

			/* no return - wait for power shutdown */
			while (1) {;}
		}
		printf("Transitioning to Active State (%x)\n", data);

		/* turn on long pwr button press reset*/
		data = 0x40;
		i2c_write(0x4b, 0x46, 1, &data, 1);
		printf("Power Button Active\n");
	}
#endif

	tps65921_keypad_init();
	dieid_num_r();
	return (0);
}

/******************************************************
 * Routine: wait_for_command_complete
 * Description: Wait for posting to finish on watchdog
 ******************************************************/
void wait_for_command_complete(unsigned int wd_base)
{
	int pending = 1;
	do {
		pending = __raw_readl(wd_base + WWPS);
	} while (pending);
}

/****************************************
 * Routine: watchdog_init
 * Description: Shut down watch dogs
 *****************************************/
void watchdog_init(void)
{
	/* There are 3 watch dogs WD1=Secure, WD2=MPU, WD3=IVA. WD1 is
	 * either taken care of by ROM (HS/EMU) or not accessible (GP).
	 * We need to take care of WD2-MPU or take a PRCM reset.  WD3
	 * should not be running and does not generate a PRCM reset.
	 */

	sr32(CM_FCLKEN_WKUP, 5, 1, 1);
	sr32(CM_ICLKEN_WKUP, 5, 1, 1);
	wait_on_value(BIT5, 0x20, CM_IDLEST_WKUP, 5); /* some issue here */

	__raw_writel(WD_UNLOCK1, WD2_BASE + WSPR);
	wait_for_command_complete(WD2_BASE);
	__raw_writel(WD_UNLOCK2, WD2_BASE + WSPR);
}

/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
    #define NOT_EARLY 0
    DECLARE_GLOBAL_DATA_PTR;
	unsigned int size0 = 0, size1 = 0;
	u32 mtype, btype;

	btype = get_board_type();
	mtype = get_mem_type();
#ifndef CONFIG_3430ZEBU
	/* fixme... dont know why this func is crashing in ZeBu */
	display_board_info(btype);
#endif
    /* If a second bank of DDR is attached to CS1 this is
     * where it can be started.  Early init code will init
     * memory on CS0.
     */
	if ((mtype == DDR_COMBO) || (mtype == DDR_STACKED)) {
		do_sdrc_init(SDRC_CS1_OSET, NOT_EARLY);
	}
	size0 = get_sdr_cs_size(SDRC_CS0_OSET);
	size1 = get_sdr_cs_size(SDRC_CS1_OSET);

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = size0;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_1+size0;
	gd->bd->bi_dram[1].size = size1;

	return 0;
}

#define 	MUX_VAL(OFFSET,VALUE)\
		__raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)

/*
 * IEN  - Input Enable
 * IDIS - Input Disable
 * PTD  - Pull type Down
 * PTU  - Pull type Up
 * DIS  - Pull type selection is inactive
 * EN   - Pull type selection is active
 * M0   - Mode 0
 * The commented string gives the final mux configuration for that pin
 */

#define MUX_EVT1A()\
	/*SDRC*/\
	MUX_VAL(CP(SDRC_D0),        (IEN  | PTD | DIS | M0)) /*SDRC_D0*/\
	MUX_VAL(CP(SDRC_D1),        (IEN  | PTD | DIS | M0)) /*SDRC_D1*/\
	MUX_VAL(CP(SDRC_D2),        (IEN  | PTD | DIS | M0)) /*SDRC_D2*/\
	MUX_VAL(CP(SDRC_D3),        (IEN  | PTD | DIS | M0)) /*SDRC_D3*/\
	MUX_VAL(CP(SDRC_D4),        (IEN  | PTD | DIS | M0)) /*SDRC_D4*/\
	MUX_VAL(CP(SDRC_D5),        (IEN  | PTD | DIS | M0)) /*SDRC_D5*/\
	MUX_VAL(CP(SDRC_D6),        (IEN  | PTD | DIS | M0)) /*SDRC_D6*/\
	MUX_VAL(CP(SDRC_D7),        (IEN  | PTD | DIS | M0)) /*SDRC_D7*/\
	MUX_VAL(CP(SDRC_D8),        (IEN  | PTD | DIS | M0)) /*SDRC_D8*/\
	MUX_VAL(CP(SDRC_D9),        (IEN  | PTD | DIS | M0)) /*SDRC_D9*/\
	MUX_VAL(CP(SDRC_D10),       (IEN  | PTD | DIS | M0)) /*SDRC_D10*/\
	MUX_VAL(CP(SDRC_D11),       (IEN  | PTD | DIS | M0)) /*SDRC_D11*/\
	MUX_VAL(CP(SDRC_D12),       (IEN  | PTD | DIS | M0)) /*SDRC_D12*/\
	MUX_VAL(CP(SDRC_D13),       (IEN  | PTD | DIS | M0)) /*SDRC_D13*/\
	MUX_VAL(CP(SDRC_D14),       (IEN  | PTD | DIS | M0)) /*SDRC_D14*/\
	MUX_VAL(CP(SDRC_D15),       (IEN  | PTD | DIS | M0)) /*SDRC_D15*/\
	MUX_VAL(CP(SDRC_D16),       (IEN  | PTD | DIS | M0)) /*SDRC_D16*/\
	MUX_VAL(CP(SDRC_D17),       (IEN  | PTD | DIS | M0)) /*SDRC_D17*/\
	MUX_VAL(CP(SDRC_D18),       (IEN  | PTD | DIS | M0)) /*SDRC_D18*/\
	MUX_VAL(CP(SDRC_D19),       (IEN  | PTD | DIS | M0)) /*SDRC_D19*/\
	MUX_VAL(CP(SDRC_D20),       (IEN  | PTD | DIS | M0)) /*SDRC_D20*/\
	MUX_VAL(CP(SDRC_D21),       (IEN  | PTD | DIS | M0)) /*SDRC_D21*/\
	MUX_VAL(CP(SDRC_D22),       (IEN  | PTD | DIS | M0)) /*SDRC_D22*/\
	MUX_VAL(CP(SDRC_D23),       (IEN  | PTD | DIS | M0)) /*SDRC_D23*/\
	MUX_VAL(CP(SDRC_D24),       (IEN  | PTD | DIS | M0)) /*SDRC_D24*/\
	MUX_VAL(CP(SDRC_D25),       (IEN  | PTD | DIS | M0)) /*SDRC_D25*/\
	MUX_VAL(CP(SDRC_D26),       (IEN  | PTD | DIS | M0)) /*SDRC_D26*/\
	MUX_VAL(CP(SDRC_D27),       (IEN  | PTD | DIS | M0)) /*SDRC_D27*/\
	MUX_VAL(CP(SDRC_D28),       (IEN  | PTD | DIS | M0)) /*SDRC_D28*/\
	MUX_VAL(CP(SDRC_D29),       (IEN  | PTD | DIS | M0)) /*SDRC_D29*/\
	MUX_VAL(CP(SDRC_D30),       (IEN  | PTD | DIS | M0)) /*SDRC_D30*/\
	MUX_VAL(CP(SDRC_D31),       (IEN  | PTD | DIS | M0)) /*SDRC_D31*/\
	MUX_VAL(CP(SDRC_CLK),       (IEN  | PTD | DIS | M0)) /*SDRC_CLK*/\
	MUX_VAL(CP(SDRC_DQS0),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS0*/\
	MUX_VAL(CP(SDRC_DQS1),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS1*/\
	MUX_VAL(CP(SDRC_DQS2),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS2*/\
	MUX_VAL(CP(SDRC_DQS3),      (IEN  | PTD | DIS | M0)) /*SDRC_DQS3*/\
	MUX_VAL(CP(SDRC_nCS0),      (IEN  | PTD | DIS | M0)) /*SDRC_nCS0*/\
	MUX_VAL(CP(SDRC_nCS1),      (IEN  | PTD | DIS | M0)) /*SDRC_nCS1*/\
	MUX_VAL(CP(SYS_NRESWARM),   (IDIS | PTD | DIS | M0)) /*SYS_NRESWARM*/\
	/*GPMC - NC*/\
	MUX_VAL(CP(GPMC_D0),        (IEN  | PTD | EN  | M4)) /*GPMC_D0 NC*/\
	MUX_VAL(CP(GPMC_D1),        (IEN  | PTD | EN  | M4)) /*GPMC_D1 NC*/\
	MUX_VAL(CP(GPMC_D2),        (IEN  | PTD | EN  | M4)) /*GPMC_D2 NC*/\
	MUX_VAL(CP(GPMC_D3),        (IEN  | PTD | EN  | M4)) /*GPMC_D3 NC*/\
	MUX_VAL(CP(GPMC_D4),        (IEN  | PTD | EN  | M4)) /*GPMC_D4 NC*/\
	MUX_VAL(CP(GPMC_D5),        (IEN  | PTD | EN  | M4)) /*GPMC_D5 NC*/\
	MUX_VAL(CP(GPMC_D6),        (IEN  | PTD | EN  | M4)) /*GPMC_D6 NC*/\
	MUX_VAL(CP(GPMC_D7),        (IEN  | PTD | EN  | M4)) /*GPMC_D7 NC*/\
	MUX_VAL(CP(GPMC_nCS0),      (IEN  | PTD | EN  | M4)) /*GPMC_nCS0 NC*/\
	MUX_VAL(CP(GPMC_nADV_ALE),  (IEN  | PTD | EN  | M4)) /*GPMC_nADV_ALE NC*/\
	MUX_VAL(CP(GPMC_nOE),       (IEN  | PTD | EN  | M4)) /*GPMC_nOE NC*/\
	MUX_VAL(CP(GPMC_nWE),       (IEN  | PTD | EN  | M4)) /*GPMC_nWE NC*/\
	MUX_VAL(CP(GPMC_nBE1),      (IEN  | PTD | DIS | M4)) /*GPMC_nBE1 DC-CHG-ILM*/\
	MUX_VAL(CP(GPMC_WAIT0),     (IEN  | PTD | EN  | M4)) /*GPMC_WAIT0 NC*/\
	/*GPIOs*/\
	MUX_VAL(CP(ETK_D0_ES2 ),    (IEN  | WAKEUP_EN  | PTD | DIS | M4)) /*GIO-14  SYS-PWR-ON-KEY*/\
	MUX_VAL(CP(ETK_D1_ES2 ),    (IEN  | PTD | DIS | M4)) /*GIO-15  WLAN-IRQ */\
	MUX_VAL(CP(ETK_D2_ES2 ),    (IEN  | PTD | DIS | M4)) /*GIO-16  EN_WIFI_POW */\
	MUX_VAL(CP(ETK_D7_ES2 ),    (IEN  | PTD | EN  | M4)) /*GIO-21  MODEM_EN */\
	MUX_VAL(CP(ETK_D8_ES2 ),    (IEN  | PTD | DIS | M4)) /*GIO-22  WLAN-EN */\
	MUX_VAL(CP(ETK_D9_ES2 ),    (IEN  | PTD | EN  | M4)) /*GIO-23  HSUSB2_RSTn */\
	MUX_VAL(CP(GPMC_A1),        (IEN  | PTD | EN  | M4)) /*GIO-34  MODEM_nON NOT PRESENT */\
	MUX_VAL(CP(GPMC_A2),        (IEN  | PTD | DIS | M4)) /*GIO-35  HW-ID3  SKU*/\
	MUX_VAL(CP(GPMC_A3),        (IEN  | PTD | DIS | M4)) /*GIO-36  LCD_PWR_EN */\
	MUX_VAL(CP(GPMC_A4),        (IEN  | PTD | DIS | M4)) /*GIO-37  AUD_nRESET */\
	MUX_VAL(CP(GPMC_A5),        (IEN  | PTD | DIS | M4)) /*GIO-38  HW-ID2  PRODUCT ID*/\
	MUX_VAL(CP(GPMC_A6),        (IEN  | PTD | DIS | M4)) /*GIO-39  CRADLE_DET */\
	MUX_VAL(CP(GPMC_A7),        (IEN  | PTD | DIS | M4)) /*GIO-40  HW-ID7  HW-REV*/\
	MUX_VAL(CP(GPMC_A8),        (IEN  | PTD | DIS | M4)) /*GIO-41  HW-ID6  HW-REV*/\
	MUX_VAL(CP(GPMC_A9),        (IEN  | PTD | DIS | M4)) /*GIO-42  HW-ID5  HW-REV*/\
	MUX_VAL(CP(GPMC_A10),       (IEN  | PTD | DIS | M4)) /*GIO-43  HW-ID4  SKU*/\
	MUX_VAL(CP(GPMC_D8),        (IDIS | PTD | EN  | M4)) /*GPMC_D8 GPIO44 CABC0*/\
	MUX_VAL(CP(GPMC_D9),        (IDIS | PTD | EN  | M4)) /*GPMC_D9 GPIO45 CABC1*/\
	MUX_VAL(CP(GPMC_D10),       (IEN  | PTD | DIS | M4)) /*GPMC_D10 GPIO46 TP-nRESET*/\
	MUX_VAL(CP(GPMC_D11),       (IDIS | PTD | DIS | M4))  /* GPMC_D11 */\
	MUX_VAL(CP(GPMC_D12),       (IEN  | PTU | EN  | M4)) /* GPMC_D12 */\
	MUX_VAL(CP(GPMC_D13),       (IEN  | PTD | EN  | M4)) /*GPMC_D13 GPIO49 misc I/O*/\
	MUX_VAL(CP(GPMC_D14),       (IEN  | PTD | EN  | M4)) /*GPMC_D14 GPIO50 misc I/O*/\
	MUX_VAL(CP(GPMC_D15),       (IEN  | PTD | EN  | M4)) /*GPMC_D15 GPIO51 misc I/O*/\
	MUX_VAL(CP(GPMC_CLK),       (IEN  | PTD | DIS | M4)) /*GIO-59 AUD-CODEC-nINT*/\
	MUX_VAL(CP(GPMC_nBE0_CLE),  (IEN  | PTD | DIS | M4)) /*GPIO60 BT-EN*/\
	MUX_VAL(CP(GPMC_nWP),       (IEN  | PTD | EN  | M4)) /*GPIO62 AMBI-LIGHT-SENSE-EN*/\
	MUX_VAL(CP(CAM_XCLKA),      (IEN  | PTD | DIS | M4)) /*GIO-96 HW-ID0  PRODUCT ID*/\
	MUX_VAL(CP(CAM_D0 ),        (IEN  | PTD | DIS | M4)) /*GIO-99  TS_INT */\
	MUX_VAL(CP(CAM_D1 ),        (IEN  | PTD | DIS | M4)) /*GIO-100 GG_INT*/\
	MUX_VAL(CP(CAM_D2 ),        (IEN  | PTD | DIS | M4)) /*GIO-101 nCHG_FAULT*/\
	MUX_VAL(CP(CAM_D3 ),        (IEN  | PTD | DIS | M4)) /*GIO-102 CHRG_EN1*/\
	MUX_VAL(CP(CAM_D4 ),        (IEN  | PTD | DIS | M4)) /*GIO-103 AUD-CODEC-EN */\
	MUX_VAL(CP(CAM_D5 ),        (IEN  | PTD | DIS | M4)) /*GIO-104 CHRG_EN2*/\
	MUX_VAL(CP(CAM_D10),        (IEN  | PTD | EN  | M4)) /*GIO-109 UART_GPIO (spare)*/\
	MUX_VAL(CP(CAM_D11),        (IEN  | PTD | DIS | M4)) /*GIO-110 CHRG_Cen*/\
	MUX_VAL(CP(CAM_XCLKB),      (IEN  | PTD | DIS | M4)) /*GIO-111 CHRG_STATUS*/\
	MUX_VAL(CP(CSI2_DX0),       (IEN  | PTD | DIS | M4)) /*GIO-112 HW-ID1 PRODUCT ID*/\
	MUX_VAL(CP(CSI2_DY0),       (IEN  | PTD | DIS | M4)) /*GIO-113 MOT-nINT*/\
	MUX_VAL(CP(CSI2_DX1),       (IEN  |  WAKEUP_EN|PTD | DIS | M4)) /*GIO-114 nDC-CHG-OK, should be wakup source */\
	MUX_VAL(CP(CSI2_DY1),       (IEN  |  WAKEUP_EN|PTD | DIS | M4)) /*GIO-115 nUSB-CHG-OK should be wakeup source*/\
	MUX_VAL(CP(McBSP1_FSR),     (IEN  | PTD | DIS | M4)) /*GIO-157 HS-nDETECT*/ \
	/* Backlight */ \
	MUX_VAL(CP(GPMC_nCS7),      (IDIS | PTD | DIS | M3)) /*GPT_8_PWM_EVT */ \
	/*DSS*/\
	MUX_VAL(CP(DSS_PCLK),       (IDIS | PTD | DIS | M0)) /*DSS_PCLK*/\
	MUX_VAL(CP(DSS_HSYNC),      (IDIS | PTD | DIS | M0)) /*DSS_HSYNC*/\
	MUX_VAL(CP(DSS_VSYNC),      (IDIS | PTD | DIS | M0)) /*DSS_VSYNC*/\
	MUX_VAL(CP(DSS_ACBIAS),     (IEN  | PTD | DIS | M0)) /*LCD_ENABLE */\
	MUX_VAL(CP(DSS_DATA0),      (IDIS | PTD | DIS | M0)) /*DSS_DATA0 */\
	MUX_VAL(CP(DSS_DATA1),      (IDIS | PTD | DIS | M0)) /*DSS_DATA1 */\
	MUX_VAL(CP(DSS_DATA2),      (IDIS | PTD | DIS | M0)) /*DSS_DATA2 */\
	MUX_VAL(CP(DSS_DATA3),      (IDIS | PTD | DIS | M0)) /*DSS_DATA3 */\
	MUX_VAL(CP(DSS_DATA4),      (IDIS | PTD | DIS | M0)) /*DSS_DATA4 */\
	MUX_VAL(CP(DSS_DATA5),      (IDIS | PTD | DIS | M0)) /*DSS_DATA5 */\
	MUX_VAL(CP(DSS_DATA6),      (IDIS | PTD | DIS | M0)) /*DSS_DATA6 */\
	MUX_VAL(CP(DSS_DATA7),      (IDIS | PTD | DIS | M0)) /*DSS_DATA7 */\
	MUX_VAL(CP(DSS_DATA8),      (IDIS | PTD | DIS | M0)) /*DSS_DATA8*/\
	MUX_VAL(CP(DSS_DATA9),      (IDIS | PTD | DIS | M0)) /*DSS_DATA9*/\
	MUX_VAL(CP(DSS_DATA10),     (IDIS | PTD | DIS | M0)) /*DSS_DATA10*/\
	MUX_VAL(CP(DSS_DATA11),     (IDIS | PTD | DIS | M0)) /*DSS_DATA11*/\
	MUX_VAL(CP(DSS_DATA12),     (IDIS | PTD | DIS | M0)) /*DSS_DATA12*/\
	MUX_VAL(CP(DSS_DATA13),     (IDIS | PTD | DIS | M0)) /*DSS_DATA13*/\
	MUX_VAL(CP(DSS_DATA14),     (IDIS | PTD | DIS | M0)) /*DSS_DATA14*/\
	MUX_VAL(CP(DSS_DATA15),     (IDIS | PTD | DIS | M0)) /*DSS_DATA15*/\
	MUX_VAL(CP(DSS_DATA16),     (IDIS | PTD | DIS | M0)) /*DSS_DATA16*/\
	MUX_VAL(CP(DSS_DATA17),     (IDIS | PTD | DIS | M0)) /*DSS_DATA17*/\
	MUX_VAL(CP(DSS_DATA18),     (IDIS | PTD | DIS | M0)) /*DSS_DATA18*/\
	MUX_VAL(CP(DSS_DATA19),     (IDIS | PTD | DIS | M0)) /*DSS_DATA19*/\
	MUX_VAL(CP(DSS_DATA20),     (IDIS | PTD | DIS | M0)) /*DSS_DATA20*/\
	MUX_VAL(CP(DSS_DATA21),     (IDIS | PTD | DIS | M0)) /*DSS_DATA21*/\
	MUX_VAL(CP(DSS_DATA22),     (IDIS | PTD | DIS | M0)) /*DSS_DATA22*/\
	MUX_VAL(CP(DSS_DATA23),     (IDIS | PTD | DIS | M0)) /*DSS_DATA23*/\
	/*Audio Interface */\
	MUX_VAL(CP(McBSP2_FSX),     (IEN  | PTD | DIS | M0)) /*McBSP2_FSX*/\
	MUX_VAL(CP(McBSP2_CLKX),    (IEN  | PTD | DIS | M0)) /*McBSP2_CLKX*/\
	MUX_VAL(CP(McBSP2_DR),      (IEN  | PTD | DIS | M0)) /*McBSP2_DR*/\
	MUX_VAL(CP(McBSP2_DX),      (IDIS | PTD | DIS | M0)) /*McBSP2_DX*/\
	/*micro SD connector - expansion card  */\
	MUX_VAL(CP(MMC1_CLK),       (IEN  | PTD | DIS | M0)) /*MMC1_CLK */\
	MUX_VAL(CP(MMC1_CMD),       (IEN  | PTD | DIS | M0)) /*MMC1_CMD */\
	MUX_VAL(CP(MMC1_DAT0),      (IEN  | PTD | DIS | M0)) /*MMC1_DAT0*/\
	MUX_VAL(CP(MMC1_DAT1),      (IEN  | PTD | DIS | M0)) /*MMC1_DAT1*/\
	MUX_VAL(CP(MMC1_DAT2),      (IEN  | PTD | DIS | M0)) /*MMC1_DAT2*/\
	MUX_VAL(CP(MMC1_DAT3),      (IEN  | PTD | DIS | M0)) /*MMC1_DAT3*/\
	/* eMMC */\
	MUX_VAL(CP(MMC2_CLK),       (IEN  | PTD | DIS | M0)) /*MMC2_CLK */\
	MUX_VAL(CP(MMC2_CMD),       (IEN  | PTD | DIS | M0)) /*MMC2_CMD */\
	MUX_VAL(CP(MMC2_DAT0),      (IEN  | PTD | DIS | M0)) /*MMC2_DAT0*/\
	MUX_VAL(CP(MMC2_DAT1),      (IEN  | PTD | DIS | M0)) /*MMC2_DAT1*/\
	MUX_VAL(CP(MMC2_DAT2),      (IEN  | PTD | DIS | M0)) /*MMC2_DAT2*/\
	MUX_VAL(CP(MMC2_DAT3),      (IEN  | PTD | DIS | M0)) /*MMC2_DAT3*/\
	MUX_VAL(CP(MMC2_DAT4),      (IEN  | PTD | DIS | M0)) /*MMC2_DIR_DAT0*/\
	MUX_VAL(CP(MMC2_DAT5),      (IEN  | PTD | DIS | M0)) /*MMC2_DIR_DAT1*/\
	MUX_VAL(CP(MMC2_DAT6),      (IEN  | PTD | DIS | M0)) /*MMC2_DIR_CMD */\
	MUX_VAL(CP(MMC2_DAT7),      (IEN  | PTD | DIS | M0)) /*MMC2_CLKIN*/\
	/*MMC3 (WiFi)*/\
	MUX_VAL(CP(ETK_CTL_ES2),    (IEN  | PTU | EN  | M2)) /*MMC3_CMD */\
	MUX_VAL(CP(ETK_CLK_ES2),    (IEN  | PTU | EN  | M2)) /*MMC3_CLK */\
	MUX_VAL(CP(ETK_D4_ES2 ),    (IEN  | PTU | EN  | M2)) /*MMC3_DAT0 */\
	MUX_VAL(CP(ETK_D5_ES2 ),    (IEN  | PTU | EN  | M2)) /*MMC3_DAT1 */\
	MUX_VAL(CP(ETK_D6_ES2 ),    (IEN  | PTU | EN  | M2)) /*MMC3_DAT2 */\
	MUX_VAL(CP(ETK_D3_ES2 ),    (IEN  | PTU | EN  | M2)) /*MMC3_DAT3 */\
	/*Bluetooth*/\
	MUX_VAL(CP(McBSP3_DX),      (IEN  | PTU | EN  | M1)) /*UART2_CTS*/\
	MUX_VAL(CP(McBSP3_DR),      (IDIS | PTD | DIS | M1)) /*UART2_RTS*/\
	MUX_VAL(CP(McBSP3_CLKX),    (IDIS | PTD | DIS | M1)) /*UART2_TX*/\
	MUX_VAL(CP(McBSP3_FSX),     (IEN  | PTU | EN  | M1)) /*UART2_RX*/\
	/*Console Interface */\
	MUX_VAL(CP(UART1_TX),       (IDIS | PTD | DIS | M0)) /*UART1_TX*/\
	MUX_VAL(CP(UART1_RTS),      (IDIS | PTD | DIS | M0)) /*UART1_RTS*/ \
	MUX_VAL(CP(UART1_CTS),      (IEN  | PTU | EN  | M0)) /*UART1_CTS even if input for omap as pin is connected to test point*/ \
	MUX_VAL(CP(UART1_RX),       (WAKEUP_EN  | IEN  | PTU | EN  | M0)) /*UART1_RX*/\
	/* LCD-SPI */ \
	MUX_VAL(CP(McBSP1_CLKR),    (IEN  | PTD | DIS | M1)) /*McSPI4-CLK*/ \
	MUX_VAL(CP(McBSP1_DX),      (IDIS | PTD | DIS | M1)) /*McSPI4-SIMO*/ \
	MUX_VAL(CP(McBSP1_DR),      (IEN  | PTD | DIS | M1)) /*McSPI4-SOMI*/\
	MUX_VAL(CP(McBSP1_FSX),     (IDIS | PTD | DIS | M1)) /*McSPI4-CS0*/ \
	/*Serial Interface*/\
	MUX_VAL(CP(HSUSB0_CLK),     (IEN  | PTD | DIS | M0)) /*HSUSB0_CLK*/\
	MUX_VAL(CP(HSUSB0_STP),     (IDIS | PTU | DIS | M0)) /*HSUSB0_STP*/\
	MUX_VAL(CP(HSUSB0_DIR),     (IEN  | PTD | DIS | M0)) /*HSUSB0_DIR*/\
	MUX_VAL(CP(HSUSB0_NXT),     (IEN  | PTD | DIS | M0)) /*HSUSB0_NXT*/\
	MUX_VAL(CP(HSUSB0_DATA0),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA0 */\
	MUX_VAL(CP(HSUSB0_DATA1),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA1 */\
	MUX_VAL(CP(HSUSB0_DATA2),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA2 */\
	MUX_VAL(CP(HSUSB0_DATA3),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA3 */\
	MUX_VAL(CP(HSUSB0_DATA4),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA4 */\
	MUX_VAL(CP(HSUSB0_DATA5),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA5 */\
	MUX_VAL(CP(HSUSB0_DATA6),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA6 */\
	MUX_VAL(CP(HSUSB0_DATA7),   (IEN  | PTD | DIS | M0)) /*HSUSB0_DATA7 */\
	MUX_VAL(CP(I2C1_SCL),       (IEN  | PTU | DIS | M0)) /*I2C1_SCL*/\
	MUX_VAL(CP(I2C1_SDA),       (IEN  | PTU | DIS | M0)) /*I2C1_SDA*/\
	MUX_VAL(CP(I2C2_SCL),       (IEN  | PTU | DIS | M0)) /*I2C2_SCL*/\
	MUX_VAL(CP(I2C2_SDA),       (IEN  | PTU | DIS | M0)) /*I2C2_SDA*/\
	MUX_VAL(CP(I2C4_SCL),       (IEN  | PTU | DIS | M0)) /*I2C4_SCL*/\
	MUX_VAL(CP(I2C4_SDA),       (IEN  | PTU | DIS | M0)) /*I2C4_SDA*/\
	MUX_VAL(CP(McSPI1_CS3),     (IEN  | PTD | DIS | M3)) /*HSUSB2_D2*/\
	MUX_VAL(CP(McSPI2_CLK),     (IEN  | PTD | DIS | M3)) /*HSUSB2_D7*/\
	MUX_VAL(CP(McSPI2_SIMO),    (IEN  | PTD | DIS | M3)) /*HSUSB2_D4*/\
	MUX_VAL(CP(McSPI2_SOMI),    (IEN  | PTD | DIS | M3)) /*HSUSB2_D5*/\
	MUX_VAL(CP(McSPI2_CS0),     (IEN  | PTD | DIS | M3)) /*HSUSB2_D6*/\
	MUX_VAL(CP(McSPI2_CS1),     (IEN  | PTD | DIS | M3)) /*HSUSB2_D3*/\
	/*Control and debug */\
	MUX_VAL(CP(SYS_32K),        (IEN  | PTD | DIS | M0)) /*SYS_32K*/\
	MUX_VAL(CP(SYS_CLKREQ),     (IEN  | PTD | DIS | M0)) /*SYS_CLKREQ*/\
	MUX_VAL(CP(SYS_nIRQ),       (WAKEUP_EN  | IEN  | PTU | EN  | M0)) /*SYS_nIRQ 65921 Int*/\
	MUX_VAL(CP(SYS_BOOT0),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT0*/\
	MUX_VAL(CP(SYS_BOOT1),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT1*/\
	MUX_VAL(CP(SYS_BOOT2),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT2*/\
	MUX_VAL(CP(SYS_BOOT3),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT3*/\
	MUX_VAL(CP(SYS_BOOT4),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT4*/\
        MUX_VAL(CP(SYS_BOOT5),  (IEN  | PTD | DIS | M0)) /*SYS_BOOT5*/\
	MUX_VAL(CP(SYS_BOOT6),      (IEN  | PTD | DIS | M0)) /*SYS_BOOT6*/\
	MUX_VAL(CP(SYS_OFF_MODE),   (IDIS | PTD | DIS | M0)) /*SYS_OFF_MODE */\
	MUX_VAL(CP(SYS_CLKOUT2),    (IDIS | PTD | DIS | M0)) /*SYS_CLKOUT2	 */\
	MUX_VAL(CP(JTAG_nTRST),     (IEN  | PTD | DIS | M0)) /*JTAG_nTRST*/\
	MUX_VAL(CP(JTAG_TCK),       (IEN  | PTD | DIS | M0)) /*JTAG_TCK*/\
	MUX_VAL(CP(JTAG_TMS),       (IEN  | PTD | DIS | M0)) /*JTAG_TMS*/\
	MUX_VAL(CP(JTAG_TDI),       (IEN  | PTD | DIS | M0)) /*JTAG_TDI*/\
	MUX_VAL(CP(JTAG_TDO),  	    (IEN  | PTD | DIS | M0)) /*JTAG_TDO*/\
	MUX_VAL(CP(JTAG_RTCK),      (IEN  | PTD | DIS | M0)) /*JTAG_RTCK*/\
	MUX_VAL(CP(JTAG_EMU0),      (IEN  | PTD | DIS | M0)) /*JTAG_EMU0*/\
	MUX_VAL(CP(JTAG_EMU1),      (IEN  | PTD | DIS | M0)) /*JTAG_EMU1*/\
	MUX_VAL(CP(ETK_D10_ES2),    (IDIS | PTD | DIS | M3)) /*HSUSB2_CLK*/\
	MUX_VAL(CP(ETK_D11_ES2),    (IDIS | PTU | DIS | M3)) /*HSUSB2_STP*/\
	MUX_VAL(CP(ETK_D12_ES2),    (IEN  | PTD | DIS | M3)) /*HSUSB2_DIR*/\
	MUX_VAL(CP(ETK_D13_ES2),    (IEN  | PTD | DIS | M3)) /*HSUSB2_NXT*/\
	MUX_VAL(CP(ETK_D14_ES2),    (IEN  | PTD | DIS | M3)) /*HSUSB2_D0 */\
	MUX_VAL(CP(ETK_D15_ES2),    (IEN  | PTD | DIS | M3)) /*HSUSB2_D1 */\
	/* IO not connected muxed to default value to prevent spurious interrupts*/\
	MUX_VAL(CP(GPMC_nCS1),      (IEN | PTD | EN | M4)) /*GPIO-52 Not balled*/\
	MUX_VAL(CP(GPMC_nCS2),      (IEN | PTD | EN | M4)) /*GPIO-53 Not balled*/\
	MUX_VAL(CP(GPMC_nCS3),      (IEN | PTD | EN | M4)) /*GPIO-54 Not balled*/\
	MUX_VAL(CP(GPMC_nCS4),      (IEN | PTD | EN | M4)) /*GPIO-55 Not balled*/\
	MUX_VAL(CP(GPMC_nCS5),      (IEN | PTD | EN | M4)) /*GPIO-56 Not balled*/\
	MUX_VAL(CP(GPMC_nCS6),      (IEN | PTD | EN | M4)) /*GPIO-57 Not balled*/\
	MUX_VAL(CP(GPMC_WAIT1),     (IEN | PTD | EN | M4)) /*GPI0-63 Not balled*/\
	MUX_VAL(CP(GPMC_WAIT2),     (IEN | PTD | EN | M4)) /*GPI0-64 Not balled*/\
	MUX_VAL(CP(GPMC_WAIT3),     (IEN | PTD | EN | M4)) /*GPI0-65 Not balled*/\
        MUX_VAL(CP(CAM_STROBE), (IEN | PTD | EN | M4)) /*GPI0-126 Not balled*/\
        MUX_VAL(CP(CAM_WEN),    (IEN | PTD | EN | M4)) /*GPI0-167 Not balled*/\
	MUX_VAL(CP(CAM_HS),         (IEN | PTD | EN | M4)) /*GPIO-94 Not balled*/\
	MUX_VAL(CP(CAM_VS),         (IEN | PTD | EN | M4)) /*GPIO-95 Not balled*/\
	MUX_VAL(CP(CAM_PCLK),       (IEN | PTD | EN | M4)) /*GPIO-97 Not balled*/\
	MUX_VAL(CP(CAM_FLD),        (IEN | PTD | EN | M4)) /*GPIO-98 Not balled*/\
	MUX_VAL(CP(CAM_D6 ),        (IEN | PTD | EN | M4)) /*GPIO-105 Not balled*/\
	MUX_VAL(CP(CAM_D7 ),        (IEN | PTD | EN | M4)) /*GPIO-106 Not balled*/\
	MUX_VAL(CP(CAM_D8 ),        (IEN | PTD | EN | M4)) /*GPIO-107 Not balled*/\
	MUX_VAL(CP(CAM_D9 ),        (IEN | PTD | EN | M4)) /*GPIO-108 Not balled*/\
	MUX_VAL(CP(UART2_CTS),      (IEN | PTD | EN | M4)) /*GPIO-144 Not balled*/\
	MUX_VAL(CP(UART2_RTS),      (IEN | PTD | EN | M4)) /*GPIO-145 Not balled*/\
	MUX_VAL(CP(UART2_TX),       (IEN | PTD | EN | M4)) /*UPIO-146 Not balled*/\
	MUX_VAL(CP(UART2_RX),       (IEN | PTD | EN | M4)) /*GPIO-147 Not balled*/\
	/*McBSP4 Interface */\
	MUX_VAL(CP(McBSP4_CLKX),    (IEN | PTD | EN | M4)) /*GPIO-152 Not balled*/\
	MUX_VAL(CP(McBSP4_DR),      (IEN | PTD | EN | M4)) /*GPIO-153 Not balled*/\
	MUX_VAL(CP(McBSP4_FSX),     (IEN | PTD | EN | M4)) /*GPIO-154 Not balled*/\
	MUX_VAL(CP(McBSP4_DX),      (IEN | PTD | EN | M4)) /*GPIO-155 Not balled*/\
	MUX_VAL(CP(McBSP_CLKS),     (IEN | PTD | EN | M4)) /*GPIO-160 Not balled*/\
	MUX_VAL(CP(McBSP1_CLKX),    (IEN | PTD | EN | M4)) /*GPIO-162 Not balled*/\
	MUX_VAL(CP(UART3_CTS_RCTX), (IEN | PTD | EN | M4)) /*GPIO-163 Not balled*/\
	MUX_VAL(CP(UART3_RX_IRRX),  (IEN | PTD | EN | M4)) /*GPIO-164 Not balled*/\
	MUX_VAL(CP(UART3_RTS_SD),   (IEN | PTD | EN | M4)) /*GPIO-165 Not balled*/\
	MUX_VAL(CP(UART3_TX_IRTX),  (IEN | PTD | EN | M4)) /*GPIO-165 Not balled*/\
	/*I2C3 */\
	MUX_VAL(CP(I2C3_SCL),       (IEN | PTD | EN | M4)) /*GPIO_184 Not balled*/\
	MUX_VAL(CP(I2C3_SDA),       (IEN | PTD | EN | M4)) /*GPIO_185 Not balled*/\
	/*McSPI1 */\
	MUX_VAL(CP(McSPI1_SIMO),    (IEN | PTD  | EN | M4)) /*GPIO_171 Not balled*/\
	MUX_VAL(CP(McSPI1_CLK),     (IEN | PTD  | EN | M4)) /*GPIO_172 Not balled*/\
	MUX_VAL(CP(McSPI1_CS0),     (IEN | PTD  | EN | M4)) /*GPIO_173 Not balled*/\
	MUX_VAL(CP(McSPI1_SOMI),    (IEN | PTD  | EN | M4)) /*GPIO_174 Not balled*/\
	MUX_VAL(CP(McSPI1_CS1),     (IEN | PTD  | EN | M4)) /*GPIO_175 Not balled*/\
	MUX_VAL(CP(McSPI1_CS2),     (IEN | PTD  | EN | M4)) /*GPIO_176 Not balled*/\
	/*Die to Die */\
	MUX_VAL(CP(sdrc_cke0),      (IEN  | PTD | DIS | M0)) /*sdrc_cke0 */\
	MUX_VAL(CP(sdrc_cke1),      (IEN  | PTD | DIS | M0)) /*sdrc_cke1 not used*/\
	MUX_VAL(CP(d2d_mcad0),      (IEN  | PTD | EN  | M0)) /*d2d_mcad0*/\
	MUX_VAL(CP(d2d_mcad1),      (IEN  | PTD | EN  | M0)) /*d2d_mcad1*/\
	MUX_VAL(CP(d2d_mcad2),      (IEN  | PTD | EN  | M0)) /*d2d_mcad2*/\
	MUX_VAL(CP(d2d_mcad3),      (IEN  | PTD | EN  | M0)) /*d2d_mcad3*/\
	MUX_VAL(CP(d2d_mcad4),      (IEN  | PTD | EN  | M0)) /*d2d_mcad4*/\
	MUX_VAL(CP(d2d_mcad5),      (IEN  | PTD | EN  | M0)) /*d2d_mcad5*/\
	MUX_VAL(CP(d2d_mcad6),      (IEN  | PTD | EN  | M0)) /*d2d_mcad6*/\
	MUX_VAL(CP(d2d_mcad7),      (IEN  | PTD | EN  | M0)) /*d2d_mcad7*/\
	MUX_VAL(CP(d2d_mcad8),      (IEN  | PTD | EN  | M0)) /*d2d_mcad8*/\
	MUX_VAL(CP(d2d_mcad8),      (IEN  | PTD | EN  | M0)) /*d2d_mcad9*/\
	MUX_VAL(CP(d2d_mcad10),     (IEN  | PTD | EN  | M0)) /*d2d_mcad10*/\
	MUX_VAL(CP(d2d_mcad11),     (IEN  | PTD | EN  | M0)) /*d2d_mcad11*/\
	MUX_VAL(CP(d2d_mcad12),     (IEN  | PTD | EN  | M0)) /*d2d_mcad12*/\
	MUX_VAL(CP(d2d_mcad13),     (IEN  | PTD | EN  | M0)) /*d2d_mcad13*/\
	MUX_VAL(CP(d2d_mcad14),     (IEN  | PTD | EN  | M0)) /*d2d_mcad14*/\
	MUX_VAL(CP(d2d_mcad15),     (IEN  | PTD | EN  | M0)) /*d2d_mcad15*/\
	MUX_VAL(CP(d2d_mcad16),     (IEN  | PTD | EN  | M0)) /*d2d_mcad16*/\
	MUX_VAL(CP(d2d_mcad17),     (IEN  | PTD | EN  | M0)) /*d2d_mcad17*/\
	MUX_VAL(CP(d2d_mcad18),     (IEN  | PTD | EN  | M0)) /*d2d_mcad18*/\
	MUX_VAL(CP(d2d_mcad19),     (IEN  | PTD | EN  | M0)) /*d2d_mcad19*/\
	MUX_VAL(CP(d2d_mcad20),     (IEN  | PTD | EN  | M0)) /*d2d_mcad20*/\
	MUX_VAL(CP(d2d_mcad21),     (IEN  | PTD | EN  | M0)) /*d2d_mcad21*/\
	MUX_VAL(CP(d2d_mcad22),     (IEN  | PTD | EN  | M0)) /*d2d_mcad22*/\
	MUX_VAL(CP(d2d_mcad23),     (IEN  | PTD | EN  | M0)) /*d2d_mcad23*/\
	MUX_VAL(CP(d2d_mcad24),     (IEN  | PTD | EN  | M0)) /*d2d_mcad24*/\
	MUX_VAL(CP(d2d_mcad25),     (IEN  | PTD | EN  | M0)) /*d2d_mcad25*/\
	MUX_VAL(CP(d2d_mcad26),     (IEN  | PTD | EN  | M0)) /*d2d_mcad26*/\
	MUX_VAL(CP(d2d_mcad27),     (IEN  | PTD | EN  | M0)) /*d2d_mcad27*/\
	MUX_VAL(CP(d2d_mcad28),     (IEN  | PTD | EN  | M0)) /*d2d_mcad28*/\
	MUX_VAL(CP(d2d_mcad29),     (IEN  | PTD | EN  | M0)) /*d2d_mcad29*/\
	MUX_VAL(CP(d2d_mcad30),     (IEN  | PTD | EN  | M0)) /*d2d_mcad30*/\
	MUX_VAL(CP(d2d_mcad31),     (IEN  | PTD | EN  | M0)) /*d2d_mcad31*/\
	MUX_VAL(CP(d2d_mcad32),     (IEN  | PTD | EN  | M0)) /*d2d_mcad32*/\
	MUX_VAL(CP(d2d_mcad33),     (IEN  | PTD | EN  | M0)) /*d2d_mcad33*/\
	MUX_VAL(CP(d2d_mcad34),     (IEN  | PTD | EN  | M0)) /*d2d_mcad34*/\
	MUX_VAL(CP(d2d_mcad35),     (IEN  | PTD | EN  | M0)) /*d2d_mcad35*/\
	MUX_VAL(CP(d2d_mcad36),     (IEN  | PTD | EN  | M0)) /*d2d_mcad36*/\
	MUX_VAL(CP(d2d_clk26mi),    (IEN  | PTD | EN  | M0)) /*d2d_clk26msi*/\
	MUX_VAL(CP(d2d_nrespwron),  (IEN  | PTD | DIS | M0)) /*d2d_nrespwron*/\
	MUX_VAL(CP(d2d_nreswarm),   (IEN  | PTU | EN  | M0)) /*d2d_nreswarm*/\
	MUX_VAL(CP(d2d_arm9nirq),   (IEN  | PTD | DIS | M0)) /*d2d_arm9nirq*/\
	MUX_VAL(CP(d2d_uma2p6fiq),  (IEN  | PTD | DIS | M0)) /*d2d_uma2p6fi*/\
	MUX_VAL(CP(d2d_spint),      (IEN  | PTD | EN  | M0)) /*d2d_spint*/\
	MUX_VAL(CP(d2d_frint),      (IEN  | PTD | EN  | M0)) /*d2d_clk26msi*/\
	MUX_VAL(CP(d2d_dmareq0),    (IEN  | PTD | EN  | M0)) /*d2d_dmareq0*/\
	MUX_VAL(CP(d2d_dmareq1),    (IEN  | PTD | EN  | M0)) /*d2d_dmareq1*/\
	MUX_VAL(CP(d2d_dmareq2),    (IEN  | PTD | EN  | M0)) /*d2d_dmareq2*/\
	MUX_VAL(CP(d2d_dmareq3),    (IEN  | PTD | EN  | M0)) /*d2d_dmareq3*/\
	MUX_VAL(CP(d2d_n3gtrst),    (IEN  | PTD | EN  | M0)) /*d2d_n3gtrst*/\
	MUX_VAL(CP(d2d_n3gtdi),     (IEN  | PTD | EN  | M0)) /*d2d_n3gtdi*/\
	MUX_VAL(CP(d2d_n3gtdo),     (IEN  | PTD | EN  | M0)) /*d2d_n3gtdo*/\
	MUX_VAL(CP(d2d_n3gtms),     (IEN  | PTD | EN  | M0)) /*d2d_n3gtms*/\
	MUX_VAL(CP(d2d_n3gtck),     (IEN  | PTD | DIS | M0)) /*d2d_n3gtck*/\
	MUX_VAL(CP(d2d_n3grtck),    (IEN  | PTD | EN  | M0)) /*d2d_rtck*/\
	MUX_VAL(CP(d2d_mstdby),     (IEN  | PTU | EN  | M0)) /*d2d_mstdby*/\
	MUX_VAL(CP(d2d_idlereq),    (IEN  | PTD | EN  | M0)) /*d2d_idlereq*/\
	MUX_VAL(CP(d2d_idleack),    (IEN  | PTU | EN  | M0)) /*d2d_idleack*/\
	MUX_VAL(CP(d2d_mwrite),     (IEN  | PTD | EN  | M0)) /*d2d_mwrite*/\
	MUX_VAL(CP(d2d_swrite),     (IEN  | PTD | EN  | M0)) /*d2d_swrite*/\
	MUX_VAL(CP(d2d_mread),      (IEN  | PTD | EN  | M0)) /*d2d_mread*/\
	MUX_VAL(CP(d2d_sread),      (IEN  | PTD | EN  | M0)) /*d2d_sread*/\
	MUX_VAL(CP(d2d_mbusflag),   (IEN  | PTD | EN  | M0)) /*d2d_mbusflag*/\
	MUX_VAL(CP(d2d_sbusflag),   (IEN  | PTD | EN  | M0)) /*d2d_sbusflag*/

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	MUX_EVT1A();
}


/******************************************************************************
 * Routine: get_boot_device()
 * Description:Sets the "bootdevice" variable with the name of the device from
 *             which the device booted.
 *****************************************************************************/
void get_boot_device(void)
{
	u32 boot_device = __raw_readl(0x480029c0) & 0xff;
	const char * boot_dev;
	switch(boot_device)
	{
	  case 2: boot_dev = "NAND"; break;
	  case 3: boot_dev = "OneNAND"; break;
	  case 5: boot_dev = "eMMC"; break;
	  case 6: boot_dev = "SD"; break;
	  default:
		boot_dev = "unknown";
		printf("Error, no valid booting device found for board\n");
		break;
	}
	setenv ("bootdevice",(char *)boot_dev);
	printf("Booting from %s\n", boot_dev);
}
