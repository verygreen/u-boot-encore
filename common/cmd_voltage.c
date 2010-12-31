/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 * Tom Rix <Tom.Rix@windriver.com>
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
#include <common.h>
#include <command.h>

#if defined(CONFIG_CMD_VOLTAGE)

/* To use, the board should define its own voltag_info function */
void inline __voltage_info (void) {}

int do_voltage (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;

	if (argc < 2) {
		printf("Usage:\n%s\n", cmdtp->usage);
	} else {
		if (0 == strncmp(argv[1], "info", 4)) {
			voltage_info();
		} else {
			printf("Unsupported option:\n%s\n", argv[1]);
			printf("Usage:\n%s\n", cmdtp->usage);
		}
	}

	return ret;
}

U_BOOT_CMD(
	voltage,	2,	1,	do_voltage,
	"voltage   - Manage system voltages\n",
	"  options : \n"
	"            info - display voltage information\n"
);



#endif	/* CONFIG_CMD_VOLTAGE */
