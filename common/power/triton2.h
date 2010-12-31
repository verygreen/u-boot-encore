/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  triton2.h
*
*  TRITON2 power chip constants, registers and bitmasks
*
*  @author  Pavel Nedev
*
*  @date  20/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _TRITON2_H
#define _TRITON2_H

/******************************************************************************
 * I2C SLAVE ADDRESSES
 ******************************************************************************/
#define T2_ID4    0x4B



/******************************************************************************
 * REGISTERS AND BITMASKS
 ******************************************************************************/

/******************************************************************************
 * T2_ID4_VAUX2_DEV_GRP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX2_DEV_GRP    0x76

#define T2_VAUX2_DEV_GRP_BITPOS     5
#define T2_VAUX2_DEV_GRP_MASK       (7 << 5)
#define T2_VAUX2_WARM_CFG_BITPOS    4
#define T2_VAUX2_WARM_CFG_MASK      (1 << 4)
#define T2_VAUX2_STATE_BITPOS       0
#define T2_VAUX2_STATE_MASK         (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX2_REMAP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX2_REMAP    0x78

#define T2_VAUX2_OFF_STATE_BITPOS      4
#define T2_VAUX2_OFF_STATE_MASK        (0xF << 4)
#define T2_VAUX2_SLEEP_STATE_BITPOS    0
#define T2_VAUX2_SLEEP_STATE_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX2_DEDICATED REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX2_DEDICATED    0x79

#define T2_VAUX2_VSEL_BITPOS    0
#define T2_VAUX2_VSEL_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX3_DEV_GRP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX3_DEV_GRP    0x7A

#define T2_VAUX3_DEV_GRP_BITPOS     5
#define T2_VAUX3_DEV_GRP_MASK       (7 << 5)
#define T2_VAUX3_WARM_CFG_BITPOS    4
#define T2_VAUX3_WARM_CFG_MASK      (1 << 4)
#define T2_VAUX3_STATE_BITPOS       0
#define T2_VAUX3_STATE_MASK         (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX3_REMAP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX3_REMAP    0x7C

#define T2_VAUX3_OFF_STATE_BITPOS      4
#define T2_VAUX3_OFF_STATE_MASK        (0xF << 4)
#define T2_VAUX3_SLEEP_STATE_BITPOS    0
#define T2_VAUX3_SLEEP_STATE_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX3_DEDICATED REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX3_DEDICATED    0x7D

#define T2_VAUX3_VSEL_BITPOS    0
#define T2_VAUX3_VSEL_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX4_DEV_GRP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX4_DEV_GRP    0x7E

#define T2_VAUX4_DEV_GRP_BITPOS     5
#define T2_VAUX4_DEV_GRP_MASK       (7 << 5)
#define T2_VAUX4_WARM_CFG_BITPOS    4
#define T2_VAUX4_WARM_CFG_MASK      (1 << 4)
#define T2_VAUX4_STATE_BITPOS       0
#define T2_VAUX4_STATE_MASK         (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX4_REMAP REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX4_REMAP    0x80

#define T2_VAUX4_OFF_STATE_BITPOS      4
#define T2_VAUX4_OFF_STATE_MASK        (0xF << 4)
#define T2_VAUX4_SLEEP_STATE_BITPOS    0
#define T2_VAUX4_SLEEP_STATE_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VAUX4_DEDICATED REGISTER
 ******************************************************************************/
#define T2_ID4_VAUX4_DEDICATED    0x81

#define T2_VAUX4_VSEL_BITPOS    0
#define T2_VAUX4_VSEL_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VPLL2_DEV_GRP REGISTER
 ******************************************************************************/
#define T2_ID4_VPLL2_DEV_GRP    0x8E

#define T2_VPLL2_DEV_GRP_BITPOS     5
#define T2_VPLL2_DEV_GRP_MASK       (7 << 5)
#define T2_VPLL2_WARM_CFG_BITPOS    4
#define T2_VPLL2_WARM_CFG_MASK      (1 << 4)
#define T2_VPLL2_STATE_BITPOS       0
#define T2_VPLL2_STATE_MASK         (0xF << 0)



/******************************************************************************
 * T2_ID4_VPLL2_REMAP REGISTER
 ******************************************************************************/
#define T2_ID4_VPLL2_REMAP    0x90

#define T2_VPLL2_OFF_STATE_BITPOS      4
#define T2_VPLL2_OFF_STATE_MASK        (0xF << 4)
#define T2_VPLL2_SLEEP_STATE_BITPOS    0
#define T2_VPLL2_SLEEP_STATE_MASK      (0xF << 0)



/******************************************************************************
 * T2_ID4_VPLL2_DEDICATED REGISTER
 ******************************************************************************/
