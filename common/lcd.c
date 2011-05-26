/*
 * Common LCD routines for supported CPUs
 *
 * (C) Copyright 2001-2002
 * Wolfgang Denk, DENX Software Engineering -- wd@denx.de
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
 * GNU General Public License for more detail *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/************************************************************************/
/* ** HEADER FILES							*/
/************************************************************************/

//#define DEBUG 

#include <config.h>
#include <common.h>
#include <command.h>
#include <version.h>
#include <stdarg.h>
#include <linux/types.h>
#include <devices.h>
#if defined(CONFIG_POST)
#include <post.h>
#endif
#include <lcd.h>
#include <watchdog.h>

#if defined(CONFIG_PXA250) || defined CONFIG_PXA27X || defined CONFIG_CPU_MONAHANS
#include <asm/byteorder.h>
#endif

#if defined(CONFIG_MPC823)
#include <lcdvideo.h>
#endif

//#include <bmp_layout.h>

//#ifdef CONFIG_LCD

#if defined(CONFIG_ATMEL_LCD)
#include <atmel_lcdc.h>
#endif

/************************************************************************/
/* ** FONT DATA								*/
/************************************************************************/
#include <video_font.h>		/* Get font data, width and height	*/

/************************************************************************/
/* ** LOGO DATA								*/
/************************************************************************/
#ifdef CONFIG_LCD_LOGO
# include <bmp_logo_b.h>		/* Get logo data, width and height	*/
# include <bmp_logo_c.h>
# include <bmp_logo_l.h>
# if LCD_BPP != LCD_COLOR16
# if (CONSOLE_COLOR_WHITE >= BMP_LOGO_OFFSET_B) && (LCD_BPP != LCD_COLOR16)
#  error Default Color Map overlaps with Logo Color Map
# endif
# endif
#endif

DECLARE_GLOBAL_DATA_PTR;

ulong lcd_setmem (ulong addr);

static void lcd_drawchars (ushort x, ushort y, uchar *str, int count);
static inline void lcd_puts_xy (ushort x, ushort y, uchar *s);
static inline void lcd_putc_xy (ushort x, ushort y, uchar  c);

static int lcd_init (void *lcdbase);

