/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  omap_i2c_regs.h
*
*  OMAP3 I2C registers and bitmasks
*
*  @author  Pavel Nedev
*
*  @date  19/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _OMAP_I2C_REGS_H
#define _OMAP_I2C_REGS_H

/******************************************************************************
 * BASE ADDRESSES
 ******************************************************************************/
#define I2C1_BASE    0x48070000
#define I2C2_BASE    0x48072000
#define I2C3_BASE    0x48060000



/******************************************************************************
 * REGISTERS AND BITMASKS
 ******************************************************************************/

/******************************************************************************
 * I2C_IE REGISTER
 ******************************************************************************/
#define I2C_IE    (0x4 >> 1)

#define I2C_XDR_IE_BITPOS     14
#define I2C_XDR_IE_MASK       (1 << 14)
#define I2C_RDR_IE_BITPOS     13
#define I2C_RDR_IE_MASK       (1 << 13)
#define I2C_AAS_IE_BITPOS     9
#define I2C_AAS_IE_MASK       (1 << 9)
#define I2C_BF_IE_BITPOS      8
#define I2C_BF_IE_MASK        (1 << 8)
#define I2C_AERR_IE_BITPOS    7
#define I2C_AERR_IE_MASK      (1 << 7)
#define I2C_STC_IE_BITPOS     6
#define I2C_STC_IE_MASK       (1 << 6)
#define I2C_GC_IE_BITPOS      5
#define I2C_GC_IE_MASK        (1 << 5)
#define I2C_XRDY_IE_BITPOS    4
#define I2C_XRDY_IE_MASK      (1 << 4)
#define I2C_RRDY_IE_BITPOS    3
#define I2C_RRDY_IE_MASK      (1 << 3)
#define I2C_ARDY_IE_BITPOS    2
#define I2C_ARDY_IE_MASK      (1 << 2)
#define I2C_NACK_IE_BITPOS    1
#define I2C_NACK_IE_MASK      (1 << 1)
#define I2C_AL_IE_BITPOS      0
#define I2C_AL_IE_MASK        (1 << 0)



/******************************************************************************
 * I2C_STAT REGISTER
 ******************************************************************************/
#define I2C_STAT    (0x8 >> 1)

#define I2C_XDR_BITPOS     14
#define I2C_XDR_MASK       (1 << 14)
#define I2C_RDR_BITPOS     13
#define I2C_RDR_MASK       (1 << 13)
#define I2C_BB_BITPOS      12
#define I2C_BB_MASK        (1 << 12)
#define I2C_ROVR_BITPOS    11
#define I2C_ROVR_MASK      (1 << 11)
#define I2C_XUDF_BITPOS    10
#define I2C_XUDF_MASK      (1 << 10)
#define I2C_AAS_BITPOS     9
#define I2C_AAS_MASK       (1 << 9)
#define I2C_BF_BITPOS      8
#define I2C_BF_MASK        (1 << 8)
#define I2C_AERR_BITPOS    7
#define I2C_AERR_MASK      (1 << 7)
#define I2C_STC_BITPOS     6
#define I2C_STC_MASK       (1 << 6)
#define I2C_GC_BITPOS      5
#define I2C_GC_MASK        (1 << 5)
#define I2C_XRDY_BITPOS    4
#define I2C_XRDY_MASK      (1 << 4)
#define I2C_RRDY_BITPOS    3
#define I2C_RRDY_MASK      (1 << 3)
#define I2C_ARDY_BITPOS    2
#define I2C_ARDY_MASK      (1 << 2)
#define I2C_NACK_BITPOS    1
#define I2C_NACK_MASK      (1 << 1)
#define I2C_AL_BITPOS      0
#define I2C_AL_MASK        (1 << 0)



/******************************************************************************
 * I2C_SYSS REGISTER
 ******************************************************************************/
#define I2C_SYSS    (0x10 >> 1)

#define I2C_RDONE_BITPOS    0
#define I2C_RDONE_MASK      (1 << 0)



/******************************************************************************
 * I2C_BUF REGISTER
 ******************************************************************************/
#define I2C_BUF    (0x14 >> 1)

#define I2C_RDMA_EN_BITPOS       15
#define I2C_RDMA_EN_MASK         (1 << 15)
#define I2C_RXFIFO_CLR_BITPOS    14
#define I2C_RXFIFO_CLR_MASK      (1 << 14)
#define I2C_RTRSH_BITPOS         8
#define I2C_RTRSH_MASK           (0x3F << 8)
#define I2C_XDMA_EN_BITPOS       7
#define I2C_XDMA_EN_MASK         (1 << 7)
#define I2C_TXFIFO_CLR_BITPOS    6
#define I2C_TXFIFO_CLR_MASK      (1 << 6)
#define I2C_XTRSH_BITPOS         0
#define I2C_XTRSH_MASK           (0x3F << 0)



/******************************************************************************
 * I2C_CNT REGISTER
 ******************************************************************************/
#define I2C_CNT    (0x18 >> 1)

#define I2C_DCOUNT_BITPOS    0
#define I2C_DCOUNT_MASK      (0xFFFF << 0)



