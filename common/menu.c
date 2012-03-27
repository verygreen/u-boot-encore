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

#define NUM_OPTS		9  //number of boot options

char *opt_list[NUM_OPTS] = 	{" Internal eMMC Normal     ",
				 " Internal eMMC Recovery   ",
				 " Internal eMMC Alternate  ",
				 " SD Card Normal           ",
				 " SD Card Recovery         ",
				 " SD Card Alternate        ",
//				 " Start fastboot           ",
				 " Default Boot From:       ",
				 "       Device: ",
				 "        Image: ",
				};

// Shared sprintf buffer for fatsave/load
static char buf[64];

extern char lcd_is_enabled;

int check_device_image(enum image_dev device, const char* file) {
	char res = ((device == DEV_SD) ? 0 : 1);
	lcd_is_enabled = 0;
	sprintf(buf, "mmcinit %d; fatload mmc %d 0x%08x %s 1", res, res, &res, file);
	if (run_command(buf, 0)) {  //no such file
		res = 0;
	} else {
		res = 1;
	}
	lcd_is_enabled = 1;
	return res;
}

char read_u_boot_file(const char* file) {
	char res;
	lcd_is_enabled = 0;
	sprintf(buf, "mmcinit 1; fatload mmc 1:2 0x%08x %s 1", &res, file);
	if (run_command(buf, 0)) {  //no such file
		res = 'X'; // this is going to mean no such file, or I guess the file could have 'X'...
	}
	lcd_is_enabled = 1;
	return res;
}


int write_u_boot_file(const char* file, char value) {
	lcd_is_enabled = 0;
	sprintf(buf, "mmcinit 1; fatsave mmc 1:2 0x%08x %s 1", &value, file);
	if (run_command(buf, 0)) {
		printf("Error: Cannot write /bootdata/%s.\n", file);
		value = 0;
	}
	lcd_is_enabled = 1;
	return value;
}

// -----------------------------------

void print_u_boot_dev(void) {
	if (read_u_boot_file("u-boot.device") == '1') {
		lcd_puts("EMMC       ");
	} else {
		lcd_puts("SD         ");
	}
}

void print_u_boot_img(void) {
	if (read_u_boot_file("u-boot.altboot") == '1') {
		lcd_puts("Alternate  ");
	} else {
		lcd_puts("Normal     ");
	}
}