int lcd_clear (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
static void *lcd_logo (void);

extern void lcd_ctrl_init (void *lcdbase);
extern void lcd_enable (void);

#if LCD_BPP == LCD_COLOR8
extern void lcd_setcolreg (ushort regno,
				ushort red, ushort green, ushort blue);
#endif
#if LCD_BPP == LCD_MONOCHROME
extern void lcd_initcolregs (void);
#endif

#if LCD_BPP == LCD_COLOR16
static uchar  pixel_size = 0;
static uint   pixel_line_length = 0;
#endif

static int lcd_getbgcolor (void);
static void lcd_setfgcolor (int color);
static void lcd_setbgcolor (int color);

char lcd_is_enabled = 0;

//extern vidinfo_t panel_info;
//extern ulong calc_fbsize(void);
 static ulong fb_size;
//static ushort* local_fb=0;
//static ushort  h,w;

#ifdef	NOT_USED_SO_FAR
static void lcd_getcolreg (ushort regno,
				ushort *red, ushort *green, ushort *blue);
static int lcd_getfgcolor (void);
#endif	/* NOT_USED_SO_FAR */

static uchar c_orient = O_PORTRAIT;
static uchar c_max_rows; // = CONSOLE_ROWS;
static uchar c_max_cols; // = CONSOLE_COLS;


/************************************************************************/

/*----------------------------------------------------------------------*/

static void console_scrollup (void)
{
	/* Copy up rows ignoring the first one */
	memcpy (CONSOLE_ROW_FIRST, CONSOLE_ROW_SECOND, CONSOLE_SCROLL_SIZE);

	/* Clear the last one */
	memset (CONSOLE_ROW_LAST, COLOR_MASK(lcd_color_bg), CONSOLE_ROW_SIZE);
}

/*----------------------------------------------------------------------*/

static inline void console_back (void)
{
	if (--console_col < 0) {
		console_col = c_max_cols-1 ;
		if (--console_row < 0) {
			console_row = 0;
		}
	}

	lcd_putc_xy (console_col * VIDEO_FONT_WIDTH,
		     console_row * VIDEO_FONT_HEIGHT,
		     ' ');
}

/*----------------------------------------------------------------------*/

static inline void console_newline (void)
{
	++console_row;
	console_col = 0;

	/* Check if we need to scroll the terminal */
	if (console_row >= c_max_rows) {
		/* Scroll everything up */
	//	console_scrollup () ;
	//	--console_row; 
	console_row = 0;
	}
}

/*----------------------------------------------------------------------*/

static inline void lcd_console_setpixel(ushort x, ushort y, ushort c)
{
    ushort rx = (c_orient == O_PORTRAIT)? (y) : (x);
    ushort ry = (c_orient == O_PORTRAIT)? (panel_info.vl_row-x) : (y);
    ushort *dest = ((ushort *)lcd_base) + rx + (ry*pixel_line_length);
    *dest = c;   
}


static void lcd_drawchar(ushort x, ushort y, uchar c)
{
//    LOG_CONSOLE("lcd_drawchar: %c [%d, %d]\n", c, x, y);
    ushort row, col, rx, ry, sy, sx;
    for(row=0; row<VIDEO_FONT_HEIGHT; row++)
    {
        sy = y + (row);
        for(ry = sy; ry < (sy+1); ry++)
        {
            uchar bits = video_fontdata[c*VIDEO_FONT_HEIGHT+row];
            for(col=0; col<VIDEO_FONT_WIDTH; col++)
            {
                sx = x + (col);
                for(rx = sx; rx < (sx+1); rx++)
                {
                    lcd_console_setpixel(rx, ry,
                        (bits & 0x80)? lcd_color_fg:lcd_color_bg);
                }
                bits <<= 1;
            }
        }
    }
}


void lcd_putc (const char c)
{
	if (!lcd_is_enabled) {
		serial_putc(c);
		return;
	}

	switch (c) {
	case '\r':	console_col = 0;
			return;

	case '\n':	console_newline();
			return;

	case '\t':	/* Tab (8 chars alignment) */
			console_col +=  8;
			console_col &= ~7;

			if (console_col >= c_max_cols) {
				console_newline();
			}
			return;

	case '\b':	console_back();
			return;

	default:	lcd_drawchar(console_col*VIDEO_FONT_WIDTH,
//lcd_putc_xy (console_col * VIDEO_FONT_WIDTH,
				     console_row * VIDEO_FONT_HEIGHT,
				     c);
			if (++console_col >= c_max_cols) {
				console_newline();
			}
			return;
	}
	/* NOTREACHED */
}

/*----------------------------------------------------------------------*/

void lcd_puts (const char *s)
{
	if (!lcd_is_enabled) {
		serial_puts (s);
		return;
	}

	while (*s) {
		lcd_putc (*s++);
	}
}


void lcd_printf(const char *fmt, ...)
{
        va_list args;
        char buf[CONFIG_SYS_PBSIZE];

        va_start(args, fmt);
        vsprintf(buf, fmt, args);
        va_end(args);

        lcd_puts(buf);

}

/*----------------------------------------------------------------------*/

void lcd_console_setpos(short row, short col)
{
  console_row = (row>0)? ((row > c_max_rows)? c_max_rows:row):0;
  console_col = (col>0)? ((col > c_max_cols)? c_max_cols:col):0;
}

/*----------------------------------------------------------------------*/

void lcd_console_setcolor(int fg, int bg)
{
  lcd_color_fg = fg;
  lcd_color_bg = bg;
}

/************************************************************************/
/* ** Low-Level Graphics Routines					*/
/************************************************************************/


static void lcd_drawchars (ushort x, ushort y, uchar *str, int count)
{
	ushort *dest;
	ushort off, row;

	dest = (ushort *)(lcd_base + y * lcd_line_length + x * (1 << LCD_BPP) / 8);
	off  = x * (1 << LCD_BPP) % 8;

	for (row=0;  row < VIDEO_FONT_HEIGHT;  ++row, dest += lcd_line_length)  {
		uchar *s = str;
		int i;

#if LCD_BPP == LCD_COLOR16
		ushort *d = (ushort *)dest;
#else
		uchar *d = dest;
#endif

#if LCD_BPP == LCD_MONOCHROME
		uchar rest = *d & -(1 << (8-off));
		uchar sym;
#endif
		for (i=0; i<count; ++i) {
			uchar c, bits;

			c = *s++;
			bits = video_fontdata[c * VIDEO_FONT_HEIGHT + row];

#if LCD_BPP == LCD_MONOCHROME
			sym  = (COLOR_MASK(lcd_color_fg) & bits) |
			       (COLOR_MASK(lcd_color_bg) & ~bits);

			*d++ = rest | (sym >> off);
			rest = sym << (8-off);
#elif LCD_BPP == LCD_COLOR8
			for (c=0; c<8; ++c) {
				*d++ = (bits & 0x80) ?
						lcd_color_fg : lcd_color_bg;
				bits <<= 1;
			}
#elif LCD_BPP == LCD_COLOR16
			for (c=0; c<8; ++c) {
				*d++ = (bits & 0x80) ?
						lcd_color_fg : lcd_color_bg;
				bits <<= 1;
			}
#endif
		}
#if LCD_BPP == LCD_MONOCHROME
		*d  = rest | (*d & ((1 << (8-off)) - 1));
#endif
	}
}

/*----------------------------------------------------------------------*/

static inline void lcd_puts_xy (ushort x, ushort y, uchar *s)
{
#if defined(CONFIG_LCD_LOGO) && !defined(CONFIG_LCD_INFO_BELOW_LOGO) \
  && !defined(CONFIG_3621EVT1A)
	lcd_drawchars (x, y+BMP_LOGO_HEIGHT_B, s, strlen ((char *)s));
#else
	lcd_drawchars (x, y, s, strlen ((char *)s));
#endif
}

/*----------------------------------------------------------------------*/

static inline void lcd_putc_xy (ushort x, ushort y, uchar c)
{ /*  this isn't used
#if defined(CONFIG_LCD_LOGO) && !defined(CONFIG_LCD_INFO_BELOW_LOGO) \
  && !defined(CONFIG_3621EVT1A)
	lcd_drawchar (x, y+BMP_LOGO_HEIGHT_B, &c);
#else
	lcd_drawchar (x, y, &c);
#endif */
}

/************************************************************************/
/**  Small utility to check that you got the colours right		*/
/************************************************************************/
//#define LCD_TEST_PATTERN
#ifdef LCD_TEST_PATTERN

#define	N_BLK_VERT	2
#define	N_BLK_HOR	3

static int test_colors[N_BLK_HOR*N_BLK_VERT] = {
	CONSOLE_COLOR_RED,	CONSOLE_COLOR_GREEN,	CONSOLE_COLOR_YELLOW,
	CONSOLE_COLOR_BLUE,	CONSOLE_COLOR_MAGENTA,	CONSOLE_COLOR_CYAN,
};

static void test_pattern (void)
{
	ushort v_max  = panel_info.vl_row;
	ushort h_max  = panel_info.vl_col;
	ushort v_step = (v_max + N_BLK_VERT - 1) / N_BLK_VERT;
	ushort h_step = (h_max + N_BLK_HOR  - 1) / N_BLK_HOR;
	ushort v, h;
	ushort *pix = (uchar *)lcd_base;

	printf ("[LCD] Test Pattern: %d x %d [%d x %d]\n",
		h_max, v_max, h_step, v_step);

	/* WARNING: Code silently assumes 8bit/pixel */
	for (v=0; v<v_max; ++v) {
		uchar iy = v / v_step;
		for (h=0; h<h_max; ++h) {
			uchar ix = N_BLK_HOR * iy + (h/h_step);
			*pix++ = test_colors[ix];
		}
	}
}
#endif /* LCD_TEST_PATTERN */


/************************************************************************/
/* ** GENERIC Initialization Routines					*/
/************************************************************************/

int drv_lcd_init (void)
{
	device_t lcddev;
	int rc;

	lcd_base = (void *)(gd->fb_base);

	lcd_line_length = (panel_info.vl_col * NBITS (panel_info.vl_bpix)) / 8;
  
	lcd_init (lcd_base);		/* LCD initialization */

	/* Device initialization */
	memset (&lcddev, 0, sizeof (lcddev));

	strcpy (lcddev.name, "lcd");
	lcddev.ext   = 0;			/* No extensions */
// #ifdef CONFIG_3621EVT1A
//    lcddev.flags = 0; /* Use only for splash */
//#else
	lcddev.flags = DEV_FLAGS_OUTPUT;	/* Output only */
// #endif
	lcddev.putc  = lcd_putc;		/* 'putc' function */
	lcddev.puts  = lcd_puts;		/* 'puts' function */

	rc = device_register (&lcddev);

	return (rc == 0) ? 1 : rc;
}

/*----------------------------------------------------------------------*/
int lcd_clear (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
 
#if LCD_BPP == LCD_MONOCHROME
	/* Setting the palette */
	lcd_initcolregs();

#elif LCD_BPP == LCD_COLOR8
	/* Setting the palette */
	lcd_setcolreg  (CONSOLE_COLOR_BLACK,       0,    0,    0);
	lcd_setcolreg  (CONSOLE_COLOR_RED,	0xFF,    0,    0);
	lcd_setcolreg  (CONSOLE_COLOR_GREEN,       0, 0xFF,    0);
	lcd_setcolreg  (CONSOLE_COLOR_YELLOW,	0xFF, 0xFF,    0);
	lcd_setcolreg  (CONSOLE_COLOR_BLUE,        0,    0, 0xFF);
	lcd_setcolreg  (CONSOLE_COLOR_MAGENTA,	0xFF,    0, 0xFF);
	lcd_setcolreg  (CONSOLE_COLOR_CYAN,	   0, 0xFF, 0xFF);
	lcd_setcolreg  (CONSOLE_COLOR_GREY,	0xAA, 0xAA, 0xAA);
	lcd_setcolreg  (CONSOLE_COLOR_WHITE,	0xFF, 0xFF, 0xFF);
#endif

#ifdef CFG_WHITE_ON_BLACK
	lcd_setfgcolor (CONSOLE_COLOR_WHITE);
	lcd_setbgcolor (CONSOLE_COLOR_BLACK);
#elif defined(CFG_CYAN_ON_BLACK)
	lcd_setfgcolor (CONSOLE_COLOR_CYAN);
	lcd_setbgcolor (CONSOLE_COLOR_BLACK);
#else  /*black on white*/
        lcd_setfgcolor (CONSOLE_COLOR_BLACK);
        lcd_setbgcolor (CONSOLE_COLOR_WHITE);
#endif	/* CFG_WHITE_ON_BLACK */

#ifdef	LCD_TEST_PATTERN
	test_pattern();
#else
	/* set framebuffer to background color */
	memset ((char *)lcd_base,
		COLOR_MASK(lcd_getbgcolor()),
		fb_size);
//		0,
//		fb_size);

#endif
	/*taking out the logo from DENX SW engineering*/
	/* Paint the logo and retrieve LCD base address */
	// debug ("[LCD] Drawing the logo...\n");
	lcd_console_address = lcd_base; //lcd_logo()
	console_col = 0;
	console_row = 0;
	lcd_console_setpos(0, 0);

	return (0);
}

U_BOOT_CMD(
	cls,	1,	1,	lcd_clear,
	"cls     - clear screen\n",
	""
);

/*----------------------------------------------------------------------*/

static int lcd_init (void *lcdbase)
{

	/* Initialize the lcd controller */
	debug ("[LCD] Initializing LCD framebuffer at %p\n", lcdbase);

	lcd_ctrl_init (lcdbase);
	lcd_is_enabled =1;

      /* Initialize the console */
    if(c_orient == O_PORTRAIT)
    {
        c_max_cols = panel_info.vl_row/(VIDEO_FONT_WIDTH);
        c_max_rows = panel_info.vl_col/(VIDEO_FONT_HEIGHT);
    }
    else
    {
        c_max_cols = panel_info.vl_col/(VIDEO_FONT_WIDTH);
        c_max_rows = panel_info.vl_row/(VIDEO_FONT_HEIGHT);
    }

	lcd_clear (NULL, 1, 1, NULL);	/* dummy args */
#ifndef CONFIG_LCD_NOT_ENABLED_AT_INIT
	lcd_enable ();
#endif
	console_col = 0;
	lcd_console_address = lcd_base;
#ifdef CONFIG_LCD_INFO_BELOW_LOGO
	console_row = 7 + BMP_LOGO_HEIGHT_B / VIDEO_FONT_HEIGHT;
#else
	console_row = 1;	/* leave 1 blank line below logo */
#endif

#ifdef CONFIG_LCD_NOT_ENABLED_AT_INIT
	lcd_is_enabled = 0;
#else
	lcd_is_enabled = 1;
#endif

#if LCD_BPP == LCD_COLOR16
  pixel_size = NBITS(LCD_BPP)/8;
  pixel_line_length = lcd_line_length/pixel_size;
#endif
	lcd_console_setpos(0, 0);
	return 0;
}


/************************************************************************/
/* ** ROM capable initialization part - needed to reserve FB memory	*/
/************************************************************************/
/*
 * This is called early in the system initialization to grab memory
 * for the LCD controller.
 * Returns new address for monitor, after reserving LCD buffer memory
 *
 * Note that this is running from ROM, so no write access to global data.
 */
ulong lcd_setmem (ulong addr)
{
	ulong size;
	int line_length = (panel_info.vl_col * NBITS (panel_info.vl_bpix)) / 8;

	debug ("LCD panel info: %d x %d, %d bit/pix\n",
		panel_info.vl_col, panel_info.vl_row, NBITS (panel_info.vl_bpix) );

	size = line_length * panel_info.vl_row;

	/* Round up to nearest full page */
	size = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
  	fb_size = size;
	/* Allocate pages for the frame buffer. */
	addr -= size;

	debug ("Reserving %ldk for LCD Framebuffer at: %08lx\n", size>>10, addr);

	return (addr);
}

/*----------------------------------------------------------------------*/

static void lcd_setfgcolor (int color)
{
	lcd_color_fg = color;
}

/*----------------------------------------------------------------------*/

static void lcd_setbgcolor (int color)
{
	lcd_color_bg = color;
}

/*----------------------------------------------------------------------*/

#ifdef	NOT_USED_SO_FAR
static int lcd_getfgcolor (void)
{
	return lcd_color_fg;
}
#endif	/* NOT_USED_SO_FAR */

/*----------------------------------------------------------------------*/

static int lcd_getbgcolor (void)
{
	return lcd_color_bg;
}

/*----------------------------------------------------------------------*/

/************************************************************************/
/* ** Chipset depending Bitmap / Logo stuff...                          */
/************************************************************************/
#ifdef CONFIG_LCD_LOGO
/*
 which = 0    boot image
 which = 1    low battery image
 which = 2    connect your charger image
*/

void bitmap_plot (int x, int y, uchar which)
{
#ifdef CONFIG_ATMEL_LCD
	uint *cmap;
#else
	ushort *cmap;
#endif
	ushort i, j, bmp_height, bmp_width, colors, offset;
	uchar *bmap;
	ushort *bpalette;
	uchar *fb;
	ushort *fb16;

	switch(which) {
		case 1:
			bmp_height=BMP_LOGO_HEIGHT_L;
			bmp_width=BMP_LOGO_WIDTH_L;
			colors=BMP_LOGO_COLORS_L;
			offset=BMP_LOGO_OFFSET_L;
			bmap = &bmp_logo_bitmap_L[0];
			bpalette=&bmp_logo_palette_L[0];
			break;
		case 2:
			bmp_height= BMP_LOGO_HEIGHT_C;
			bmp_width=BMP_LOGO_WIDTH_C;
			colors=BMP_LOGO_COLORS_C;
			offset=BMP_LOGO_OFFSET_C;
			bmap = &bmp_logo_bitmap_C[0];
			bpalette=&bmp_logo_palette_C[0];
			break;
		case 0:
		default:
			bmp_height= BMP_LOGO_HEIGHT_B;
			bmp_width=BMP_LOGO_WIDTH_B;
			colors=BMP_LOGO_COLORS_B;
			offset=BMP_LOGO_OFFSET_B;
			bmap = &bmp_logo_bitmap_B[0];
			bpalette=&bmp_logo_palette_B[0];
			break;
	}

#if defined(CONFIG_PXA250)
	struct pxafb_info *fbi = &panel_info.pxa;
#elif defined(CONFIG_MPC823)
	volatile immap_t *immr = (immap_t *) CFG_IMMR;
	volatile cpm8xx_t *cp = &(immr->im_cpm);
#endif

	debug ("Logo: width %d  height %d  colors %d  cmap %d\n",
		bmp_width, bmp_height, colors,
		(int)(sizeof(bpalette)/(sizeof(ushort))));

	// bmap = &bmp_logo_bitmap[0];
	fb   = (uchar *)(lcd_base + y * lcd_line_length + x);

	if (NBITS(panel_info.vl_bpix) < 12) {  //encore it is LCD_COLOR16 which == 4
		/* Leave room for default color map */
#if defined(CONFIG_PXA250)
		cmap = (ushort *)fbi->palette;
#elif defined(CONFIG_MPC823)
		cmap = (ushort *)&(cp->lcd_cmap[offset*sizeof(ushort)]);
#elif defined(CONFIG_ATMEL_LCD)
                cmap = (uint *) (panel_info.mmio + ATMEL_LCDC_LUT(0));
#else
                  /*
                  * default case: generic system with no cmap (most likely 16bpp)
                  * We set cmap to the source palette, so no change is done.
                  * This avoids even more ifdef in the next stanza
                  */
		cmap = bpalette;
#endif

		WATCHDOG_RESET();

		/* Set color map */
		for (i=0; i<(sizeof(bpalette)/(sizeof(ushort))); ++i) {
			ushort colreg = bpalette[i];
#ifdef CONFIG_ATMEL_LCD
                        uint lut_entry;
#ifdef CONFIG_ATMEL_LCD_BGR555
                        lut_entry = ((colreg & 0x000F) << 11) |
                                    ((colreg & 0x00F0) <<  2) |
                                    ((colreg & 0x0F00) >>  7);
#else /* CONFIG_ATMEL_LCD_RGB565 */
                        lut_entry = ((colreg & 0x000F) << 1) |
                                    ((colreg & 0x00F0) << 3) |
                                    ((colreg & 0x0F00) << 4);
#endif
                        *(cmap + offset) = lut_entry;
                        cmap++;
#else /* !CONFIG_ATMEL_LCD */
#ifdef  CFG_INVERT_COLORS
			*cmap++ = 0xffff - colreg;
#else
			*cmap++ = colreg;
#endif
#endif /* config_atmel_lcd */
		}

		WATCHDOG_RESET();

		for (i=0; i<bmp_height; ++i) {
			memcpy (fb, bmap, bmp_width);
			bmap += bmp_width;
			fb   += panel_info.vl_col;
		}
	}
	else { /* true color mode */
	   	u16 col16;
		fb16 = (ushort *)(lcd_base + y * lcd_line_length + x);
		for (i=0; i<bmp_height; ++i) {
			for (j=0; j<bmp_width; j++) {
				col16 = bpalette[(bmap[j]-16)];
				fb16[j] =
					((col16 & 0x000F) << 1) |
					((col16 & 0x00F0) << 3) |
					((col16 & 0x0F00) << 4);
				}
			bmap += bmp_width;
			fb16 += panel_info.vl_col;
		}
	}

	WATCHDOG_RESET();
}
#endif /* CONFIG_LCD_LOGO */

/*----------------------------------------------------------------------*/
#if (defined(CONFIG_COMMANDS) || defined(CONFIG_SPLASH_SCREEN))
/*
 * Display the BMP file located at address bmp_image.
 * Only uncompressed.
 */

#ifdef CONFIG_SPLASH_SCREEN_ALIGN
#define BMP_ALIGN_CENTER	0x7FFF
#endif

int lcd_display_bitmap(ulong bmp_image, int x, int y)
{
	ushort *cmap = NULL;
	ushort *cmap_base = NULL;
	ushort i, j;
	uchar *fb;
	bmp_image_t *bmp=(bmp_image_t *)bmp_image;
	uchar *bmap;
	ushort padded_line;
	unsigned long width, height, byte_width;
	unsigned long pwidth = panel_info.vl_col;
	unsigned colors, bpix, bmp_bpix;
	unsigned long compression;
#if defined(CONFIG_PXA250) || defined CONFIG_PXA27X || defined CONFIG_CPU_MONAHANS
	struct pxafb_info *fbi = &panel_info.pxa;
#elif defined(CONFIG_MPC823)
	volatile immap_t *immr = (immap_t *) CFG_IMMR;
	volatile cpm8xx_t *cp = &(immr->im_cpm);
#endif

	if (!((bmp->header.signature[0]=='B') &&
		(bmp->header.signature[1]=='M'))) {
		printf ("Error: no valid bmp image at %lx\n", bmp_image);
		return 1;
	}

	width = le32_to_cpu (bmp->header.width);
	height = le32_to_cpu (bmp->header.height);
	bmp_bpix = le16_to_cpu (bmp->header.bit_count);
	colors = 1 << bmp_bpix;
	compression = le32_to_cpu (bmp->header.compression);

	bpix = NBITS(panel_info.vl_bpix);

	if ((bpix != 1) && (bpix != 8) && (bpix != 16)) {
		printf ("Error: %d bit/pixel mode, but BMP has %d bit/pixel\n",
                        bpix, bmp_bpix);
		return 1;
	}

	if (bpix != bmp_bpix && (bmp_bpix != 8 || bpix != 16)) {
		printf ("Error: %d bit/pixel mode, but BMP has %d bit/pixel\n",
			bpix,
			le16_to_cpu(bmp->header.bit_count));
		return 1;
	}

	debug ("Display-bmp: %d x %d  with %d colors\n",
		(int)width, (int)height, (int)colors);

#if !defined(CONFIG_MCC200)
	if (bmp_bpix==8) {
#if defined(CONFIG_PXA250) || defined CONFIG_PXA27X || defined CONFIG_CPU_MONAHANS
		cmap = (ushort *)fbi->palette;
#elif defined(CONFIG_MPC823)
		cmap = (ushort *)&(cp->lcd_cmap[255*sizeof(ushort)]);
#elif !defined(CONFIG_ATMEL_LCD)
		cmap = panel_info.cmap;
#endif
		cmap_base=cmap;

		/* Set color map */
		for (i=0; i<colors; ++i) {
			bmp_color_table_entry_t cte = bmp->color_table[i];
#if !defined(CONFIG_ATMEL_LCD)
			ushort colreg =
				( ((cte.red)   << 8) & 0xf800) |
				( ((cte.green) << 3) & 0x07e0) |
				( ((cte.blue)  >> 3) & 0x001f) ;
#ifdef CFG_INVERT_COLORS
			*cmap = 0xffff - colreg;
#else
			*cmap = colreg;
#endif
#if defined(CONFIG_MPC823)
			cmap--;
#else
			cmap++;
#endif
#else /* config_atmel_lcd */
			lcd_setcolreg(i, cte.red, cte.green, cte.blue);
#endif
		}
	}
#endif

	padded_line = (width&0x3) ? ((width&~0x3)+4) : (width);


#ifdef CONFIG_SPLASH_SCREEN_ALIGN
	if (x == BMP_ALIGN_CENTER)
		x = max(0, (pwidth - width) / 2);
	else if (x < 0)
		x = max(0, pwidth - width + x + 1);

	if (y == BMP_ALIGN_CENTER)
		y = max(0, (panel_info.vl_row - height) / 2);
	else if (y < 0)
		y = max(0, panel_info.vl_row - height + y + 1);
#endif /* CONFIG_SPLASH_SCREEN_ALIGN */

	if ((x + width)>pwidth)
		width = pwidth - x;
	if ((y + height)>panel_info.vl_row)
		height = panel_info.vl_row - y;

	bmap = (uchar *)bmp + le32_to_cpu (bmp->header.data_offset);
	fb   = (uchar *) (lcd_base +
		(y + height - 1) * lcd_line_length + x * bpix / 8);

	switch(bmp_bpix) {
	case 1: /* pass through */
	case 8:
		if (bpix != 16)
		byte_width = width;
	else
		byte_width=width * 2;

	for (i = 0; i < height; ++i) {
		WATCHDOG_RESET();
		for (j = 0; j < width; j++) {
		if (bpix != 16) {
#if defined CONFIG_PXA250 || defined CONFIG_PXA27X || defined CONFIG_CPU_MONAHANS || defined(CONFIG_ATMEL_LCD)
			*(fb++) = *(bmap++);
#elif defined(CONFIG_MPC823) || defined(CONFIG_MCC200)
			*(fb++) = 255 - *(bmap++);
#endif
	} else {
		*(uint16_t *)fb = cmap_base[*(bmap++)];
		fb += sizeof(uint16_t)/sizeof(*fb);
		}
	}
		bmap += (width - padded_line);
		fb   -= (byte_width + lcd_line_length);
	}
	break;

#if defined(CONFIG_BMP_16BPP)
	case 16:
		for (i = 0; i < height; ++i) {
		WATCHDOG_RESET();
		for (j = 0; j < width; j++) {
#if defined(CONFIG_ATMEL_LCD_BGR555)
                                *(fb++) = ((bmap[0] & 0x1f) << 2) |
                                        (bmap[1] & 0x03);
                                *(fb++) = (bmap[0] & 0xe0) |
                                        ((bmap[1] & 0x7c) >> 2);
                                bmap += 2;
#else
                                *(fb++) = *(bmap++);
                                *(fb++) = *(bmap++);
#endif
                        }
                        bmap += (padded_line - width) * 2;
                        fb   -= (width * 2 + lcd_line_length);
                }
                break;
#endif /* CONFIG_BMP_16BPP */

	default:
		break;
    };

	return (0);
}
#endif

static void *lcd_logo (void)
{

#ifdef CONFIG_SPLASH_SCREEN
	char *s;
	ulong addr;
	static int do_splash = 1;

	if (do_splash && (s = getenv("splashimage")) != NULL) {
		int x = 0; y = 0;
		do_splash = 0;

		addr = simpl_strtoul (s, NULL, 16);
#ifdef CONFIG_SPLASH_SCREEN_ALIGN
		if ((s = gentenv("splashpos")) != NULL {
			if (s[0] == 'm')
				x = BMP_ALIGN_CENTER;
			else
				x = simple_strtol (s, NULL, 0);

			if (( s= strchr (s + 1, ',')) != NULL) {
				if (s[1]=='m')
					y = BMP_ALIGN_CENTER;
				else
					y = simple_strtol (s + 1, NULL, 0);
			}
		}
#endif /* CONFIG_SPLASH_SCREEN_ALIGN */

#ifdef CONFIG_VIDEO_BMP_GZIP
                bmp_image_t *bmp = (bmp_image_t *)addr;
                unsigned long len;

                if (!((bmp->header.signature[0]=='B') &&
                      (bmp->header.signature[1]=='M'))) {
                        addr = (ulong)gunzip_bmp(addr, &len);
                }
#endif

		if (lcd_display_bitmap (addr, x, y) == 0) {
			return ((void *)lcd_base);
		}
	}
#endif /* CONFIG_SPLASH_SCREEN */

#ifdef CONFIG_LCD_LOGO
	bitmap_plot (0, 0, 0);
#endif /* CONFIG_LCD_LOGO */

#ifdef CONFIG_LCD_INFO
        console_col = LCD_INFO_X / VIDEO_FONT_WIDTH;
        console_row = LCD_INFO_Y / VIDEO_FONT_HEIGHT;
        lcd_show_board_info();
#endif /* CONFIG_LCD_INFO */

#if defined(CONFIG_LCD_LOGO) && !defined(CONFIG_LCD_INFO_BELOW_LOGO)
	return ((void *)((ulong)lcd_base + BMP_LOGO_HEIGHT_B * lcd_line_length));
#else
	return ((void *)lcd_base);
#endif /* CONFIG_LCD_LOGO && !CONFIG_LCD_INFO_BELOW_LOGO */
}

/************************************************************************/
/************************************************************************/