/******************************************************************************
 * I2C_DATA REGISTER
 ******************************************************************************/
#define I2C_DATA    (0x1C >> 1)

#define I2C_DATA_BITPOS    0
#define I2C_DATA_MASK      (0xFF << 0)



/******************************************************************************
 * I2C_SYSC REGISTER
 ******************************************************************************/
#define I2C_SYSC    (0x20 >> 1)

#define I2C_CLOCKACTIVITY_BITPOS    8
#define I2C_CLOCKACTIVITY_MASK      (3 << 8)
#define I2C_IDLEMODE_BITPOS         3
#define I2C_IDLEMODE_MASK           (3 << 3)
#define I2C_ENAWAKEUP_BITPOS        2
#define I2C_ENAWAKEUP_MASK          (1 << 2)
#define I2C_SRST_BITPOS             1
#define I2C_SRST_MASK               (1 << 1)
#define I2C_AUTOIDLE_BITPOS         0
#define I2C_AUTOIDLE_MASK           (1 << 0)



/******************************************************************************
 * I2C_CON REGISTER
 ******************************************************************************/
#define I2C_CON    (0x24 >> 1)

#define I2C_EN_BITPOS        15
#define I2C_EN_MASK          (1 << 15)
#define I2C_OPMODE_BITPOS    12
#define I2C_OPMODE_MASK      (3 << 12)
#define I2C_STB_BITPOS       11
#define I2C_STB_MASK         (1 << 11)
#define I2C_MST_BITPOS       10
#define I2C_MST_MASK         (1 << 10)
#define I2C_TRX_BITPOS       9
#define I2C_TRX_MASK         (1 << 9)
#define I2C_XSA_BITPOS       8
#define I2C_XSA_MASK         (1 << 8)
#define I2C_XOA0_BITPOS      7
#define I2C_XOA0_MASK        (1 << 7)
#define I2C_XOA1_BITPOS      6
#define I2C_XOA1_MASK        (1 << 6)
#define I2C_XOA2_BITPOS      5
#define I2C_XOA2_MASK        (1 << 5)
#define I2C_XOA3_BITPOS      4
#define I2C_XOA3_MASK        (1 << 4)
#define I2C_STP_BITPOS       1
#define I2C_STP_MASK         (1 << 1)
#define I2C_STT_BITPOS       0
#define I2C_STT_MASK         (1 << 0)



/******************************************************************************
 * I2C_OA0 REGISTER
 ******************************************************************************/
#define I2C_OA0    (0x28 >> 1)

#define I2C_MCODE_BITPOS    13
#define I2C_MCODE_MASK      (7 << 13)
#define I2C_OA_BITPOS       0
#define I2C_OA_MASK         (0x3FF << 0)



/******************************************************************************
 * I2C_SA REGISTER
 ******************************************************************************/
#define I2C_SA    (0x2C >> 1)

#define I2C_SA_BITPOS    0
#define I2C_SA_MASK      (0x3FF << 0)



/******************************************************************************
 * I2C_PSC REGISTER
 ******************************************************************************/
#define I2C_PSC    (0x30 >> 1)

#define I2C_PSC_BITPOS    0
#define I2C_PSC_MASK      (0xFF << 0)



/******************************************************************************
 * I2C_SCLL REGISTER
 ******************************************************************************/
#define I2C_SCLL    (0x34 >> 1)

#define I2C_HSSCLL_BITPOS    8
#define I2C_HSSCLL_MASK      (0xFF << 8)
#define I2C_SCLL_BITPOS      0
#define I2C_SCLL_MASK        (0xFF << 0)



/******************************************************************************
 * I2C_SCLH REGISTER
 ******************************************************************************/
#define I2C_SCLH    (0x38 >> 1)

#define I2C_HSSCLH_BITPOS    8
#define I2C_HSSCLH_MASK      (0xFF << 8)
#define I2C_SCLH_BITPOS      0
#define I2C_SCLH_MASK        (0xFF << 0)



/******************************************************************************
 * I2C_SYSTEST REGISTER
 ******************************************************************************/
#define I2C_SYSTEST    (0x3C >> 1)

#define I2C_ST_EN_BITPOS      15
#define I2C_ST_EN_MASK        (1 << 15)
#define I2C_FREE_BITPOS       14
#define I2C_FREE_MASK         (1 << 14)
#define I2C_TMODE_BITPOS      12
#define I2C_TMODE_MASK        (3 << 12)
#define I2C_SSB_BITPOS        11
#define I2C_SSB_MASK          (1 << 11)
#define I2C_SCCBE_O_BITPOS    4
#define I2C_SCCBE_O_MASK      (1 << 4)
#define I2C_SCL_I_BITPOS      3
#define I2C_SCL_I_MASK        (1 << 3)
#define I2C_SCL_O_BITPOS      2
#define I2C_SCL_O_MASK        (1 << 2)
#define I2C_SDA_I_BITPOS      1
#define I2C_SDA_I_MASK        (1 << 1)
#define I2C_SDA_O_BITPOS      0
#define I2C_SDA_O_MASK        (1 << 0)

#endif /* _OMAP_I2C_REGS_H */
