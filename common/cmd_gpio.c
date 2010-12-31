
/*
 * GIO Test Utilities
*/

#include <common.h>
#include <command.h>

#include "power/general.h"
#include "power/gpio.h"
#include "power/omap_cm_regs.h"
#include "power/omap_gpio_regs.h"

/* ========================================================================== */
/**
*  @fn  power_led(void)    function_description
*
*  @see  gpio.h
*/
/* ========================================================================== */
void power_led(void)
{
	gpio_pin_init(6, GPIO_OUTPUT, GPIO_LOW);

        gpio_pin_init(3, GPIO_OUTPUT, GPIO_LOW);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_HIGH);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_LOW);

        gpio_pin_init(3, GPIO_OUTPUT, GPIO_LOW);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_HIGH);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_LOW);

        gpio_pin_init(3, GPIO_OUTPUT, GPIO_HIGH);	// yellow led
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_HIGH);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_LOW);

        gpio_pin_init(3, GPIO_OUTPUT, GPIO_HIGH);	// green led
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_HIGH);
        gpio_pin_init(2, GPIO_OUTPUT, GPIO_LOW);

        gpio_pin_init(6, GPIO_OUTPUT, GPIO_HIGH);
        gpio_pin_init(6, GPIO_OUTPUT, GPIO_LOW);
}

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
/* ========================================================================== */
/**
*  @fn  gpio_module_init()    function_description
*
*  @see  gpio.h
*/
/* ========================================================================== */
int gpio_module_init(u32 gpioModule)
{
int        retVal = 0;
static u8  gpioModuleInitFlags = 0;
vu32       *gpioRegs[6] = {(vu32*)GPIO1_BASE, (vu32*)GPIO2_BASE,
                           (vu32*)GPIO3_BASE, (vu32*)GPIO4_BASE,
                           (vu32*)GPIO5_BASE, (vu32*)GPIO6_BASE};

    /* GPIO module not init yet? */
    if (!(gpioModuleInitFlags & (1 << gpioModule)))
    {
        vu32  *gpioCMregs;
        u32   fclkenReg;
        u32   iclkenReg;
        u32   gpioEnMask;
        u32   timeoutCnt;

        switch (gpioModule)
        {
            case 0:
                gpioCMregs = (vu32*)WKUP_CM_BASE;
                fclkenReg  = WKUP_CM_FCLKEN;
                iclkenReg  = WKUP_CM_ICLKEN;
                gpioEnMask = WKUP_CM_EN_GPIO1_MASK;
                break;
            case 1:
                gpioCMregs = (vu32*)PER_CM_BASE;
                fclkenReg  = PER_CM_FCLKEN;
                iclkenReg  = PER_CM_ICLKEN;
                gpioEnMask = PER_CM_EN_GPIO2_MASK;
                break;
            case 2:
                gpioCMregs = (vu32*)PER_CM_BASE;
                fclkenReg  = PER_CM_FCLKEN;
                iclkenReg  = PER_CM_ICLKEN;
                gpioEnMask = PER_CM_EN_GPIO3_MASK;
                break;
            case 3:
                gpioCMregs = (vu32*)PER_CM_BASE;
                fclkenReg  = PER_CM_FCLKEN;
                iclkenReg  = PER_CM_ICLKEN;
                gpioEnMask = PER_CM_EN_GPIO4_MASK;
                break;
            case 4:
                gpioCMregs = (vu32*)PER_CM_BASE;
                fclkenReg  = PER_CM_FCLKEN;
                iclkenReg  = PER_CM_ICLKEN;
                gpioEnMask = PER_CM_EN_GPIO5_MASK;
                break;
            case 5:
                gpioCMregs = (vu32*)PER_CM_BASE;
                fclkenReg  = PER_CM_FCLKEN;
                iclkenReg  = PER_CM_ICLKEN;
                gpioEnMask = PER_CM_EN_GPIO6_MASK;
            default:
                retVal = -1;
                goto EXIT;
        }

        /* Enable GPIO module FCLK */
        gpioCMregs[fclkenReg] |= gpioEnMask;
        /* Enable GPIO module ICLK */
        gpioCMregs[iclkenReg] |= gpioEnMask;

        /* GPIO module reset */
        gpioRegs[gpioModule][GPIO_SYSCONFIG] |= GPIO_SOFTRESET_MASK;
        for (timeoutCnt = GPIO_HW_TIMEOUT; timeoutCnt; timeoutCnt--)
        {
            if (gpioRegs[gpioModule][GPIO_SYSSTATUS] & GPIO_RESETDONE_MASK)
            {
                break;
            }
        }
        if (timeoutCnt == 0)
        {
            retVal = -1;
            goto EXIT;
        }

        gpioModuleInitFlags |= (1 << gpioModule);
    }

EXIT:
    return retVal;
}


