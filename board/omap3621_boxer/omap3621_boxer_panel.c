/*
 * Encore panel support 
 *
 * (C) Copyright 2010 Barnes & Noble
 * Author: David Bolcsfoldi <dbolcsfoldi@intrinsyc.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <linux/types.h>
#include <config.h>
#include <common.h>
#include <lcd.h>
#include <spi.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/mux.h>

typedef enum {
    GPIO_OUTPUT = 0,
    GPIO_INPUT  = 1
} gpio_dir_t;

typedef enum {
    GPIO_LOW  = 0,
    GPIO_HIGH = 1
} gpio_level_t;

extern int gpio_pin_init(u32, gpio_dir_t, gpio_level_t);
extern int gpio_pin_write(u32, gpio_level_t);

vidinfo_t panel_info = {
    .vl_col     = 1024,
    .vl_row     = 600,
    .vl_width   = 163,
    .vl_height  = 103,

    .vl_clkp    = CFG_HIGH,
    .vl_oep     = CFG_HIGH,
    .vl_hsp     = CFG_HIGH,
    .vl_vsp     = CFG_HIGH,
    .vl_dp      = CFG_HIGH,
    .vl_bpix    = LCD_COLOR16,
    .vl_lbw     = 1,
    .vl_splt    = 0,
    .vl_clor    = 1,
    .vl_tft     = 1,

    .dss_panel_config = {
        .timing_h   = 0x0c704527, 
        .timing_v   = 0x00b00a09,
        .pol_freq   = 0x7000,
        .divisor    = 0x00010008,
        .lcd_size   = (599 << 16) | (1023),
        .panel_type = 0x1,
        .data_lines = 0x3,
        .load_mode  = 0x2,
    },
};

int lcd_line_length;
int lcd_color_fg = 0;
int lcd_color_bg = 0;

void *lcd_base;
void *lcd_console_address;

short console_col;
short console_row;

#define FCK_DSS_ON 0x00000007 /* dss1+dss2+tv */
#define ICK_DSS_ON 0x00000001 

#define     MUX_VAL(OFFSET,VALUE) __raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET));
#define     CP(x)   (CONTROL_PADCONF_##x)
     
void lcd_ctrl_init(void *lcdbase)
{
     
   sr32(CM_FCLKEN_DSS, 0, 32, FCK_DSS_ON);
	sr32(CM_ICLKEN_DSS, 0, 32, ICK_DSS_ON);
    
    udelay(200);
  
    omap3_dss_panel_config(&panel_info.dss_panel_config);
    omap3_dss_mem_config(&panel_info.dss_panel_config, lcdbase);
}

void lcd_setcolreg (ushort regno, ushort red, ushort green, ushort blue)
{
}

#if (CONFIG_COMMANDS & CFG_CMD_SPI)
void spi_panel_chipsel(int cs)
{
    if (cs) {
        gpio_pin_init(GPIO_SPI_CS,0,0);
    } else {
        SPI_DELAY;
        gpio_pin_init(GPIO_SPI_CS,0,1);
    }
}

spi_chipsel_type spi_chipsel[] = {
    spi_panel_chipsel,
};

int spi_chipsel_cnt = sizeof(spi_chipsel) / sizeof(spi_chipsel[0]);

#endif /* (CONFIG_COMMANDS & CFG_CMD_SPI) */

void lcd_spi_send(unsigned char reg_addr, unsigned char reg_data)
{
    int ret = 0;
    int msg,imsg;
    uchar omsg[2];
    msg=(reg_addr<<10)|reg_data;

    // note big endian, so swap
    omsg[0]=(msg>>8)&0xff;
    omsg[1]=(msg)&0xff;

    spi_xfer(spi_chipsel[0], 16,( uchar *)omsg, (uchar *) &imsg);

    // wait for transaction to finish
    udelay(400);
}

static inline u32 read_gpt8_reg(u32 reg)
{
    return __raw_readl(OMAP34XX_GPT8 + reg);
}

static inline void write_gpt8_reg(u32 reg, u32 val)
{
    __raw_writel(val, OMAP34XX_GPT8 + reg); 
}

void lcd_adjust_brightness(int level);


// autoreload, overflow & cmp trigger, compare enable
#define GPT8_ST		(1<<0)
#define GPT8_AR		(1<<1)
#define GPT8_CE		(1<<6)
#define GPT8_SCPWM	(1<<7)
#define GPT8_PT 	(1<<12)
#define GPT8_GPOCFG	(1<<14)
    
#define GPT8_PWM_EN (GPT8_AR | (2 << 10) | GPT8_CE | GPT8_ST | GPT8_PT)

 void enable_backlight(void) 
{
    DECLARE_GLOBAL_DATA_PTR;
    u32 l;
   
  
    sr32(CM_CLKSEL_PER, 6, 1, 0x0); /* CLKSEL = 32Khz */
    sr32(CM_FCLKEN_PER, 9, 1, 0x1); /* FCLKEN GPT8 */
    sr32(CM_ICLKEN_PER, 9, 1, 0x1); /* ICLKEN GPT8 */
   
    udelay(200);

    /*start with a random brightness which consumes less than 500mA such that we will gain
      current into battery even when display is showing UI image*/
    lcd_adjust_brightness(40); 
    
   	gpio_pin_init(GPIO_BACKLIGHT_EN_EVT2, GPIO_OUTPUT, 0);
  
}

