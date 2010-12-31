/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  iic.h
*
*  Definitions, types and API function prototipes for OMAP3 I2C driver
*
*  @author  Pavel Nedev
*
*  @date  20/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _I2C_H
#define _I2C_H

/******************************************************************************
 * CONSTANT DEFINITIONS
 ******************************************************************************/
#define I2C_HW_TIMEOUT    10
#define I2C_TIMEOUT       1000



/******************************************************************************
 * TYPE DEFINITIONS
 ******************************************************************************/
/* ========================================================================== */
/**
*  iic_dev_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    I2C_1 = 1,
    I2C_2 = 2,
    I2C_3 = 3
} iic_dev_t;



/* ========================================================================== */
/**
*  iic_speed_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
typedef enum {
    I2C_100_KHZ = 0,
    I2C_400_KHZ = 1
} iic_speed_t;



/* ========================================================================== */
/**
*  iic_handle_t    struct_description
*
*  @see
*/
/* ========================================================================== */
typedef struct {
    /** element_description */
    vu16  *regs;
    /** element_description */
    u32   iic_clk_en_mask;
} iic_handle_t;



/******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
/* ========================================================================== */
/**
*  iic_init()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int iic_init(iic_handle_t **iic_h_ptr, iic_dev_t iic_dev,
             iic_speed_t iic_speed);



/* ========================================================================== */
/**
*  iic_deinit()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
void iic_deinit(iic_handle_t **iic_h_ptr);



/* ========================================================================== */
/**
*  iic_tx()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int iic_tx(iic_handle_t *iic_h, u32 iic_slave_addr, u8 *tx_data,
           u16 bytes_to_transmit);



/* ========================================================================== */
/**
*  iic_rx()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int iic_rx(iic_handle_t *iic_h, u32 iic_slave_addr, u8 *rx_data,
           u16 bytes_to_receive);



/* ========================================================================== */
/**
*  iic_wr_r8d8()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int iic_wr_r8d8(iic_handle_t *iic_h, u8 iic_slave_addr, u8 slave_reg_addr,
                u8 tx_data);



/* ========================================================================== */
/**
*  iic_rd_r8d8()    function_description
*
*  @param   param_name    param_description
*
*  @return  return_value    return_value_description
*
*  @pre     pre_conditions
*
*  @post    post_conditions
*
*  @see     related_functions
*/
/* ========================================================================== */
int iic_rd_r8d8(iic_handle_t *iic_h, u8 iic_slave_addr, u8 slave_reg_addr,
                u8 *rx_data);

#endif /* _I2C_H */
