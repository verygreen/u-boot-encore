/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  omap_cm_regs.h
*
*  OMAP3 Clock Manager registers and bitmasks
*
*  @author  Pavel Nedev
*
*  @date  19/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _OMAP_CM_REGS_H
#define _OMAP_CM_REGS_H

/******************************************************************************
 * BASE ADDRESSES
 ******************************************************************************/
#define CORE_CM_BASE     0x48004A00
#define WKUP_CM_BASE     0x48004C00
#define CLOCK_CM_BASE    0x48004D00
#define DSS_CM_BASE      0x48004E00
#define CAM_CM_BASE      0x48004F00
#define PER_CM_BASE      0x48005000



/******************************************************************************
 * REGISTERS AND BITMASKS
 ******************************************************************************/

/******************************************************************************
 * CORE_CM_BASE REGISTERS
 ******************************************************************************/

/******************************************************************************
 * CORE_CM_FCLKEN1, CORE_CM_ICLKEN1, CORE_CM_IDLEST1 REGISTERS
 ******************************************************************************/
#define CORE_CM_FCLKEN1    (0x00 >> 2)
#define CORE_CM_ICLKEN1    (0x10 >> 2)
#define CORE_CM_IDLEST1    (0x20 >> 2)

#define CORE_CM_EN_I2C3_BITPOS    17
#define CORE_CM_EN_I2C3_MASK      (1 << 17)
#define CORE_CM_EN_I2C2_BITPOS    16
#define CORE_CM_EN_I2C2_MASK      (1 << 16)
#define CORE_CM_EN_I2C1_BITPOS    15
#define CORE_CM_EN_I2C1_MASK      (1 << 15)



/******************************************************************************
 * WKUP_CM_BASE REGISTERS
 ******************************************************************************/

/******************************************************************************
 * WKUP_CM_FCLKEN, WKUP_CM_ICLKEN, WKUP_CM_IDLEST REGISTERS
 ******************************************************************************/
#define WKUP_CM_FCLKEN    (0x00 >> 2)
#define WKUP_CM_ICLKEN    (0x10 >> 2)
#define WKUP_CM_IDLEST    (0x20 >> 2)

#define WKUP_CM_EN_GPIO1_BITPOS    3
#define WKUP_CM_EN_GPIO1_MASK      (1 << 3)



/******************************************************************************
 * DSS_CM_BASE REGISTERS
 ******************************************************************************/

/******************************************************************************
 * DSS_CM_FCLKEN REGISTER
 ******************************************************************************/
#define DSS_CM_FCLKEN    (0x00 >> 2)

#define DSS_CM_EN_TV_BITPOS      2
#define DSS_CM_EN_TV_MASK        (1 << 2)
#define DSS_CM_EN_DSS2_BITPOS    1
#define DSS_CM_EN_DSS2_MASK      (1 << 1)
#define DSS_CM_EN_DSS1_BITPOS    0
#define DSS_CM_EN_DSS1_MASK      (1 << 0)



/******************************************************************************
 * DSS_CM_ICLKEN REGISTER
 ******************************************************************************/
#define DSS_CM_ICLKEN    (0x10 >> 2)

#define DSS_CM_EN_DSS_BITPOS    0
#define DSS_CM_EN_DSS_MASK      (1 << 0)



/******************************************************************************
 * DSS_CM_IDLEST REGISTER
 ******************************************************************************/
#define DSS_CM_IDLEST    (0x20 >> 2)

#define DSS_CM_ST_DSS_IDLE_BITPOS     1
#define DSS_CM_ST_DSS_IDLE_MASK       (1 << 1)
#define DSS_CM_ST_DSS_STDBY_BITPOS    0
#define DSS_CM_ST_DSS_STDBY_MASK      (1 << 0)



/******************************************************************************
 * DSS_CM_CLKSEL REGISTER
 ******************************************************************************/
#define DSS_CM_CLKSEL    (0x40 >> 2)

#define DSS_CM_CLKSEL_TV_BITPOS      8
#define DSS_CM_CLKSEL_TV_MASK        (0x1F << 8)
#define DSS_CM_CLKSEL_DSS1_BITPOS    0
#define DSS_CM_CLKSEL_DSS1_MASK      (0x1F << 0)



/******************************************************************************
 * CAM_CM_BASE REGISTERS
 ******************************************************************************/

/******************************************************************************
 * CAM_CM_FCLKEN, CAM_CM_ICLKEN, CAM_CM_IDLEST REGISTERS
 ******************************************************************************/
#define CAM_CM_FCLKEN    (0x00 >> 2)
#define CAM_CM_ICLKEN    (0x10 >> 2)
#define CAM_CM_IDLEST    (0x20 >> 2)

#define CAM_CM_EN_CSI2_BITPOS    1
#define CAM_CM_EN_CSI2_MASK      (1 << 1)
#define CAM_CM_EN_CAM_BITPOS     0
#define CAM_CM_EN_CAM_MASK       (1 << 0)



/******************************************************************************
 * CAM_CM_CLKSEL REGISTER
 ******************************************************************************/
#define CAM_CM_CLKSEL    (0x40 >> 2)

#define CAM_CM_CLKSEL_CAM_BITPOS    0
#define CAM_CM_CLKSEL_CAM_MASK      (0x1F << 0)



/******************************************************************************
 * PER_CM_BASE REGISTERS
 ******************************************************************************/

/******************************************************************************
 * PER_CM_FCLKEN, PER_CM_ICLKEN, PER_CM_IDLEST REGISTERS
 ******************************************************************************/
#define PER_CM_FCLKEN    (0x00 >> 2)
#define PER_CM_ICLKEN    (0x10 >> 2)
#define PER_CM_IDLEST    (0x20 >> 2)

#define PER_CM_EN_GPIO6_BITPOS    17
#define PER_CM_EN_GPIO6_MASK      (1 << 17)
#define PER_CM_EN_GPIO5_BITPOS    16
#define PER_CM_EN_GPIO5_MASK      (1 << 16)
#define PER_CM_EN_GPIO4_BITPOS    15
#define PER_CM_EN_GPIO4_MASK      (1 << 15)
#define PER_CM_EN_GPIO3_BITPOS    14
#define PER_CM_EN_GPIO3_MASK      (1 << 14)
#define PER_CM_EN_GPIO2_BITPOS    13
#define PER_CM_EN_GPIO2_MASK      (1 << 13)

#endif /* _OMAP_CM_REGS_H */