/* ========================================================================== */
/**
*  @fn  gpio_pin_init()    function_description
*
*  @see  gpio.h
*/
/* ========================================================================== */
int gpio_pin_init(u32 gpio_pin, gpio_dir_t gpio_dir, gpio_level_t gpio_level)
{
int        retVal = 0;
static u8  gpioModuleInitFlags = 0;
u32        gpioModule;
vu32       *gpioRegs[6] = {(vu32*)GPIO1_BASE, (vu32*)GPIO2_BASE,
                           (vu32*)GPIO3_BASE, (vu32*)GPIO4_BASE,
                           (vu32*)GPIO5_BASE, (vu32*)GPIO6_BASE};

    if (gpio_pin > GPIOs_COUNT)
    {
        retVal = -1;
        goto EXIT;
    }

    gpioModule = gpio_pin >> 5;

    gpio_module_init(gpioModule);    /* ensure gpio module is initialized */

    if (gpio_dir == GPIO_OUTPUT)
    {
        gpioRegs[gpioModule][GPIO_OE] &= ~GPIO_MASK(gpio_pin);
        gpio_pin_write(gpio_pin, gpio_level);
    }
    else
    {
        gpioRegs[gpioModule][GPIO_OE] |= GPIO_MASK(gpio_pin);
    }

EXIT:
    return retVal;
}


/* ========================================================================== */
/**
*  @fn  gpio_pin_write()    function_description
*
*  @see  gpio.h
*/
/* ========================================================================== */
int gpio_pin_write(u32 gpio_pin, gpio_level_t gpio_level)
{
int   retVal = 0;
u32   gpioModule;
vu32  *gpioRegs[6] = {(vu32*)GPIO1_BASE, (vu32*)GPIO2_BASE,
                      (vu32*)GPIO3_BASE, (vu32*)GPIO4_BASE,
                      (vu32*)GPIO5_BASE, (vu32*)GPIO6_BASE};

    if (gpio_pin > GPIOs_COUNT)
    {
        retVal = -1;
        goto EXIT;
    }

    gpioModule = gpio_pin >> 5;

    if (gpio_level == GPIO_LOW)
	{
        gpioRegs[gpioModule][GPIO_DATAOUT] &= ~GPIO_MASK(gpio_pin);
	}
    else
    {
        gpioRegs[gpioModule][GPIO_DATAOUT] |= GPIO_MASK(gpio_pin);
    }

EXIT:
    return retVal;
}

/* ========================================================================== */
/**
*  @fn  gpio_pin_status()    function_description
*
*  @see  gpio.h
*/
/* ========================================================================== */
int gpio_pin_status(u32 gpio_pin)
{
int   retVal = 0;
u32   gpioModule;
vu32  *gpioRegs[6] = {(vu32*)GPIO1_BASE, (vu32*)GPIO2_BASE,
                      (vu32*)GPIO3_BASE, (vu32*)GPIO4_BASE,
                      (vu32*)GPIO5_BASE, (vu32*)GPIO6_BASE};

    if (gpio_pin > GPIOs_COUNT)
    {
        printf("Invalid GPIO number\n");
	retVal = -1;
        goto EXIT;
    }

    gpioModule = gpio_pin >> 5;

    gpio_module_init(gpioModule);    /* ensure gpio module is initialized */

   
    printf("  GPIO %3d: %s %s\n", gpio_pin,
	(gpioRegs[gpioModule][GPIO_OE] & GPIO_MASK(gpio_pin)) ? "IN" : "OUT", 
	(gpioRegs[gpioModule][GPIO_DATAIN] & GPIO_MASK(gpio_pin)) ? "1" : "0");

   if (gpioRegs[gpioModule][GPIO_DATAIN] & GPIO_MASK(gpio_pin))
   	retVal=1;
EXIT:
    return retVal;
}

int gpio_pin_read(u32 gpio_pin)
{
int   retVal = 0;
u32   gpioModule;
vu32  *gpioRegs[6] = {(vu32*)GPIO1_BASE, (vu32*)GPIO2_BASE,
                      (vu32*)GPIO3_BASE, (vu32*)GPIO4_BASE,
                      (vu32*)GPIO5_BASE, (vu32*)GPIO6_BASE};

    if (gpio_pin > GPIOs_COUNT)
    {
        printf("Invalid GPIO number\n");
	    retVal = -1;
        goto EXIT;
    }

    gpioModule = gpio_pin >> 5;

    gpio_module_init(gpioModule);    /* ensure gpio module is initialized */

    if (gpioRegs[gpioModule][GPIO_DATAIN] & GPIO_MASK(gpio_pin))
        retVal=1;
EXIT:
    return retVal;
}

int do_gpio(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int rcode = 0;
	ulong pin;

	switch (argc) {
	case 3:			/* set pin */
		pin = simple_strtoul(argv[1], NULL, 10);
		if (strcmp(argv[2],"in") == 0) {
			gpio_pin_init(pin, GPIO_INPUT, 0);
		} else if ((argv[2][0] == 'h') || (argv[2][0] == '1')) {
			gpio_pin_init(pin, GPIO_OUTPUT, 1);
		} else if ((argv[2][0] == 'l') || (argv[2][0] == '0')) {
			gpio_pin_init(pin, GPIO_OUTPUT, 0);
		} else {
			printf ("Usage:\n%s\n", cmdtp->usage);
			printf ("%s\n", cmdtp->help);
			rcode = 1;
			break;
		}
		/* FALL TROUGH */
	case 2:			/* show pin status */
		pin = simple_strtoul(argv[1], NULL, 10);
		gpio_pin_status(pin);
		return 0;
	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		rcode = 1;
	}
	return rcode;
}

U_BOOT_CMD(
	gpio,	3,	1,	do_gpio,
	"gpio   - set/display gpio pins\n",
	"num [in|low|high|0|1]\n"
);

