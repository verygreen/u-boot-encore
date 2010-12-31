/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  omap_gpio_regs.h
*
*  OMAP3 GPIO registers and bitmasks
*
*  @author  Pavel Nedev
*
*  @date  19/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _OMAP_GPIO_REGS_H
#define _OMAP_GPIO_REGS_H

/******************************************************************************
 * BASE ADDRESSES
 ******************************************************************************/
#define GPIO1_BASE    0x48310000
#define GPIO2_BASE    0x49050000
#define GPIO3_BASE    0x49052000
#define GPIO4_BASE    0x49054000
#define GPIO5_BASE    0x49056000
#define GPIO6_BASE    0x49058000



/******************************************************************************
 * REGISTERS AND BITMASKS
 ******************************************************************************/

/******************************************************************************
 * GPIO_SYSCONFIG REGISTER
 ******************************************************************************/
#define GPIO_SYSCONFIG    (0x10 >> 2)

#define GPIO_IDLEMODE_BITPOS     3
#define GPIO_IDLEMODE_MASK       (3 << 3)
#define GPIO_ENAWAKEUP_BITPOS    2
#define GPIO_ENAWAKEUP_MASK      (1 << 2)
#define GPIO_SOFTRESET_BITPOS    1
#define GPIO_SOFTRESET_MASK      (1 << 1)
#define GPIO_AUTOIDLE_BITPOS     0
#define GPIO_AUTOIDLE_MASK       (1 << 0)



/******************************************************************************
 * GPIO_SYSSTATUS REGISTER
 ******************************************************************************/
#define GPIO_SYSSTATUS    (0x14 >> 2)

#define GPIO_RESETDONE_BITPOS    0
#define GPIO_RESETDONE_MASK      (1 << 0)



/******************************************************************************
 * GPIO_CTRL REGISTER
 ******************************************************************************/
#define GPIO_CTRL    (0x30 >> 2)

#define GPIO_GATINGRATIO_BITPOS      1
#define GPIO_GATINGRATIO_MASK        (3 << 1)
#define GPIO_DISABLEMODULE_BITPOS    0
#define GPIO_DISABLEMODULE_MASK      (1 << 0)



/******************************************************************************
 * GPIO_OE, GPIO_DATAIN, GPIO_DATAOUT, GPIO_CLEARDATAOUT, GPIO_SETDATAOUT
 * REGISTERS
 ******************************************************************************/
#define GPIO_OE              (0x34 >> 2)
#define GPIO_DATAIN          (0x38 >> 2)
#define GPIO_DATAOUT         (0x3C >> 2)
#define GPIO_CLEARDATAOUT    (0x90 >> 2)
#define GPIO_SETDATAOUT      (0x94 >> 2)

#define GPIO_BITPOS(num)    ((num) & 0x1F)
#define GPIO_MASK(num)      (1 << ((num) & 0x1F))

#endif /* _OMAP_GPIO_REGS_H */
