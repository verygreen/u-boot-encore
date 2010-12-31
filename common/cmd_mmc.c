/*
 * (C) Copyright 2003
 * Kyle Harris, kharris@nexus-tech.net
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

#if (CONFIG_COMMANDS & CFG_CMD_MMC)

#include <mmc.h>
int mmc_flag[2] = {0, 0} ;

int do_mmc (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong src_addr, dst_addr, size;
	char *cmd;
	/*Default Setting to SLOT-0*/
	int slot_no = 0, mmc_cont = 0;

	if (argc < 2) {
		goto mmc_cmd_usage;
	} else if (argc == 2) {
                if (strncmp(argv[0],"mmcinit",7) !=0) {
                        goto mmc_cmd_usage;
                } else {
			slot_no = simple_strtoul(argv[1], NULL, 16);
			if ((slot_no != 0) && (slot_no != 1))
				goto mmc_cmd_usage;
			if (mmc_init(slot_no) != 0) {
				printf("No MMC card found\n");
                                return 1;
			} else {
				mmc_flag[slot_no] = 1;
			}
                }
	} else {
		mmc_cont = simple_strtoul(argv[1], NULL, 16);
		if ((mmc_cont != 0) && (mmc_cont != 1))
			goto mmc_cmd_usage;

		if (!mmc_flag[mmc_cont]) {
			printf("Try to do init First b4 read/write\n");
			goto mmc_cmd_usage;
		}

		cmd = argv[2];
                if (strncmp(cmd, "read", 4) != 0 && strncmp(cmd, "write", 5) != 0
                                        && strncmp(cmd, "erase", 5) != 0)
                goto mmc_cmd_usage;

                if (strcmp(cmd, "erase") == 0) {
			if (argc != 5) {
				goto mmc_cmd_usage;
			} else {
				src_addr = simple_strtoul(argv[3], NULL, 16);
				size = simple_strtoul(argv[4], NULL, 16);
				mmc_erase(mmc_cont, src_addr, size);
			}
		}
                if (strcmp(cmd, "read") == 0) {
			if (argc != 6) {
                                goto mmc_cmd_usage;
                        } else {
				src_addr = simple_strtoul(argv[3], NULL, 16);
				dst_addr = simple_strtoul(argv[4], NULL, 16);
				size = simple_strtoul(argv[5], NULL, 16);
				mmc_read(mmc_cont, src_addr,
					(unsigned char *)dst_addr, size);
			}
		}
		if (strcmp(cmd, "write") == 0) {
			if (argc != 6) {
				goto mmc_cmd_usage;
			} else {
				src_addr = simple_strtoul(argv[3], NULL, 16);
				dst_addr = simple_strtoul(argv[4], NULL, 16);
				size = simple_strtoul(argv[5], NULL, 16);
				mmc_write(mmc_cont, (unsigned char *)src_addr,
							dst_addr, size);
			}
		}
	}
	return 0;

mmc_cmd_usage:
	printf("Usage:\n%s\n", cmdtp->usage);
	return 1;
}

U_BOOT_CMD(
       mmc, 6, 1, do_mmc,
       "mmc - MMC sub-system\n"
       "mmcinit <dev>\n"
       "mmc <dev> write[.i] <addr>   <offset> [size]\n",
       "mmc <dev> read[.i]  <offset> <addr>   [size]\n"
);
U_BOOT_CMD(
       mmcinit, 2, 0, do_mmc,
       "mmcinit <dev> - init mmc card (0 for MMC1, 1 for MMC2)\n",
       NULL
);

#endif  /* CFG_CMD_MMC */