void highlight_boot_line(int cursor, enum highlight_type highl) {
	switch (highl) {
	case HIGHLIGHT_GRAY:
		lcd_console_setcolor(CONSOLE_COLOR_GRAY, CONSOLE_COLOR_BLACK);
		break;
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
	if (cursor == CHANGE_BOOT_IMG) {
		print_u_boot_img();
	}
}

int do_menu() {

	unsigned char key = 0;

		int valid_opt[NUM_OPTS] = {0};
		int x;
		int cursor = 0;
		u8 pwron = 0;
		int ret = 0;

//		valid_opt[BOOT_FASTBOOT] = 1;

		if (check_device_image(DEV_EMMC, "uImage") && check_device_image(DEV_EMMC, "uRamdisk"))
			valid_opt[BOOT_EMMC_NORMAL] = 1;
		if (check_device_image(DEV_EMMC, "uRecImg") && check_device_image(DEV_EMMC, "uRecRam"))
			valid_opt[BOOT_EMMC_RECOVERY] = 1;
		if (check_device_image(DEV_EMMC, "uAltImg") && check_device_image(DEV_EMMC, "uAltRam"))
			valid_opt[BOOT_EMMC_ALTBOOT] = 1;

		if (check_device_image(DEV_SD, "uImage") && check_device_image(DEV_SD, "uRamdisk"))
			valid_opt[BOOT_SD_NORMAL] = 1;
		if (check_device_image(DEV_SD, "uRecImg") && check_device_image(DEV_SD, "uRecRam"))
			valid_opt[BOOT_SD_RECOVERY] = 1;
		if (check_device_image(DEV_SD, "uAltImg") && check_device_image(DEV_SD, "uAltRam"))
			valid_opt[BOOT_SD_ALTBOOT] = 1;

		if (read_u_boot_file("u-boot.device") != 'X') // if that file is there
			valid_opt[CHANGE_BOOT_DEV] = 1;
		if (read_u_boot_file("u-boot.altboot") != 'X') // if that file is there
			valid_opt[CHANGE_BOOT_IMG] = 1;

                /* clear instructions at bottom */
                lcd_console_setpos(59, 31);
                lcd_console_setcolor(CONSOLE_COLOR_BLACK, CONSOLE_COLOR_BLACK);
                lcd_puts("Hold ^ for menu");

		lcd_console_setcolor(CONSOLE_COLOR_CYAN, CONSOLE_COLOR_BLACK);
		// lcd_clear (NULL, 1, 1, NULL);
		lcd_console_setpos(MENUTOP-3, INDENT);
			lcd_puts(" Boot Menu");
		lcd_console_setpos(MENUTOP-2, INDENT);
			lcd_puts(" ---------             ");

		for (x=0; x < NUM_OPTS; x++) {
		    lcd_console_setpos(MENUTOP+x, INDENT);
		    if ((!valid_opt[CHANGE_BOOT_DEV] && ! valid_opt[CHANGE_BOOT_IMG]) &&
			(x == DEFAULT_BOOT_STR || x == CHANGE_BOOT_DEV || x == CHANGE_BOOT_IMG))
			continue;
		    if (valid_opt[x])
			highlight_boot_line(x, HIGHLIGHT_NONE);
		    else
			highlight_boot_line(x, HIGHLIGHT_GRAY);
                   }

		lcd_console_setpos(MENUTOP+NUM_OPTS+2, INDENT);
			lcd_puts(" VOL-DOWN moves to next item");
		lcd_console_setpos(MENUTOP+NUM_OPTS+3, INDENT);
			lcd_puts(" VOL-UP moves to previous item");
		lcd_console_setpos(MENUTOP+NUM_OPTS+4, INDENT);
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
			while(!valid_opt[++cursor] || cursor >= NUM_OPTS) {
				if (cursor >= NUM_OPTS)
					cursor = -1;

			}
			// highlight new option
			highlight_boot_line(cursor, HIGHLIGHT_CYAN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release

		   }

		if (key & VOLUP_KEY) // button is pressed
		   {
			// unhighlight current option
			highlight_boot_line(cursor, HIGHLIGHT_NONE);
			while(!valid_opt[--cursor] || cursor < 0) {
				if (cursor < 0)
					cursor = NUM_OPTS;

			}
			// highlight new option
			highlight_boot_line(cursor, HIGHLIGHT_CYAN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release

		   }

		if ((key & HOME_KEY) && (cursor == CHANGE_BOOT_DEV)) {  //selected modify device
                        const char* file = "u-boot.device";
			if (read_u_boot_file(file) == '1') {write_u_boot_file(file, '0');}
				else {write_u_boot_file(file, '1'); }
			udelay(RESET_TICK);
			highlight_boot_line(cursor, HIGHLIGHT_GREEN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release
		}

		if ((key & HOME_KEY) && (cursor == CHANGE_BOOT_IMG)) {  //selected modify image
                        const char* file = "u-boot.altboot";
			if (read_u_boot_file(file) == '1') {write_u_boot_file(file, '0');}
				else {write_u_boot_file(file, '1'); }
			udelay(RESET_TICK);
			highlight_boot_line(cursor, HIGHLIGHT_GREEN);
			do {udelay(RESET_TICK);} while (tps65921_keypad_keys_pressed(&key));  //wait for release
		}
			udelay(RESET_TICK);

		} while (!(key & HOME_KEY) || (cursor == CHANGE_BOOT_DEV) || (cursor == CHANGE_BOOT_IMG));  // power button to select

		highlight_boot_line(cursor, HIGHLIGHT_GREEN);

 	lcd_console_setpos(MENUTOP+NUM_OPTS+2, 25);
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
