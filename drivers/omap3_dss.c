/* (C) Copyright 2010
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/dss.h>

/*
 * Configure Panel Specific parameters
 */
void omap3_dss_panel_config(const struct panel_config *panel_cfg)
{
   dss_write_reg(DISPC_TIMING_H, panel_cfg->timing_h);
   dss_write_reg(DISPC_TIMING_V, panel_cfg->timing_v);
   dss_write_reg(DISPC_POL_FREQ, panel_cfg->pol_freq);
   dss_write_reg(DISPC_DIVISOR, panel_cfg->divisor);
   dss_write_reg(DISPC_SIZE_LCD, panel_cfg->lcd_size);
   dss_write_reg(DISPC_CONFIG, (panel_cfg->load_mode << FRAME_MODE_OFFSET) | (1 << 9));
   dss_write_reg(DISPC_CONTROL, ((panel_cfg->panel_type << TFTSTN_OFFSET) |
                                (panel_cfg->data_lines << DATALINES_OFFSET)));
}

void omap3_dss_mem_config(const struct panel_config *panel_cfg, void *mem)
{
    dss_write_reg(DISPC_GFX_ATTRIBUTES, 0x1 | (0x6 << 1) | (2 << 6)); // enable and use RGB565, 15x32bit burst size
    dss_write_reg(DISPC_GFX_BA0, (u32) mem);
    dss_write_reg(DISPC_GFX_SIZE, (599 << 16) | 1023);
}

/*
 * Enable LCD and DIGITAL OUT in DSS
 */
void omap3_dss_enable(void)
{
    u32 l = 0;

    l = dss_read_reg(DISPC_CONTROL);
    l |= DISPC_ENABLE;

    dss_write_reg(DISPC_CONTROL, l);
}

void omap3_dss_disable(void)
{
    u32 l = 0;

    l = dss_read_reg(DISPC_CONTROL);
    l &= ~DISPC_ENABLE;

    dss_write_reg(DISPC_CONTROL, l);
}

#define MAX_WAIT 10 * 1000 * 1000

void omap3_dss_reset(void)
{
    int i;
    u32 l = 0;

    l = dss_read_reg(DSS_SYSCONFIG);
    l |= (1 << 1);
    
    dss_write_reg(DSS_SYSCONFIG, l);
    for (i = 0; (dss_read_reg(DSS_SYSSTATUS) == 0x0) && i < MAX_WAIT; ++i);
}

/*
 * Set Background Color in DISPC
 */
void omap3_dss_set_background_col(u32 color)
{
    dss_write_reg(DISPC_DEFAULT_COLOR0, color);
}

