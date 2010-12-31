/*
 * (C) Copyright 2010
 * Texas Instruments, <www.ti.com>
 * Syed Mohammed Khasim <khasim at ti.com>
 *
 * Referred to Linux DSS driver files for OMAP3
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation's version 2 of
 * the License.
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

#ifndef DSS_H
#define DSS_H

#include <asm/io.h>

/* DSS register addresses */
#define DSS_SYSCONFIG                           0x48050010
#define DSS_SYSSTATUS                           0x48050014
#define DSS_CONTROL                             0x48050040

/* DISPC register addresses */
#define DISPC_SYSCONFIG                         0x48050410
#define DISPC_SYSSTATUS                         0x48050414
#define DISPC_IRQSTATUS                         0x48050418
#define DISPC_CONTROL                           0x48050440
#define DISPC_CONFIG                            0x48050444
#define DISPC_DEFAULT_COLOR0                    0x4805044c
#define DISPC_DEFAULT_COLOR1                    0x48050450
#define DISPC_TRANS_COLOR0                      0x48050454
#define DISPC_TRANS_COLOR1                      0x48050458
#define DISPC_TIMING_H                          0x48050464
#define DISPC_TIMING_V                          0x48050468
#define DISPC_POL_FREQ                          0x4805046c
#define DISPC_DIVISOR                           0x48050470
#define DISPC_SIZE_DIG                          0x48050478
#define DISPC_SIZE_LCD                          0x4805047c

/* GFX register addresses */
#define DISPC_GFX_BA0                           0x48050480
#define DISPC_GFX_POSITION                      0x48050488
#define DISPC_GFX_SIZE                          0x4805048c
#define DISPC_GFX_ATTRIBUTES                    0x480504a0
#define DISPC_GFX_ROW_INC                       0x480504ac
#define DISPC_GFX_PIXEL_INC                     0x480504b0

/* Few Register Offsets */
#define FRAME_MODE_OFFSET          1
#define TFTSTN_OFFSET              3
#define DATALINES_OFFSET           8

/* Enabling Display controller */
#define LCD_ENABLE          1
#define GO_LCD              (1 << 5)
#define GP_OUT0             (1 << 15)
#define GP_OUT1             (1 << 16)

#define DISPC_ENABLE (LCD_ENABLE | \
                     GO_LCD | \
                     GP_OUT0 | \
                     GP_OUT1 )


struct panel_config {
    u32 timing_h;
    u32 timing_v;
    u32 pol_freq;
    u32 divisor;
    u32 lcd_size;
    u32 panel_type;
    u32 data_lines;
    u32 load_mode;
};

static inline void dss_write_reg(int reg, u32 val)
{
     __raw_writel(val, reg);
}

static inline u32 dss_read_reg(int reg)
{
    u32 l = __raw_readl(reg);
    return l;
}

void omap3_dss_panel_config(const struct panel_config *panel_cfg);
void omap3_dss_mem_config(const struct panel_config *panel_cfg, void *mem);
void omap3_dss_enable(void);
void omap3_dss_disable(void);
void omap3_dss_reset(void);
void omap3_dss_set_background_col(u32 color);

#endif /* DSS_H */



