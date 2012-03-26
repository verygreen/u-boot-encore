/*
 * Original Boot Menu code by j3mm3r
 * (C) Copyright 2011 j3mm3r
 * 1.2 Enhancements/NC port by fattire
 * (C) Copyright 2011-2012 The CyanogenMod Project
 *
 *
 * See file CREDITS for list of more people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
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

#include "menu.h"

#include <common.h>
#include <lcd.h>
#include <tps65921.h>
#include "power/gpio.h"

#define INDENT		25
#define MENUTOP		42

//	lcd_bl_set_brightness(255);
//	lcd_console_init();

#define NUM_OPTS		7  //number of boot options

char *opt_list[NUM_OPTS] = 	{" Internal eMMC Normal     ",
				 " Internal eMMC Recovery   ",
				 " Internal eMMC Alternate  ",
				 " SD Card Normal           ",
				 " SD Card Recovery         ",
				 " SD Card Alternate        ",
//				 " Start fastboot           ",
				 " default boot from:  ",
				};

char read_u_boot_device(void) { return 'X'; }
int write_u_boot_device(char nothing) { return 0; }

// -----------------------------------

void print_u_boot_dev(void) {
	if (read_u_boot_device() == '1') {
		lcd_puts("EMMC ");
	} else {
		lcd_puts("SD   ");
	}
}


void highlight_boot_line(int cursor, enum highlight_type highl) {
	switch (highl) {
	case HIGHLIGHT_GREEN:
		lcd_console_setcolor( CONSOLE_COLOR_BLACK, CONSOLE_COLOR_GREEN);
		break;
	case HIGHLIGHT_CYAN:
		lcd_console_setcolor(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_CYAN);
		break;
	case HIGHLIGHT_NONE:
	default:
		lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);
		break;
	}
	lcd_console_setpos(MENUTOP+cursor, INDENT);
	lcd_puts(opt_list[cursor]);
	if (cursor == CHANGE_BOOT_DEV) {
		print_u_boot_dev();
	}
}

int do_menu() {

	unsigned char key = 0;

		int x;
		int cursor = 0;
		u8 pwron = 0;
		int ignore_last_option = 1;  // assume file is missing
		int ret = 0;

                /* clear instructions at bottom */
                lcd_console_setpos(59, 31);
                lcd_console_setcolor(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_BLACK);
                lcd_puts("Hold ^ for menu");

		if (read_u_boot_device() != 'X') // if that file is there
			{ ignore_last_option = 0;};

		lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);
		// lcd_clear (NULL, 1, 1, NULL);
		lcd_console_setpos(MENUTOP-3, INDENT);
			lcd_puts(" Boot Menu");
		lcd_console_setpos(MENUTOP-2, INDENT);
			lcd_puts(" ---------             ");

		for (x=0; x < (NUM_OPTS - ignore_last_option); x++) {
		    lcd_console_setpos(MENUTOP+x, INDENT);
	            lcd_puts(opt_list[x]);
                   }
		if (ignore_last_option == 0) {
			print_u_boot_dev();
		}
		lcd_console_setpos(MENUTOP+9, INDENT);
			lcd_puts(" VOL-DOWN moves to next item");
		lcd_console_setpos(MENUTOP+10, INDENT);
			lcd_puts(" VOL-UP moves to previous item");
		lcd_console_setpos(MENUTOP+11, INDENT);
			lcd_puts(" Press ^ to select");
		lcd_console_setpos(60, 0);
			lcd_puts(" ------\n Menu by j4mm3r.\n Redone by fattire - ALPHA (" __TIMESTAMP__  ") - ** EXPERIMENTAL **");

		cursor=0;

		// highlight first option
		highlight_boot_line(cursor, HIGHLIGHT_CYAN);

		do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  // wait for release

		do {
		key = 0;
		tps65921_keypad_keys_pressed(&key);
		if (key & VOLDN_KEY) // button is pressed
		   {
			// unhighlight current option
			highlight_boot_line(cursor, HIGHLIGHT_NONE);
			cursor++;
			if (cursor == NUM_OPTS - ignore_last_option) cursor = 0;
			// highlight new option
			highlight_boot_line(cursor, HIGHLIGHT_CYAN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release

		   }

		if (key & VOLUP_KEY) // button is pressed
		   {
			// unhighlight current option
			highlight_boot_line(cursor, HIGHLIGHT_NONE);
			cursor--;
			if (cursor <0) cursor = (NUM_OPTS-ignore_last_option-1);
			// highlight new option
			highlight_boot_line(cursor, HIGHLIGHT_CYAN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release

		   }

		if ((key & HOME_KEY) && (cursor == CHANGE_BOOT_DEV)) {  //selected last option
			if (read_u_boot_device() == '1') {write_u_boot_device('0');}
				else {write_u_boot_device('1'); }
			udelay(RESET_TICK);
			highlight_boot_line(cursor, HIGHLIGHT_GREEN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release
		}
			udelay(RESET_TICK);

		} while (!(key & HOME_KEY) || (cursor == CHANGE_BOOT_DEV));  // power button to select

		highlight_boot_line(cursor, HIGHLIGHT_GREEN);

 	lcd_console_setpos(MENUTOP+9, 25);
	lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);

	switch(cursor) {

	case BOOT_EMMC_NORMAL:
		lcd_puts("    Loading (EMMC)...        ");
		break;
	case BOOT_SD_RECOVERY:
		lcd_puts("Loading Recovery from SD...  ");
		break;
	case BOOT_SD_ALTBOOT:
		lcd_puts(" Loading AltBoot from SD...  ");
		break;
	case BOOT_SD_NORMAL:
		lcd_puts("     Loading (SD)...         ");
		break;
	case BOOT_EMMC_RECOVERY:
		lcd_puts("Loading Recovery from EMMC...");
		break;
	case BOOT_EMMC_ALTBOOT:
		lcd_puts(" Loading AltBoot from EMMC...");
		break;
/*	case BOOT_FASTBOOT:
		lcd_puts(" - fastboot has started -");
		break; */
	default:
		lcd_puts("        Loading...           ");
		break;
	}

	lcd_puts("    \n                                                       \n                                                        ");
		return cursor;
}
