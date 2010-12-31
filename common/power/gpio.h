/* ============================================================================
*                       MultiMedia Solutions
*   (c) Copyright 2009, MultiMedia Solutions  All Rights Reserved.
*
*   Use of this software is controlled by the terms and conditions found
*   in the license agreement under which this software has been supplied.
* =========================================================================== */
/**
*  @file  gpio.h
*
*  Definitions, types and API function prototypes for OMAP3 GPIO driver
*
*  @author  Pavel Nedev
*
*  @date  20/11/2009
*
*  @version  1.00
*/
/* ========================================================================== */

#ifndef _GPIO_H
#define _GPIO_H

/******************************************************************************
 * CONSTANT DEFINITIONS
 ******************************************************************************/
#define GPIOs_COUNT        192
#define GPIO_HW_TIMEOUT    1000



/******************************************************************************
 * TYPE DEFINITIONS
 ******************************************************************************/
/* ========================================================================== */
/**
*  gpio_dir_t    enum_description
*
*  @param  element_name
*/
/* ========================================================================== */
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



/******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/
/* ========================================================================== */
/**
*  gpio_pin_init()    function_description
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
int gpio_pin_init(u32 gpio_pin, gpio_dir_t gpio_dir, gpio_level_t gpio_level);



/* ========================================================================== */
/**
*  gpio_pin_write()    function_description
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
int gpio_pin_write(u32 gpio_pin, gpio_level_t gpio_level);

int gpio_pin_read(u32 gpio_pin);



/* ========================================================================== */
/**
*  gpio_pin_toggle()    function_description
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
int gpio_pin_toggle(u32 gpio_pin);



/* ========================================================================== */
/**
*  gpio_pin_pulse()    function_description
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
int gpio_pin_pulse(u32 gpio_pin, u32 pulse_loop_cycles);

#endif /* _GPIO_H */