void disable_backlight(void)
{
    DECLARE_GLOBAL_DATA_PTR;
    u32 l;

    l = read_gpt8_reg(TCLR);
    l &= ~GPT8_PWM_EN;
    write_gpt8_reg(TLDR, 0x0);
    write_gpt8_reg(TTGR, 0x0);
    write_gpt8_reg(TMAR, 0x0);

    sr32(CM_FCLKEN_PER, 9, 1, 0x0); /* FCLKEN GPT8 */
    sr32(CM_ICLKEN_PER, 9, 1, 0x0); /* ICLKEN GPT8 */

    gpio_pin_write( GPIO_BACKLIGHT_EN_EVT2, 1 );

}
void lcd_adjust_brightness(int level)
{
	 u32 value=0;
    u32 v_tldr=0xfffffff0;
    
   /*stop timer first*/
    write_gpt8_reg(TCLR, 0x0);
    
     /*set the match value*/
    value =  v_tldr  + ((0xFFFFFFFE - v_tldr) * level/100);
  
  	 write_gpt8_reg(TMAR, value);  
  	  	 
    /*set the carrier freq to based on TLDR value*/
    write_gpt8_reg(TLDR, v_tldr);
  
  	 /*set the counter to compare */
    write_gpt8_reg(TCRR, v_tldr);

   
    /*start the PWM*/
    value=GPT8_PWM_EN;
    write_gpt8_reg(TCLR, value);
	
}

void boxer_disable_panel(void)
{
	 lcd_spi_send( 0x0, 0x00);
}
void boxer_init_panel(void)
{
   lcd_spi_send( 0x0, 0x00);
	lcd_spi_send(   0, 0xad);
	lcd_spi_send(   1, 0x30);
	lcd_spi_send(   2, 0x40);
	lcd_spi_send( 0xe, 0x5f);
	lcd_spi_send( 0xf, 0xa4);
	lcd_spi_send( 0xd, 0x00);
	lcd_spi_send( 0x2, 0x43);
	lcd_spi_send( 0xa, 0x28);
	lcd_spi_send( 0x10, 0x41);
}


void lcd_enable(void)
{
    gpio_pin_init(36, GPIO_OUTPUT, 1);

    // Wait one ms before sending down SPI init sequence
    udelay(1000);

    MUX_VAL(CP(McBSP1_CLKR),    (OFF_IN_PD  | IEN  | PTD | DIS | M4))  /*McSPI4-CLK*/ \
    MUX_VAL(CP(McBSP1_DX),      (OFF_IN_PD  | IDIS | PTD | DIS | M4))   /*McSPI4-SIMO*/ \
    MUX_VAL(CP(McBSP1_DR),      (OFF_IN_PD  | IEN  | PTD | DIS | M4))  /*McSPI4-SOMI*/\
    MUX_VAL(CP(McBSP1_FSX),     (OFF_IN_PD  | IEN  | PTU | DIS | M4))  /*McSPI4-CS0*/
   
    gpio_pin_init(GPIO_SPI_CLK,GPIO_OUTPUT,1);  
    gpio_pin_init(GPIO_SPI_SIMO,GPIO_OUTPUT,1);   
    gpio_pin_init(GPIO_SPI_CS,GPIO_OUTPUT,1);  

    gpio_pin_init(GPIO_SPI_SOMI,GPIO_INPUT,1);   
  
	boxer_init_panel();
    
    omap3_dss_enable();
    enable_backlight();
}

void lcd_disable(void)
{
    disable_backlight();

    sr32(CM_FCLKEN_DSS, 0, 32, 0x0);
	sr32(CM_ICLKEN_DSS, 0, 32, 0x0); 

    gpio_pin_write(36, 0);

    // Restore SPI registers
    MUX_VAL(CP(McBSP1_CLKR),    (IEN  | PTD | DIS | M1)) /*McSPI4-CLK*/ \
    MUX_VAL(CP(McBSP1_DX),      (IDIS | PTD | DIS | M1)) /*McSPI4-SIMO*/ \
    MUX_VAL(CP(McBSP1_DR),      (IEN  | PTD | DIS | M1)) /*McSPI4-SOMI*/\
    MUX_VAL(CP(McBSP1_FSX),     (IDIS | PTD | DIS | M1)) /*McSPI4-CS0*/ \
}

void lcd_panel_disable(void)
{
}

ulong calc_fbsize(void)
{
    ulong size;
    int line_length = (panel_info.vl_col * NBITS (panel_info.vl_bpix)) / 8;

    size = line_length * panel_info.vl_row;
    size += PAGE_SIZE;

    return size;
}

