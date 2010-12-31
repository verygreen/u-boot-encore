/*
 * Copyright 2008 - 2009 (C) Wind River Systems, Inc.
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
 *
 * Part of the rx_handler were copied from the Android project. 
 * Specifically rx command parsing in the  usb_rx_data_complete 
 * function of the file bootable/bootloader/legacy/usbloader/usbloader.c
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_* 
 * They come from bootable/bootloader/legacy/libboot/flash.c
 *
 * This is their Copyright:
 * 
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <asm/byteorder.h>
#include <common.h>
#include <command.h>
#include <nand.h>
#include <fastboot.h>
#include <environment.h>

#if (CONFIG_FASTBOOT)

/* Use do_reset for fastboot's 'reboot' command */
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

#if (CONFIG_MMC)
extern int do_mmc(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#endif

/* Use do_setenv and do_saveenv to permenantly save data */
int do_saveenv (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_setenv ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
/* Use do_bootm and do_go for fastboot's 'boot' command */
int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
int do_go (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

/* Forward decl */
static int rx_handler (const unsigned char *buffer, unsigned int buffer_size);
static unsigned char * get_rx_buffer( unsigned int* buffer_size);
static void reset_handler (void);

static struct cmd_fastboot_interface interface = 
{
	.rx_handler            = rx_handler,
	.get_rx_buffer         = get_rx_buffer,
	.reset_handler         = reset_handler,
	.product_name          = NULL,
	.serial_no             = NULL,
	.storage_medium        = 0,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};

static unsigned int download_size;
static unsigned int download_bytes;
static unsigned int download_bytes_unpadded;
static unsigned int download_error;
static unsigned int mmc_controller_no;

static void set_env(char *var, char *val)
{
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };

	setenv[1] = var;
	setenv[2] = val;

	do_setenv(NULL, 0, 3, setenv);
}


static void reset_handler ()
{
	/* If there was a download going on, bail */
	download_size = 0;
	download_bytes = 0;
	download_bytes_unpadded = 0;
	download_error = 0;
}

static unsigned char * get_rx_buffer( unsigned int* buffer_size)
{
	unsigned char * buffer;

	if (download_size)
	{
		/*
		 * When we have something to download we ask usb driver to directly store
		 * data into the final buffer (to save some CPU copy and improve througput
		 */
		*buffer_size = buffer_size - download_bytes;
		buffer = interface.transfer_buffer + download_bytes;
	}
	else
	{
		*buffer_size = 0;
		buffer = NULL;
	}

	return buffer;
}

static int rx_handler (const unsigned char *buffer, unsigned int buffer_size)
{
	int   ret          = 1;
	
	/* Use 65 instead of 64
	   null gets dropped  
	   strcpy's need the extra byte */
	char response[65];

	if (download_size) 
	{
		/* Something to download */

		if (buffer_size)
		{
			/* Handle possible overflow */
			unsigned int transfer_size = 
				download_size - download_bytes;

			if (buffer_size < transfer_size)
				transfer_size = buffer_size;
			
			/* In this case we don"t need to perform any manual copy as USB driver
			 * has already stored data into the final buffer
			 */
			download_bytes += transfer_size;
			
			/* Check if transfer is done */
			if (download_bytes >= download_size) {
				/* Reset global transfer variable,
				   Keep download_bytes because it will be
				   used in the next possible flashing command */
				download_size = 0;

				if (download_error) {
					/* There was an earlier error */
					sprintf(response, "ERROR");
				} else {
					/* Everything has transferred,
					   send the OK response */
					sprintf(response, "OKAY");
				}
				fastboot_tx_status(response, strlen(response));

				printf ("\ndownloading of %d bytes finished\n",
					download_bytes);

				/* Padding is required only if storage medium is NAND */
				if (interface.storage_medium == NAND) {
					/* Pad to block length
					   In most cases, padding the download to be
					   block aligned is correct. The exception is
					   when the following flash writes to the oob
					   area.  This happens when the image is a
					   YAFFS image.  Since we do not know what
					   the download is until it is flashed,
					   go ahead and pad it, but save the true
					   size in case if should have
					   been unpadded */
					download_bytes_unpadded = download_bytes;
					if (interface.nand_block_size)
					{
						if (download_bytes %
						    interface.nand_block_size)

						{
							unsigned int pad = interface.nand_block_size - (download_bytes % interface.nand_block_size);
							unsigned int i;

							for (i = 0; i < pad; i++)
							{
								if (download_bytes >= interface.transfer_buffer_size)
									break;

								interface.transfer_buffer[download_bytes] = 0;
								download_bytes++;
							}
						}
					}
				}
			}

			/* Provide some feedback */
			if (download_bytes &&
			    0 == (download_bytes %
				  (16 * interface.nand_block_size)))
			{
				/* Some feeback that the
				   download is happening */
				if (download_error)
					printf("X");
				else
					printf(".");
				if (0 == (download_bytes %
					  (80 * 16 *
					   interface.nand_block_size)))
					printf("\n");
				
			}
		}
		else
		{
			/* Ignore empty buffers */
			printf ("Warning empty download buffer\n");
			printf ("Ignoring\n");
		}
		ret = 0;
	}
	else
	{
		/* A command */

		/* Cast to make compiler happy with string functions */
		const char *cmdbuf = (char *) buffer;

		/* Generic failed response */
		sprintf(response, "FAIL");

		/* reboot 
		   Reboot the board. */

		if(memcmp(cmdbuf, "reboot", 6) == 0) 
		{
			sprintf(response,"OKAY");
			fastboot_tx_status(response, strlen(response));
			udelay (1000000); /* 1 sec */
			
			do_reset (NULL, 0, 0, NULL);
			
			/* This code is unreachable,
			   leave it to make the compiler happy */
			return 0;
		}
		
		/* getvar
		   Get common fastboot variables
		   Board has a chance to handle other variables */
		if(memcmp(cmdbuf, "getvar:", 7) == 0) 
		{
			strcpy(response,"OKAY");
        
			if(!strcmp(cmdbuf + strlen("version"), "version")) 
			{
				strcpy(response + 4, FASTBOOT_VERSION);
			} 
			else if(!strcmp(cmdbuf + strlen("product"), "product")) 
			{
				if (interface.product_name) 
					strcpy(response + 4, interface.product_name);
			
			} else if(!strcmp(cmdbuf + strlen("serialno"), "serialno")) {
				if (interface.serial_no) 
					strcpy(response + 4, interface.serial_no);

			} else if(!strcmp(cmdbuf + strlen("downloadsize"), "downloadsize")) {
				if (interface.transfer_buffer_size) 
					sprintf(response + 4, "08x", interface.transfer_buffer_size);
			} 
			else 
			{
				fastboot_getvar(cmdbuf + 7, response + 4);
			}
			ret = 0;

		}


		/* erase
		   Erase a register flash partition
		   Board has to set up flash partitions */

		if(memcmp(cmdbuf, "erase:", 6) == 0){
			if (interface.storage_medium == NAND) {
				/* storage medium is NAND */
				
				struct fastboot_ptentry *ptn;

				ptn = fastboot_flash_find_ptn(cmdbuf + 6);
				if(ptn == 0)
				{
					sprintf(response, "FAILpartition does not exist");
				}
				else
				{
					char start[32], length[32];
					int status = 0, repeat, repeat_max;

					printf("erasing '%s'\n", ptn->name);

					char *lock[5]   = { "nand", "lock",   NULL, NULL, NULL, };
					char *unlock[5] = { "nand", "unlock", NULL, NULL, NULL,	};
					char *erase[5]  = { "nand", "erase",  NULL, NULL, NULL, };

					lock[2] = unlock[2] = erase[2] = start;
					lock[3] = unlock[3] = erase[3] = length;

					repeat_max = 1;
					if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK)
						repeat_max = ptn->flags & FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK;

					sprintf (length, "0x%x", ptn->length);
					for (repeat = 0; repeat < repeat_max; repeat++)
					{
						sprintf (start, "0x%x", ptn->start + (repeat * ptn->length));
 #if 0
						do_nand (NULL, 0, 4, unlock);
						status = do_nand (NULL, 0, 4, erase);
						do_nand (NULL, 0, 4, lock);
 #endif
 
						if (status)
							break;
					}
 
					if (status)
					{
						sprintf(response,"FAILfailed to erase partition");
					}
					else
					{
						printf("partition '%s' erased\n", ptn->name);
						sprintf(response, "OKAY");
					}
 				}
 			} else if (interface.storage_medium == EMMC) {
			/* storage medium is EMMC */

				struct fastboot_ptentry *ptn;

				/* Save the MMC controller number */
				mmc_controller_no = CFG_FASTBOOT_MMC_NO;

				/* Find the partition and erase it */
				ptn = fastboot_flash_find_ptn(cmdbuf + 6);

				if (ptn == 0) {
					sprintf(response,
					"FAIL: partition doesn't exist");
				} else {
					/* Call MMC erase function here */
					char start[32], length[32];
					char slot_no[32];

					char *erase[5]  = { "mmc", NULL, "erase",
							NULL, NULL, };
					char *mmc_init[2] = {"mmcinit", NULL,};

					mmc_init[1] = slot_no;
					erase[1] = slot_no;
					erase[3] = start;
					erase[4] = length;

					sprintf(slot_no, "%d", mmc_controller_no);
					sprintf(length, "0x%x", ptn->length);
					sprintf(start, "0x%x", ptn->start);

					printf("Initializing '%s'\n", ptn->name);
					if (do_mmc(NULL, 0, 2, mmc_init)) {
						sprintf(response, "FAIL: Init of MMC card");
					} else {
						sprintf(response, "OKAY");
					}

					printf("Erasing '%s'\n", ptn->name);
					if (do_mmc(NULL, 0, 5, erase)) {
						printf("Erasing '%s' FAILED!\n", ptn->name);
						sprintf(response, "FAIL: Erase partition");
					} else {
						printf("Erasing '%s' DONE!\n", ptn->name);
						sprintf(response, "OKAY");
					}
				}
 			}


 			ret = 0;
 		}

		/* download
		   download something .. 
		   What happens to it depends on the next command after data */
		if(memcmp(cmdbuf, "download:", 9) == 0) {

			/* save the size */
			download_size = simple_strtoul(cmdbuf + 9, NULL, 16);
			/* Reset the bytes count, now it is safe */
			download_bytes = 0;
			/* Reset error */
			download_error = 0;

			printf("Starting download of %d bytes\n",
							download_size);

			if (0 == download_size)
			{
				/* bad user input */
				sprintf(response, "FAILdata invalid size");
			} else if (download_size >
						interface.transfer_buffer_size)
			{
				/* set download_size to 0
				 * because this is an error */
				download_size = 0;
				sprintf(response, "FAILdata too large");
			}
			else
			{
				/* The default case, the transfer fits
				   completely in the interface buffer */
				sprintf(response, "DATA%08x", download_size);
			}
			ret = 0;
		}

		/* boot
		   boot what was downloaded

		   WARNING WARNING WARNING

		   This is not what you expect.
		   The fastboot client does its own packaging of the
		   kernel.  The layout is defined in the android header
		   file bootimage.h.  This layeout is copiedlooks like this,

		   **
		   ** +-----------------+
		   ** | boot header     | 1 page
		   ** +-----------------+
		   ** | kernel          | n pages
		   ** +-----------------+
		   ** | ramdisk         | m pages
		   ** +-----------------+
		   ** | second stage    | o pages
		   ** +-----------------+
		   **

		   We only care about the kernel.
		   So we have to jump past a page.

		   What is a page size ?
		   The fastboot client uses 2048

		   The is the default value of

		   CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE

		*/

		if(memcmp(cmdbuf, "boot", 4) == 0) {

			if ((download_bytes) &&
			    (CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE < download_bytes))
			{
				char start[32];
				char *bootm[3] = { "bootm", NULL, NULL, };
				char *go[3]    = { "go",    NULL, NULL, };

				/*
				 * Use this later to determine if a command line was passed
				 * for the kernel.
				 */
				struct fastboot_boot_img_hdr *fb_hdr =
					(image_header_t *) interface.transfer_buffer;

				/* Skip the mkbootimage header */
				image_header_t *hdr =
					(image_header_t *)
					&interface.transfer_buffer[CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE];

				bootm[1] = go[1] = start;
				sprintf (start, "0x%x", hdr);

				/* Execution should jump to kernel so send the response
				   now and wait a bit.  */
				sprintf(response, "OKAY");
				fastboot_tx_status(response, strlen(response));
				udelay (1000000); /* 1 sec */

				if (ntohl(hdr->ih_magic) == IH_MAGIC) {
					/* Looks like a kernel.. */
					printf ("Booting kernel..\n");

					/*
					 * Check if the user sent a bootargs down.
					 * If not, do not override what is already there
					 */
					if (strlen ((char *) &fb_hdr->cmdline[0]))
						set_env ("bootargs", (char *) &fb_hdr->cmdline[0]);

					do_bootm (NULL, 0, 2, bootm);
				} else {
					/* Raw image, maybe another uboot */
					printf ("Booting raw image..\n");

					do_go (NULL, 0, 2, go);
				}
				printf ("ERROR : bootting failed\n");
				printf ("You should reset the board\n");
			}
			sprintf(response, "FAILinvalid boot image");
			ret = 0;
		}

		/* flash
		   Flash what was downloaded */
		if (memcmp(cmdbuf, "flash:", 6) == 0) {
			if (interface.storage_medium == NAND) {
				/* storage medium is NAND */

				if (download_bytes)
				{
					struct fastboot_ptentry *ptn;

					ptn = fastboot_flash_find_ptn(cmdbuf + 6);
					if (ptn == 0) {
						sprintf(response, "FAILpartition does not exist");
					} else if ((download_bytes > ptn->length) &&
						   !(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
						sprintf(response, "FAILimage too large for partition");
						/* TODO : Improve check for yaffs write */
 					} else {
						/* Check if this is not really a flash write
						   but rather a saveenv */
						if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
					//		/* Since the response can only be 64 bytes,
					//		   there is no point in having a large error message. */
					//		char err_string[32];
					//		if (saveenv_to_ptn(ptn, &err_string[0])) {
					//			printf("savenv '%s' failed : %s\n", ptn->name, err_string);
					//			sprintf(response, "FAIL%s", err_string);
					//		} else {
					//			printf("partition '%s' saveenv-ed\n", ptn->name);
					//			sprintf(response, "OKAY");
					//		}
 						} else {
							/* Normal case */
					//		if (write_to_ptn(ptn)) {
					//			printf("flashing '%s' failed\n", ptn->name);
					//			sprintf(response, "FAILfailed to flash partition");
					//		} else {
					//			printf("partition '%s' flashed\n", ptn->name);
					//			sprintf(response, "OKAY");
					//		}
 						}
 					}
 				}
				else
				{
					sprintf(response, "FAILno image downloaded");
				}
			} else if (interface.storage_medium == EMMC) {
				/* storage medium is EMMC */

				if (download_bytes) {

		                        struct fastboot_ptentry *ptn;

		                        /* Save the MMC controller number */
		                        mmc_controller_no = CFG_FASTBOOT_MMC_NO;

		                        /* Next is the partition name */
		                        ptn = fastboot_flash_find_ptn(cmdbuf + 6);

		                        if (ptn == 0) {
						printf("Partition:'%s' does not exist\n", ptn->name);
		                                sprintf(response, "FAILpartition does not exist");
					} else if ((download_bytes > ptn->length) &&
		                                   !(ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV)) {
						printf("Image too large for the partition\n");
		                                sprintf(response, "FAILimage too large for partition");
		                //      } else if (ptn->flags & FASTBOOT_PTENTRY_FLAGS_WRITE_ENV) {
				//		/* Check if this is not really a flash write,
				//		 * but instead a saveenv
				//		 */
				//		unsigned int i = 0;
				//		/* Env file is expected with a NULL delimeter between
				//		 * env variables So replace New line Feeds (0x0a) with
				//		 * NULL (0x00)
				//		 */
				//		for (i=0; i < download_bytes; i++) {
				//			if (interface.transfer_buffer[i] == 0x0a)
				//				interface.transfer_buffer[i] = 0x00;
				//		}
				//		memset(env_ptr->data, 0, ENV_SIZE);
				//		memcpy(env_ptr->data, interface.transfer_buffer, download_bytes);
				//		do_saveenv (NULL, 0, 1, NULL);
				//		printf("saveenv to '%s' DONE!\n", ptn->name);
				  //              sprintf(response, "OKAY");
					} else {
					/* Normal case */

						char source[32], dest[32], length[32];
						char slot_no[32];

						printf("writing to partition '%s'\n", ptn->name);
						char *mmc_write[6]  = {"mmc", NULL, "write", NULL, NULL, NULL};
						char *mmc_init[2] = {"mmcinit", NULL,};

						mmc_init[1] = slot_no;
						mmc_write[1] = slot_no;
						mmc_write[3] = source;
						mmc_write[4] = dest;
						mmc_write[5] = length;

						sprintf(slot_no, "%d", mmc_controller_no);
						sprintf(source, "0x%x", interface.transfer_buffer);
						sprintf(dest, "0x%x", ptn->start);
						sprintf(length, "0x%x", download_bytes);

						printf("Initializing '%s'\n", ptn->name);
						if (do_mmc(NULL, 0, 2, mmc_init)) {
							sprintf(response, "FAIL:Init of MMC card");
						} else {
							sprintf(response, "OKAY");
						}

						printf("Writing '%s'\n", ptn->name);
						if (do_mmc(NULL, 0, 6, mmc_write)) {
							printf("Writing '%s' FAILED!\n", ptn->name);
						        sprintf(response, "FAIL: Write partition");
						} else {
							printf("Writing '%s' DONE!\n", ptn->name);
						        sprintf(response, "OKAY");
						}
					}

				} else {
		                        sprintf(response, "FAILno image downloaded");
		                }

			}
			
			ret = 0;
		}

		fastboot_tx_status(response, strlen(response));

	} /* End of command */
	
	return ret;
}


	
int do_fastboot (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;

	/* Initialize the board specific support */
	if (0 == fastboot_init(&interface))
	{
		printf ("Disconnect USB cable to finish fastboot..\n");
		
		/* If we got this far, we are a success */
		ret = 0;

		/* On disconnect or error, polling returns non zero */
		while (1)
		{
			if (fastboot_poll())
				break;
		}
	}

	/* Reset the board specific support */
	printf ("fastboot shutdown\n");	
	fastboot_shutdown();
	
	return ret;
}

U_BOOT_CMD(
	fastboot,	1,	1,	do_fastboot,
	"fastboot- use USB Fastboot protocol\n",
	NULL
);


/* To support the Android-style naming of flash */
#define MAX_PTN 16

static fastboot_ptentry ptable[MAX_PTN];
static unsigned int pcount = 0;

void fastboot_flash_add_ptn(fastboot_ptentry *ptn)
{
    if(pcount < MAX_PTN){
        memcpy(ptable + pcount, ptn, sizeof(*ptn));
        pcount++;
    }
}

void fastboot_flash_dump_ptn(void)
{
    unsigned int n;
    for(n = 0; n < pcount; n++) {
        fastboot_ptentry *ptn = ptable + n;
        printf("ptn %d name='%s' start=%d len=%d\n",
                n, ptn->name, ptn->start, ptn->length);
    }
}


fastboot_ptentry *fastboot_flash_find_ptn(const char *name)
{
    unsigned int n;
    
    for(n = 0; n < pcount; n++) {
	    /* Make sure a substring is not accepted */
	    if (strlen(name) == strlen(ptable[n].name))
	    {
		    if(0 == strcmp(ptable[n].name, name))
			    return ptable + n;
	    }
    }
    return 0;
}

fastboot_ptentry *fastboot_flash_get_ptn(unsigned int n)
{
    if(n < pcount) {
        return ptable + n;
    } else {
        return 0;
    }
}

unsigned int fastboot_flash_get_ptn_count(void)
{
    return pcount;
}



#endif	/* CONFIG_FASTBOOT */