#define T2_ID4_VPLL2_DEDICATED    0x91

#define T2_VPLL2_VSEL_BITPOS    0
#define T2_VPLL2_VSEL_MASK      (0xF << 0)



/******************************************************************************
 * TYPE DEFINITIONS
 ******************************************************************************/
/* ========================================================================== */
/**
*  t2_vaux2_dev_grp_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX2_NO_GRP   = 0,
    T2_VAUX2_P1       = 1,
    T2_VAUX2_P2       = 2,
    T2_VAUX2_P1_P2    = 3,
    T2_VAUX2_P3       = 4,
    T2_VAUX2_P1_P3    = 5,
    T2_VAUX2_P2_P3    = 6,
    T2_VAUX2_P1_P2_P3 = 7
} t2_vaux2_dev_grp_t;



/* ========================================================================== */
/**
*  t2_vaux2_state_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX2_OFF    = 0,
    T2_VAUX2_SLEEP  = 8,
    T2_VAUX2_ACTIVE = 14
} t2_vaux2_state_t;



/* ========================================================================== */
/**
*  t2_vaux2_vsel_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX2_1V3 = 3,
    T2_VAUX2_1V5 = 4,
    T2_VAUX2_1V8 = 5,
    T2_VAUX2_2V5 = 7,
    T2_VAUX2_2V8 = 9
} t2_vaux2_vsel_t;



/* ========================================================================== */
/**
*  t2_vaux3_dev_grp_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX3_NO_GRP   = 0,
    T2_VAUX3_P1       = 1,
    T2_VAUX3_P2       = 2,
    T2_VAUX3_P1_P2    = 3,
    T2_VAUX3_P3       = 4,
    T2_VAUX3_P1_P3    = 5,
    T2_VAUX3_P2_P3    = 6,
    T2_VAUX3_P1_P2_P3 = 7
} t2_vaux3_dev_grp_t;



/* ========================================================================== */
/**
*  t2_vaux3_state_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX3_OFF    = 0,
    T2_VAUX3_SLEEP  = 8,
    T2_VAUX3_ACTIVE = 14
} t2_vaux3_state_t;



/* ========================================================================== */
/**
*  t2_vaux3_vsel_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX3_1V5 = 0,
    T2_VAUX3_1V8 = 1,
    T2_VAUX3_2V5 = 2,
    T2_VAUX3_2V8 = 3
} t2_vaux3_vsel_t;



/* ========================================================================== */
/**
*  t2_vaux4_dev_grp_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX4_NO_GRP   = 0,
    T2_VAUX4_P1       = 1,
    T2_VAUX4_P2       = 2,
    T2_VAUX4_P1_P2    = 3,
    T2_VAUX4_P3       = 4,
    T2_VAUX4_P1_P3    = 5,
    T2_VAUX4_P2_P3    = 6,
    T2_VAUX4_P1_P2_P3 = 7
} t2_vaux4_dev_grp_t;



/* ========================================================================== */
/**
*  t2_vaux4_state_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX4_OFF    = 0,
    T2_VAUX4_SLEEP  = 8,
    T2_VAUX4_ACTIVE = 14
} t2_vaux4_state_t;



/* ========================================================================== */
/**
*  t2_vaux4_vsel_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VAUX4_0V7 = 0,
    T2_VAUX4_1V0 = 1,
    T2_VAUX4_1V2 = 2,
    T2_VAUX4_1V5 = 4,
    T2_VAUX4_1V8 = 5,
    T2_VAUX4_2V5 = 7
} t2_vaux4_vsel_t;



/* ========================================================================== */
/**
*  t2_vpll2_dev_grp_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VPLL2_NO_GRP   = 0,
    T2_VPLL2_P1       = 1,
    T2_VPLL2_P2       = 2,
    T2_VPLL2_P1_P2    = 3,
    T2_VPLL2_P3       = 4,
    T2_VPLL2_P1_P3    = 5,
    T2_VPLL2_P2_P3    = 6,
    T2_VPLL2_P1_P2_P3 = 7
} t2_vpll2_dev_grp_t;



/* ========================================================================== */
/**
*  t2_vpll2_state_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VPLL2_OFF    = 0,
    T2_VPLL2_SLEEP  = 8,
    T2_VPLL2_ACTIVE = 14
} t2_vpll2_state_t;



/* ========================================================================== */
/**
*  t2_vpll2_vsel_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    T2_VPLL2_0V7 = 0,
    T2_VPLL2_1V0 = 1,
    T2_VPLL2_1V2 = 2,
    T2_VPLL2_1V3 = 3,
    T2_VPLL2_1V8 = 5,
} t2_vpll2_vsel_t;

#endif /* _TRITON2_H */
