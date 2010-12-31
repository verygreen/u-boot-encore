/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Add to readline cmdline-editing by
 * (C) Copyright 2005
 * JinHua Luo, GuangDong Linux Center, <luo.jinhua@gd-linux.com>
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


/* #define	DEBUG	*/

#include <common.h>
#include <watchdog.h>
#include <command.h>
#ifdef CONFIG_MODEM_SUPPORT
#include <malloc.h>		/* for free() prototype */
#endif

#ifdef CFG_HUSH_PARSER
#include <hush.h>
#endif

#include <bootcmdblk.h>
#include <fastboot.h>
#include <post.h>
#include <asm/arch/mux.h>
#include <asm/io.h>
#ifdef CONFIG_3621EVT1A
#include <i2c.h>
#endif
#ifdef CONFIG_OMAP3430
#include <asm/arch/sys_proto.h>
#endif
#include "power/gpio.h"
#ifdef CONFIG_SILENT_CONSOLE
DECLARE_GLOBAL_DATA_PTR;
#endif

#include <tps65921.h>

#if defined(CONFIG_BOOT_RETRY_TIME) && defined(CONFIG_RESET_TO_RETRY)
extern int do_reset (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);		/* for do_reset() prototype */
#endif

extern int do_bootd (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);


#define MAX_DELAY_STOP_STR 32

static int parse_line (char *, char *[]);
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
static int abortboot(int);
#endif

#undef DEBUG_PARSER

char        console_buffer[CFG_CBSIZE];		/* console I/O buffer	*/

#ifndef CONFIG_CMDLINE_EDITING
static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen);
static char erase_seq[] = "\b \b";		/* erase sequence	*/
static char   tab_seq[] = "        ";		/* used to expand TABs	*/
#endif /* CONFIG_CMDLINE_EDITING */

#ifdef CONFIG_BOOT_RETRY_TIME
static uint64_t endtime = 0;  /* must be set, default is instant timeout */
static int      retry_time = -1; /* -1 so can call readline before main_loop */
#endif

#define	endtick(seconds) (get_ticks() + (uint64_t)(seconds) * get_tbclk())

#ifndef CONFIG_BOOT_RETRY_MIN
#define CONFIG_BOOT_RETRY_MIN CONFIG_BOOT_RETRY_TIME
#endif

#ifdef CONFIG_MODEM_SUPPORT
int do_mdm_init = 0;
extern void mdm_init(void); /* defined in board.c */
#endif

extern int max17042_init(void);
extern int max17042_temp( uint32_t* val);
extern int max17042_voltage( uint16_t* val);

extern void tps65921_poweroff(void);

/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot
 *          1 - got key string, abort
 */
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
# if defined(CONFIG_AUTOBOOT_KEYED)
static __inline__ int abortboot(int bootdelay)
{
	int abort = 0;
	uint64_t etime = endtick(bootdelay);
	struct
	{
		char* str;
		u_int len;
		int retry;
	}
	delaykey [] =
	{
		{ str: getenv ("bootdelaykey"),  retry: 1 },
		{ str: getenv ("bootdelaykey2"), retry: 1 },
		{ str: getenv ("bootstopkey"),   retry: 0 },
		{ str: getenv ("bootstopkey2"),  retry: 0 },
	};

	char presskey [MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT) {
		/* Restore serial console */
		console_assign (stdout, "serial");
		console_assign (stderr, "serial");
	}
#endif

#  ifdef CONFIG_AUTOBOOT_PROMPT
	printf (CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif

#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_DELAY_STR2
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_DELAY_STR2;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[2].str == NULL)
		delaykey[2].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR2
	if (delaykey[3].str == NULL)
		delaykey[3].str = CONFIG_AUTOBOOT_STOP_STR2;
#  endif

	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen (delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;

		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;

#  if DEBUG_BOOTKEYS
		printf("%s key:<%s>\n",
		       delaykey[i].retry ? "delay" : "stop",
		       delaykey[i].str ? delaykey[i].str : "NULL");
#  endif
	}

	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	while (!abort && get_ticks() <= etime) {
		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i ++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
			    memcmp (presskey + presskey_len - delaykey[i].len,
				    delaykey[i].str,
				    delaykey[i].len) == 0) {
#  if DEBUG_BOOTKEYS
				printf("got %skey\n",
				       delaykey[i].retry ? "delay" : "stop");
#  endif

#  ifdef CONFIG_BOOT_RETRY_TIME
				/* don't retry auto boot */
				if (! delaykey[i].retry)
					retry_time = -1;
#  endif
				abort = 1;
			}
		}

		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey [presskey_len ++] = getc();
			}
			else {
				for (i = 0; i < presskey_max - 1; i ++)
					presskey [i] = presskey [i + 1];

				presskey [i] = getc();
			}
		}
	}
#  if DEBUG_BOOTKEYS
	if (!abort)
		puts ("key timeout\n");
#  endif

#ifdef CONFIG_SILENT_CONSOLE
	if (abort) {
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if (gd->flags & GD_FLG_SILENT) {
		/* Restore silent console */
		console_assign (stdout, "nulldev");
		console_assign (stderr, "nulldev");
	}
#endif

	return abort;
}

# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */

#ifdef CONFIG_MENUKEY
static int menukey = 0;
#endif

static __inline__ int abortboot(int bootdelay)
{
	int abort = 0;
	int ch=0;
#ifdef  CONFIG_ABORTKEY_COUNT
	int akcount=0;
#endif

#ifdef CONFIG_SILENT_CONSOLE
	if (gd->flags & GD_FLG_SILENT) {
		/* Restore serial console */
		console_assign (stdout, "serial");
		console_assign (stderr, "serial");
	}
#endif

#ifdef CONFIG_MENUPROMPT
	printf(CONFIG_MENUPROMPT, bootdelay);
#else
#ifdef CONFIG_ABORTPROMPT
	printf(CONFIG_ABORTPROMPT ,bootdelay);
#else
	printf("Hit any key to stop autoboot: %2d ", bootdelay);
#endif
#endif

#if defined CONFIG_ZERO_BOOTDELAY_CHECK
	/*
	 * Check if key already pressed
	 * Don't check if bootdelay < 0
	 */
	if (bootdelay >= 0) {
		if (tstc()) {	/* we got a key press	*/
			(void) getc();  /* consume input	*/
			puts ("\b\b\b 0");
			abort = 1; 	/* don't auto boot	*/
		}
	}
#endif

	while ((bootdelay > 0) && (!abort)) {
		int i;

		--bootdelay;
		/* delay 100 * 10ms */
		for (i=0; !abort && i<100; ++i) {
			if (tstc()) {	/* we got a key press	*/
				bootdelay = 0;	/* no more delay	*/
# ifdef CONFIG_MENUKEY
				menukey = getc();
# else
				ch = getc();  /* consume input	*/
# endif
#ifdef  CONFIG_ABORTKEY
				if (ch == CONFIG_ABORTKEY ) {
#ifdef CONFIG_ABORTKEY_COUNT
					akcount++;
					if (akcount>=CONFIG_ABORTKEY_COUNT) {
						abort=1;
					}
#else
					abort  = 1;	/* don't auto boot	*/
#endif
				}
#else
				abort  = 1;	/* don't auto boot	*/
#endif

				break;
			}
			udelay (10000);
		}

		printf ("\b\b\b%2d ", bootdelay);
	}

	putc ('\n');

#ifdef CONFIG_SILENT_CONSOLE
	if (abort) {
		/* permanently enable normal console output */
		gd->flags &= ~(GD_FLG_SILENT);
	} else if (gd->flags & GD_FLG_SILENT) {
		/* Restore silent console */
		console_assign (stdout, "nulldev");
		console_assign (stderr, "nulldev");
	}
#endif

	return abort;
}
# endif	/* CONFIG_AUTOBOOT_KEYED */
#endif	/* CONFIG_BOOTDELAY >= 0  */

#ifdef CONFIG_3621EVT1A

#define CONVERT_X(v) 		#v
#define CONVERT(v)		CONVERT_X(v)

#define 	ENCORE_BOOTUP_THRESHOLD	10

extern int max17042_init(void);
extern int max17042_soc( uint16_t* val );

#endif
/****************************************************************************/

#ifdef CONFIG_3621EVT1A


#define CONVERT_X(v) 		#v
#define CONVERT(v)		CONVERT_X(v)
#define LOADMMC 		"run autodetectmmc; run loadmax17042;" "\0"
#define LOADMAX17042		"if fatload mmc ${mmcbootdev}:2 " CONVERT(LOAD_ADDRESS) " max17042.bin 100; then "  \
						" echo Max17042 Gas Gauge init data loaded; " \
						" else echo Max17042 Gas Gauge init data is NOT found; " \
						" fi" "\0"
						
#define 	ENCORE_BOOTUP_THRESHOLD_CAP	10 
/* Android gives the "low battery dialog" at 10% so we will prevent bootup if RSOC < 10% */
/* We don't want to prevent boot-up until user has seen that dialog. Android really shuts off at 5%. */

#define 	ENCORE_BOOTUP_THRESHOLD_VOLTAGE 3700000    /*micro volts*/

#define 	ENCORE_BATT_TEMP_THRESHOLD	45000000   /*set 45 Celcius as the current temp threshold*/

#define BATTERY_V_RESOLUTION 625
#define CHARGER_DC      0x3
#define CHARGER_USB     0x2

#define RESET_TICK (100000) // 100ms
#define RESET_SECOND (1000000 / RESET_TICK)

// HOME * PWR need to be held for this period
#define FACTORY_RESET_DELAY 4

#define FACTORY_RESET_LOOP (RESET_SECOND * FACTORY_RESET_DELAY)

extern int max17042_init(void);
extern int max17042_soc( uint16_t* val );
extern void lcd_drawchars (ushort x, ushort y, uchar *str, int count);
extern int is_max17042_por(void);
static void max17042_preinit(void)
{	
	setenv("loadmax17042", LOADMAX17042);
	run_command( LOADMMC, 0 );

	return;
}


#define GET_BCB_BOOT_CODE 	"run autodetectmmc;fatload mmc ${mmcromdev}:2 0x81c00000 BCB 0x1000;" "\0"
static int get_bcb_boot_code(void)
{	
	int bootcode;
	run_command( GET_BCB_BOOT_CODE, 0 );
	bootcode = *((unsigned char *)0x81c00040);
	return bootcode;
}


void backlight_control(int on_off)
{

}

static void power_off(void)
{
#ifdef CONFIG_LCD
	extern void lcd_disable(void);

	lcd_disable();
#endif
	tps65921_poweroff();

	while (1) {
	    udelay( 1000 * 1000 );
	    printf("power_off(): failed!\n");
	    tps65921_poweroff();
	}
}
extern int lcd_clear (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
extern void lcd_enable(void);
extern void bitmap_plot (int x, int y,uchar which);
extern void lcd_adjust_brightness(int level);
extern void boxer_disable_panel(void);
extern void boxer_init_panel(void);
extern void encore_button_rtc_ack(void);

/*this is where we handle dead battery scenario. We spin idle and wait for battery to be charged over
  threshold to continue to boot to kernel*/
static void Encore_boot(void)
{
	uint16_t buf_v=0;
	uint16_t buf_soc=0;
	int i ;
	unsigned char key_pad = 0;
	int user_req = 0;
	int microvolts=0;

	/*bit0 of charger_type represents WALL CHARGER*/
	/*bit1 of charger_type represents USB  CHARGER*/
	uint8_t  charger_type=0;
	uint8_t  cap_ok = 0,vol_ok = 0;
   uint8_t  reg=0;
	int btn;
	long boot_code;
	int  bcb_code;
	int  battery_por;
	int  warm_reset=0;
	uint8_t  ui_on=0;
	uint8_t  boot_normal=0;

	boot_code= *((long *)OMAP_SCRATCHPAD_BOOT_CODE);
	printf("omap scratchpad boot code %x\n",boot_code);

	bcb_code = get_bcb_boot_code();
	printf("bcb code = %x\n",bcb_code);

	battery_por = is_max17042_por();
	printf("max17042 por = %d\n",battery_por);

	warm_reset = (boot_code & 0xffff0000) == (('B' << 24) | ('M' << 16));
	setenv("resettype", warm_reset ? "warm" : "cold");
	printf("%s reset\n", getenv("resettype"));

   /*check if charger is connected */   
   if( (! gpio_pin_read(114)) || (! gpio_pin_read(115) ) ) 
		   charger_type=1;
	

	// check power button for press, as long as it is not a warm reset or an update
	if ( !warm_reset && bcb_code == 0 && !battery_por) {
     /*just check if we see signal on DC or VBUS*/	
		
		gpio_pin_init(14,GPIO_INPUT,GPIO_LOW); 
		btn=gpio_pin_read(14);
		printf("power button : %d\n",btn);
		printf("charger presence     : %d\n",charger_type);
		if (!btn  && (charger_type==0)) {
			encore_button_rtc_ack();
			tps65921_poweroff();
		}
	}
  /*enable LCD panel*/
  lcd_enable();
  
  /*triton detection for USB/WALL charger*/
  
   reg=6;
   i2c_write(0x48,0xc,1,&reg,1);
   
   reg=8;
   i2c_write(0x48,0x5,1,&reg,1); 
    
   reg=2;
   /*enable HW detection*/                
   i2c_write(0x4a,0x76,1,&reg,1);
   
   reg=1;
   i2c_write(0x4a,0x76,1,&reg,1);
  
   
	//These have to be done before bootcmd
	max17042_preinit();	
	
	// Read the keypad and store any keys pressed during start
	tps65921_keypad_keys_pressed(&key_pad);

	// Check if the user request recovery by holding PWR & HOME keys
	if (gpio_pin_read(14) && (key_pad & HOME_KEY))
	    user_req = 1;

	
	i=0;
	while ( max17042_init() != 0 ) {
	    if ( i++ > 30 ) {
		printf("Giving up waiting for battery.  Power off\n");
		power_off();
	    }

	    udelay( 1000 * 1000 );
	}

	if ( max17042_voltage(&buf_v) ) {
	    printf("Failed to read intial battery voltage\n");
	}
	else {
	    microvolts = buf_v*BATTERY_V_RESOLUTION;
	    printf("Battery Initial Reading is (%d)uV \n", microvolts );
	}


	if ( max17042_soc( &buf_soc ) ) {
	    printf("Failed to read initial battery capacity\n");
	}
	else {
	    printf("Battery Initial SOC (capacity) is %d\n", buf_soc);
	    cap_ok = buf_soc >= ENCORE_BOOTUP_THRESHOLD_CAP;
	}
		/*detect charger type 
		 1. WALL charger
		 2. USB  charger
		 3. other charger will default to USB */
	if(charger_type){

	charger_type=0;
	reg=0;
   i2c_read(0x4a,0x76,1,&reg,1);
   printf("DTCT == 0x%X \n",reg);
     if((reg&0xC)==0x8 )
      	{
      		printf("WALL CHARGER DETECTED!\n");
      		gpio_pin_init(61,GPIO_OUTPUT,GPIO_HIGH);
  		 		charger_type =1;
  		 	}
  		else if((reg&0xC)==0x4){
  				printf("USB  CHARGER DETECTED!\n"); 
  				charger_type =2;
  			}
  	   else {
  	   		printf("Third Party CHARGER DETECTED! Default to USB CHARGER\n"); 
  				charger_type =2;
  	   }
 	}
	/*we have enough juice in battery*/	
	if(cap_ok){
		     /*since we have enough juice, we boost up the brightness*/
		     lcd_adjust_brightness(80);
		     bitmap_plot (1008,133,0);
		     boot_normal = 1;
   }
   else{
		if(charger_type){
	 			bitmap_plot (928,140,1);
	 	}
	   else{ 
	         /*if no charger and we dont have enouf juice,display the info and shut it down*/
	   	   bitmap_plot (928,140,2);
  	    	 	udelay( 3000 * 1000 );
	    	   power_off();
	   }
   }
   
	ui_on=1;
	if(charger_type){
       /*enable the charge*/		 
	    gpio_pin_init(110, GPIO_OUTPUT, GPIO_LOW);
	    /*low battery charging is done here to avoid bring complicated scenarios into kernel*/
	    if(!cap_ok){
		/*charger is in,we enter dead battery charging here*/
			udelay(1000*1000);
		
			/*USB charger,dim the icon slowly and turn BL Off*/
			if(charger_type==2)
				 {
				 		for(i=0;i<4;i++)
				 			{
				 				udelay(2000*1000);
				 				lcd_adjust_brightness(100-20*i);
				 			}
				 	 boxer_disable_panel();
				 	 ui_on=0;
				 }
	    gpio_pin_init(14,GPIO_INPUT,GPIO_LOW); 
		
	    /*need SOC > 11% and VOL > 3.8V*/
	    while((!cap_ok) || (!vol_ok))
	    {
	    	 /*if user hold the PWR button, we display the ICON for 2 seconds and then off*/
	       if( charger_type==2 )
	    	 { 	
	    	   if(gpio_pin_read(14))
	    	 	{
	    	  			
	    	  			printf("User press button when usb charging!\n");
						/*display the battery icon and dim off*/	    	  			
	    	  			if(!ui_on)
	    	  			{
	    	  			boxer_init_panel();
	    	  			lcd_adjust_brightness(40);
	    	  			udelay(3000*1000);
	    	  			boxer_disable_panel();
	    	  			ui_on=0;
	    	  			}
	    	  			
	    	  	}
	    	 
	    	 }	
	    	 /*make sure charger is present since we dont have IRQ ready in boot*/
	    	 if( gpio_pin_read(114) && gpio_pin_read(115) )
	    	 {
	    	 /*warn user shutting down before LCD goes off*/
	    	 if(!ui_on)
	    	 		{
	    	 				boxer_init_panel();
	    	 				ui_on=1;
	    	 		}
	    	 lcd_clear(0,0,0,0);
	    	 bitmap_plot (928,140,2);
	    	 udelay( 3000 * 1000 );
	    	 power_off();
	    	 
	    	 }
	    	 /*measure the V and SOC*/
	    	 udelay(1000*1000);
	       max17042_voltage(&buf_v);
	       microvolts = buf_v*BATTERY_V_RESOLUTION;
	       printf("Low Battery ----->  (%d)uV \n", microvolts );
	       vol_ok = microvolts>ENCORE_BOOTUP_THRESHOLD_VOLTAGE;
	       udelay( 1000 * 1000 );
	       max17042_soc( &buf_soc );
	       printf("Low Battery ----->  (%d)%% \n", buf_soc );
	    	 cap_ok = buf_soc >= ENCORE_BOOTUP_THRESHOLD_CAP;
	           
	    }
	   
       }
       /*finished low battery charging or we have a healthy battery already*/
       printf("BOOTING!\n");
       if( !boot_normal)
       {
				if(!ui_on)       		
       			boxer_init_panel();
       		lcd_adjust_brightness(80);
       		printf("re-enabling LCD-BL after usb charging!\n ");
       		lcd_clear(0,0,0,0);
       		 bitmap_plot (1008,133,0);
       }
    }
       /*no charger and low battery*/
       else{
	    if ( !cap_ok ) {
		    printf("Battery insufficient to boot and no charger. Power off.\n");
		    udelay( 3000 * 1000 );
		    power_off();
	    }
	}
    /*battery ok to boot*/
    if(cap_ok){
		if (user_req) {
			for (i = 0; i < FACTORY_RESET_LOOP; i++)
			{
				int ret;
				unsigned char key;
				
				key = 0;
				ret = tps65921_keypad_keys_pressed(&key);
				if (!(key_pad & HOME_KEY)) {
					user_req = 0;
					printf("HOME_KEY held for less than %d Sec\n", FACTORY_RESET_DELAY);
					break;
				}
					
				if (!gpio_pin_read(14)) {
					user_req = 0;
					printf("POWER_KEY held for less than %d Sec\n", FACTORY_RESET_DELAY);					
					break;
				}
				
				if (((i+RESET_SECOND) % RESET_SECOND) == 0)
				{
					printf("Factory reset count = %d\n", i / RESET_SECOND);	
				}
				udelay(RESET_TICK);
			}
		}
		if (user_req) {
			  setenv("forcerecovery", "2");
			  printf("Booting into Factory Reset Kernel\n");
		}
		else {
			setenv("forcerecovery", "0");
			printf("Booting into Normal Kernel\n");

		     /* note: this does not currently over-write what is in the bcb.
		      * Action on forcerecovery == 0 could read back the bcb and
		      * clear it if it is currently 'recovery -- charging.zip, but
		      * this will generally only happen in development when batteries
		      * are swapped.  In this case it'll just boot into recovery and
		      * reboot again into normal OS fairly quickly, so no special 
		      * code to handle that case.
		      */
		}
	}
   
}
#endif						

void main_loop (void)
{
#ifndef CFG_HUSH_PARSER
	static char lastcommand[CFG_CBSIZE] = { 0, };
	int len;
	int rc = 1;
	int flag;
#endif

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	char *s;
	int bootdelay;
#endif
#ifdef CONFIG_PREBOOT
	char *p;
#endif
#ifdef CONFIG_BOOTCOUNT_LIMIT
	unsigned long bootcount = 0;
	unsigned long bootlimit = 0;
	char *bcs;
	char bcs_set[16];
#endif /* CONFIG_BOOTCOUNT_LIMIT */

#if defined(CONFIG_VFD) && defined(VFD_TEST_LOGO)
	ulong bmp = 0;		/* default bitmap */
	extern int trab_vfd (ulong bitmap);

#ifdef CONFIG_MODEM_SUPPORT
	if (do_mdm_init)
		bmp = 1;	/* alternate bitmap */
#endif
	trab_vfd (bmp);
#endif	/* CONFIG_VFD && VFD_TEST_LOGO */

#ifdef CONFIG_BOOTCOUNT_LIMIT
	bootcount = bootcount_load();
	bootcount++;
	bootcount_store (bootcount);
	sprintf (bcs_set, "%lu", bootcount);
	setenv ("bootcount", bcs_set);
	bcs = getenv ("bootlimit");
	bootlimit = bcs ? simple_strtoul (bcs, NULL, 10) : 0;
#endif /* CONFIG_BOOTCOUNT_LIMIT */

#ifdef CONFIG_MODEM_SUPPORT
	debug ("DEBUG: main_loop:   do_mdm_init=%d\n", do_mdm_init);
	if (do_mdm_init) {
		char *str = strdup(getenv("mdm_cmd"));
		setenv ("preboot", str);  /* set or delete definition */
		if (str != NULL)
			free (str);
		mdm_init(); /* wait for modem connection */
	}
#endif  /* CONFIG_MODEM_SUPPORT */

#ifdef CONFIG_VERSION_VARIABLE
	{
		extern char version_string[];

		setenv ("ver", version_string);  /* set version variable */
	}
#endif /* CONFIG_VERSION_VARIABLE */

#ifdef CFG_HUSH_PARSER
	u_boot_hush_start ();
#endif

#ifdef CONFIG_AUTO_COMPLETE
	install_auto_complete();
#endif
/* Disabling the check of 'OK' key press to reduce the boot time. */
#if 0
	if (fastboot_preboot())
		run_command("fastboot", 0);

#endif
#ifdef CONFIG_PREBOOT
	if ((p = getenv ("preboot")) != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

# ifndef CFG_HUSH_PARSER
		run_command (p, 0);
# else
		parse_string_outer(p, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */

#ifdef CONFIG_OMAP3430
	get_boot_device();
#endif
#ifdef CONFIG_3621EVT1A 
	Encore_boot();
#endif

#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	s = getenv ("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	debug ("### main_loop entered: bootdelay=%d\n\n", bootdelay);

# ifdef CONFIG_BOOT_RETRY_TIME
	init_cmd_timeout ();
# endif	/* CONFIG_BOOT_RETRY_TIME */

#ifdef CONFIG_BOOTCOUNT_LIMIT
	if (bootlimit && (bootcount > bootlimit)) {
		printf ("Warning: Bootlimit (%u) exceeded. Using altbootcmd.\n",
		        (unsigned)bootlimit);
		s = getenv ("altbootcmd");
	}
	else
#endif /* CONFIG_BOOTCOUNT_LIMIT */
	{
#ifdef CONFIG_RECOVERYCMD
		switch(bootcmdblk_parse_cmd())
		{
		case BOOTCMDBLK_RECOVERY:
			s = getenv ("recoverycmd");
			break;
		default:
#endif
			s = getenv ("bootcmd");
#ifdef CONFIG_RECOVERYCMD
			break;
		}
#endif
	}
	
	debug ("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (bootdelay >= 0 && s && !abortboot (bootdelay)) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

# ifndef CFG_HUSH_PARSER
		run_command (s, 0);
# else
		parse_string_outer(s, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}

# ifdef CONFIG_MENUKEY
	if (menukey == CONFIG_MENUKEY) {
	    s = getenv("menucmd");
	    if (s) {
# ifndef CFG_HUSH_PARSER
		run_command (s, 0);
# else
		parse_string_outer(s, FLAG_PARSE_SEMICOLON |
				    FLAG_EXIT_FROM_LOOP);
# endif
	    }
	}
#endif /* CONFIG_MENUKEY */
#endif	/* CONFIG_BOOTDELAY */

#ifdef CONFIG_AMIGAONEG3SE
	{
	    extern void video_banner(void);
	    video_banner();
	}
#endif

	/*
	 * Main Loop for Monitor Command Processing
	 */
#ifdef CFG_HUSH_PARSER
	parse_file_outer();
	/* This point is never reached */
	for (;;);
#else
	for (;;) {
#ifdef CONFIG_BOOT_RETRY_TIME
		if (rc >= 0) {
			/* Saw enough of a valid command to
			 * restart the timeout.
			 */
			reset_cmd_timeout();
		}
#endif
		len = readline (CFG_PROMPT);

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;
#ifdef CONFIG_BOOT_RETRY_TIME
		else if (len == -2) {
			/* -2 means timed out, retry autoboot
			 */
			puts ("\nTimed out waiting for command\n");
# ifdef CONFIG_RESET_TO_RETRY
			/* Reinit board to run initialization code again */
			do_reset (NULL, 0, 0, NULL);
# else
			return;		/* retry autoboot */
# endif
		}
#endif

		if (len == -1)
			puts ("<INTERRUPT>\n");
		else
			rc = run_command (lastcommand, flag);

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}
#endif /*CFG_HUSH_PARSER*/
}

#ifdef CONFIG_BOOT_RETRY_TIME
/***************************************************************************
 * initialise command line timeout
 */
void init_cmd_timeout(void)
{
	char *s = getenv ("bootretry");

	if (s != NULL)
		retry_time = (int)simple_strtol(s, NULL, 10);
	else
		retry_time =  CONFIG_BOOT_RETRY_TIME;

	if (retry_time >= 0 && retry_time < CONFIG_BOOT_RETRY_MIN)
		retry_time = CONFIG_BOOT_RETRY_MIN;
}

/***************************************************************************
 * reset command line timeout to retry_time seconds
 */
void reset_cmd_timeout(void)
{
	endtime = endtick(retry_time);
}
#endif

#ifdef CONFIG_CMDLINE_EDITING

/*
 * cmdline-editing related codes from vivi.
 * Author: Janghoon Lyu <nandy@mizi.com>
 */

#if 1	/* avoid redundand code -- wd */
#define putnstr(str,n)	do {			\
		printf ("%.*s", n, str);	\
	} while (0)
#else
void putnstr(const char *str, size_t n)
{
	if (str == NULL)
		return;

	while (n && *str != '\0') {
		putc(*str);
		str++;
		n--;
	}
}
#endif

#define CTL_CH(c)		((c) - 'a' + 1)

#define MAX_CMDBUF_SIZE		256

#define CTL_BACKSPACE		('\b')
#define DEL			((char)255)
#define DEL7			((char)127)
#define CREAD_HIST_CHAR		('!')

#define getcmd_putch(ch)	putc(ch)
#define getcmd_getch()		getc()
#define getcmd_cbeep()		getcmd_putch('\a')

#define HIST_MAX		20
#define HIST_SIZE		MAX_CMDBUF_SIZE

static int hist_max = 0;
static int hist_add_idx = 0;
static int hist_cur = -1;
unsigned hist_num = 0;

char* hist_list[HIST_MAX];
char hist_lines[HIST_MAX][HIST_SIZE];

#define add_idx_minus_one() ((hist_add_idx == 0) ? hist_max : hist_add_idx-1)

static void hist_init(void)
{
	int i;

	hist_max = 0;
	hist_add_idx = 0;
	hist_cur = -1;
	hist_num = 0;

	for (i = 0; i < HIST_MAX; i++) {
		hist_list[i] = hist_lines[i];
		hist_list[i][0] = '\0';
	}
}

static void cread_add_to_hist(char *line)
{
	strcpy(hist_list[hist_add_idx], line);

	if (++hist_add_idx >= HIST_MAX)
		hist_add_idx = 0;

	if (hist_add_idx > hist_max)
		hist_max = hist_add_idx;

	hist_num++;
}

static char* hist_prev(void)
{
	char *ret;
	int old_cur;

	if (hist_cur < 0)
		return NULL;

	old_cur = hist_cur;
	if (--hist_cur < 0)
		hist_cur = hist_max;

	if (hist_cur == hist_add_idx) {
		hist_cur = old_cur;
		ret = NULL;
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

static char* hist_next(void)
{
	char *ret;

	if (hist_cur < 0)
		return NULL;

	if (hist_cur == hist_add_idx)
		return NULL;

	if (++hist_cur > hist_max)
		hist_cur = 0;

	if (hist_cur == hist_add_idx) {
		ret = "";
	} else
		ret = hist_list[hist_cur];

	return (ret);
}

#ifndef CONFIG_CMDLINE_EDITING
static void cread_print_hist_list(void)
{
	int i;
	unsigned long n;

	n = hist_num - hist_max;

	i = hist_add_idx + 1;
	while (1) {
		if (i > hist_max)
			i = 0;
		if (i == hist_add_idx)
			break;
		printf("%s\n", hist_list[i]);
		n++;
		i++;
	}
}
#endif /* CONFIG_CMDLINE_EDITING */

#define BEGINNING_OF_LINE() {			\
	while (num) {				\
		getcmd_putch(CTL_BACKSPACE);	\
		num--;				\
	}					\
}

#define ERASE_TO_EOL() {				\
	if (num < eol_num) {				\
		int tmp;				\
		for (tmp = num; tmp < eol_num; tmp++)	\
			getcmd_putch(' ');		\
		while (tmp-- > num)			\
			getcmd_putch(CTL_BACKSPACE);	\
		eol_num = num;				\
	}						\
}

#define REFRESH_TO_EOL() {			\
	if (num < eol_num) {			\
		wlen = eol_num - num;		\
		putnstr(buf + num, wlen);	\
		num = eol_num;			\
	}					\
}

static void cread_add_char(char ichar, int insert, unsigned long *num,
	       unsigned long *eol_num, char *buf, unsigned long len)
{
	unsigned long wlen;

	/* room ??? */
	if (insert || *num == *eol_num) {
		if (*eol_num > len - 1) {
			getcmd_cbeep();
			return;
		}
		(*eol_num)++;
	}

	if (insert) {
		wlen = *eol_num - *num;
		if (wlen > 1) {
			memmove(&buf[*num+1], &buf[*num], wlen-1);
		}

		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
		while (--wlen) {
			getcmd_putch(CTL_BACKSPACE);
		}
	} else {
		/* echo the character */
		wlen = 1;
		buf[*num] = ichar;
		putnstr(buf + *num, wlen);
		(*num)++;
	}
}

static void cread_add_str(char *str, int strsize, int insert, unsigned long *num,
	      unsigned long *eol_num, char *buf, unsigned long len)
{
	while (strsize--) {
		cread_add_char(*str, insert, num, eol_num, buf, len);
		str++;
	}
}

static int cread_line(char *buf, unsigned int *len)
{
	unsigned long num = 0;
	unsigned long eol_num = 0;
	unsigned long rlen;
	unsigned long wlen;
	char ichar;
	int insert = 1;
	int esc_len = 0;
	int rc = 0;
	char esc_save[8];

	while (1) {
		rlen = 1;
		ichar = getcmd_getch();

		if ((ichar == '\n') || (ichar == '\r')) {
			putc('\n');
			break;
		}

		/*
		 * handle standard linux xterm esc sequences for arrow key, etc.
		 */
		if (esc_len != 0) {
			if (esc_len == 1) {
				if (ichar == '[') {
					esc_save[esc_len] = ichar;
					esc_len = 2;
				} else {
					cread_add_str(esc_save, esc_len, insert,
						      &num, &eol_num, buf, *len);
					esc_len = 0;
				}
				continue;
			}

			switch (ichar) {

			case 'D':	/* <- key */
				ichar = CTL_CH('b');
				esc_len = 0;
				break;
			case 'C':	/* -> key */
				ichar = CTL_CH('f');
				esc_len = 0;
				break;	/* pass off to ^F handler */
			case 'H':	/* Home key */
				ichar = CTL_CH('a');
				esc_len = 0;
				break;	/* pass off to ^A handler */
			case 'A':	/* up arrow */
				ichar = CTL_CH('p');
				esc_len = 0;
				break;	/* pass off to ^P handler */
			case 'B':	/* down arrow */
				ichar = CTL_CH('n');
				esc_len = 0;
				break;	/* pass off to ^N handler */
			default:
				esc_save[esc_len++] = ichar;
				cread_add_str(esc_save, esc_len, insert,
					      &num, &eol_num, buf, *len);
				esc_len = 0;
				continue;
			}
		}

		switch (ichar) {
		case 0x1b:
			if (esc_len == 0) {
				esc_save[esc_len] = ichar;
				esc_len = 1;
			} else {
				puts("impossible condition #876\n");
				esc_len = 0;
			}
			break;

		case CTL_CH('a'):
			BEGINNING_OF_LINE();
			break;
		case CTL_CH('c'):	/* ^C - break */
			*buf = '\0';	/* discard input */
			return (-1);
		case CTL_CH('f'):
			if (num < eol_num) {
				getcmd_putch(buf[num]);
				num++;
			}
			break;
		case CTL_CH('b'):
			if (num) {
				getcmd_putch(CTL_BACKSPACE);
				num--;
			}
			break;
		case CTL_CH('d'):
			if (num < eol_num) {
				wlen = eol_num - num - 1;
				if (wlen) {
					memmove(&buf[num], &buf[num+1], wlen);
					putnstr(buf + num, wlen);
				}

				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case CTL_CH('k'):
			ERASE_TO_EOL();
			break;
		case CTL_CH('e'):
			REFRESH_TO_EOL();
			break;
		case CTL_CH('o'):
			insert = !insert;
			break;
		case CTL_CH('x'):
			BEGINNING_OF_LINE();
			ERASE_TO_EOL();
			break;
		case DEL:
		case DEL7:
		case 8:
			if (num) {
				wlen = eol_num - num;
				num--;
				memmove(&buf[num], &buf[num+1], wlen);
				getcmd_putch(CTL_BACKSPACE);
				putnstr(buf + num, wlen);
				getcmd_putch(' ');
				do {
					getcmd_putch(CTL_BACKSPACE);
				} while (wlen--);
				eol_num--;
			}
			break;
		case CTL_CH('p'):
		case CTL_CH('n'):
		{
			char * hline;

			esc_len = 0;

			if (ichar == CTL_CH('p'))
				hline = hist_prev();
			else
				hline = hist_next();

			if (!hline) {
				getcmd_cbeep();
				continue;
			}

			/* nuke the current line */
			/* first, go home */
			BEGINNING_OF_LINE();

			/* erase to end of line */
			ERASE_TO_EOL();

			/* copy new line into place and display */
			strcpy(buf, hline);
			eol_num = strlen(buf);
			REFRESH_TO_EOL();
			continue;
		}
		default:
			cread_add_char(ichar, insert, &num, &eol_num, buf, *len);
			break;
		}
	}
	*len = eol_num;
	buf[eol_num] = '\0';	/* lose the newline */

	if (buf[0] && buf[0] != CREAD_HIST_CHAR)
		cread_add_to_hist(buf);
	hist_cur = hist_add_idx;

	return (rc);
}

#endif /* CONFIG_CMDLINE_EDITING */

/****************************************************************************/

/*
 * Prompt for input and read a line.
 * If  CONFIG_BOOT_RETRY_TIME is defined and retry_time >= 0,
 * time out when time goes past endtime (timebase time in ticks).
 * Return:	number of read characters
 *		-1 if break
 *		-2 if timed out
 */
int readline (const char *const prompt)
{
#ifdef CONFIG_CMDLINE_EDITING
	char *p = console_buffer;
	unsigned int len=MAX_CMDBUF_SIZE;
	static int initted = 0;

	if (!initted) {
		hist_init();
		initted = 1;
	}

	puts (prompt);

	cread_line(p, &len);
	return len;
#else
	char   *p = console_buffer;
	int	n = 0;				/* buffer index		*/
	int	plen = 0;			/* prompt length	*/
	int	col;				/* output column cnt	*/
	char	c;

	/* print prompt */
	if (prompt) {
		plen = strlen (prompt);
		puts (prompt);
	}
	col = plen;

	for (;;) {
#ifdef CONFIG_BOOT_RETRY_TIME
		while (!tstc()) {	/* while no incoming data */
			if (retry_time >= 0 && get_ticks() > endtime)
				return (-2);	/* timed out */
		}
#endif
		WATCHDOG_RESET();		/* Trigger watchdog, if needed */

#ifdef CONFIG_SHOW_ACTIVITY
		while (!tstc()) {
			extern void show_activity(int arg);
			show_activity(0);
		}
#endif
		c = getc();

		/*
		 * Special character handling
		 */
		switch (c) {
		case '\r':				/* Enter		*/
		case '\n':
			*p = '\0';
			puts ("\r\n");
			return (p - console_buffer);

		case '\0':				/* nul			*/
			continue;

		case 0x03:				/* ^C - break		*/
			console_buffer[0] = '\0';	/* discard input */
			return (-1);

		case 0x15:				/* ^U - erase line	*/
			while (col > plen) {
				puts (erase_seq);
				--col;
			}
			p = console_buffer;
			n = 0;
			continue;

		case 0x17:				/* ^W - erase word 	*/
			p=delete_char(console_buffer, p, &col, &n, plen);
			while ((n > 0) && (*p != ' ')) {
				p=delete_char(console_buffer, p, &col, &n, plen);
			}
			continue;

		case 0x08:				/* ^H  - backspace	*/
		case 0x7F:				/* DEL - backspace	*/
			p=delete_char(console_buffer, p, &col, &n, plen);
			continue;

		default:
			/*
			 * Must be a normal character then
			 */
			if (n < CFG_CBSIZE-2) {
				if (c == '\t') {	/* expand TABs		*/
#ifdef CONFIG_AUTO_COMPLETE
					/* if auto completion triggered just continue */
					*p = '\0';
					if (cmd_auto_complete(prompt, console_buffer, &n, &col)) {
						p = console_buffer + n;	/* reset */
						continue;
					}
#endif
					puts (tab_seq+(col&07));
					col += 8 - (col&07);
				} else {
					++col;		/* echo input		*/
					putc (c);
				}
				*p++ = c;
				++n;
			} else {			/* Buffer full		*/
				putc ('\a');
			}
		}
	}
#endif /* CONFIG_CMDLINE_EDITING */
}

/****************************************************************************/

#ifndef CONFIG_CMDLINE_EDITING
static char * delete_char (char *buffer, char *p, int *colp, int *np, int plen)
{
	char *s;

	if (*np == 0) {
		return (p);
	}

	if (*(--p) == '\t') {			/* will retype the whole line	*/
		while (*colp > plen) {
			puts (erase_seq);
			(*colp)--;
		}
		for (s=buffer; s<p; ++s) {
			if (*s == '\t') {
				puts (tab_seq+((*colp) & 07));
				*colp += 8 - ((*colp) & 07);
			} else {
				++(*colp);
				putc (*s);
			}
		}
	} else {
		puts (erase_seq);
		(*colp)--;
	}
	(*np)--;
	return (p);
}
#endif /* CONFIG_CMDLINE_EDITING */

/****************************************************************************/

int parse_line (char *line, char *argv[])
{
	int nargs = 0;

#ifdef DEBUG_PARSER
	printf ("parse_line: \"%s\"\n", line);
#endif
	while (nargs < CFG_MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t')) {
			++line;
		}

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
#ifdef DEBUG_PARSER
		printf ("parse_line: nargs=%d\n", nargs);
#endif
			return (nargs);
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf ("** Too many args (max. %d) **\n", CFG_MAXARGS);

#ifdef DEBUG_PARSER
	printf ("parse_line: nargs=%d\n", nargs);
#endif
	return (nargs);
}

/****************************************************************************/

static void process_macros (const char *input, char *output)
{
	char c, prev;
	const char *varname_start = NULL;
	int inputcnt  = strlen (input);
	int outputcnt = CFG_CBSIZE;
	int state = 0;	/* 0 = waiting for '$'	*/
			/* 1 = waiting for '(' or '{' */
			/* 2 = waiting for ')' or '}' */
			/* 3 = waiting for '''  */
#ifdef DEBUG_PARSER
	char *output_start = output;

	printf ("[PROCESS_MACROS] INPUT len %d: \"%s\"\n", strlen(input), input);
#endif

	prev = '\0';			/* previous character	*/

	while (inputcnt && outputcnt) {
	    c = *input++;
	    inputcnt--;

	    if (state!=3) {
	    /* remove one level of escape characters */
	    if ((c == '\\') && (prev != '\\')) {
		if (inputcnt-- == 0)
			break;
		prev = c;
		c = *input++;
	    }
	    }

	    switch (state) {
	    case 0:			/* Waiting for (unescaped) $	*/
		if ((c == '\'') && (prev != '\\')) {
			state = 3;
			break;
		}
		if ((c == '$') && (prev != '\\')) {
			state++;
		} else {
			*(output++) = c;
			outputcnt--;
		}
		break;
	    case 1:			/* Waiting for (	*/
		if (c == '(' || c == '{') {
			state++;
			varname_start = input;
		} else {
			state = 0;
			*(output++) = '$';
			outputcnt--;

			if (outputcnt) {
				*(output++) = c;
				outputcnt--;
			}
		}
		break;
	    case 2:			/* Waiting for )	*/
		if (c == ')' || c == '}') {
			int i;
			char envname[CFG_CBSIZE], *envval;
			int envcnt = input-varname_start-1; /* Varname # of chars */

			/* Get the varname */
			for (i = 0; i < envcnt; i++) {
				envname[i] = varname_start[i];
			}
			envname[i] = 0;

			/* Get its value */
			envval = getenv (envname);

			/* Copy into the line if it exists */
			if (envval != NULL)
				while ((*envval) && outputcnt) {
					*(output++) = *(envval++);
					outputcnt--;
				}
			/* Look for another '$' */
			state = 0;
		}
		break;
	    case 3:			/* Waiting for '	*/
		if ((c == '\'') && (prev != '\\')) {
			state = 0;
		} else {
			*(output++) = c;
			outputcnt--;
		}
		break;
	    }
	    prev = c;
	}

	if (outputcnt)
		*output = 0;

#ifdef DEBUG_PARSER
	printf ("[PROCESS_MACROS] OUTPUT len %d: \"%s\"\n",
		strlen(output_start), output_start);
#endif
}

/****************************************************************************
 * returns:
 *	1  - command executed, repeatable
 *	0  - command executed but not repeatable, interrupted commands are
 *	     always considered not repeatable
 *	-1 - not executed (unrecognized, bootd recursion or too many args)
 *           (If cmd is NULL or "" or longer than CFG_CBSIZE-1 it is
 *           considered unrecognized)
 *
 * WARNING:
 *
 * We must create a temporary copy of the command since the command we get
 * may be the result from getenv(), which returns a pointer directly to
 * the environment data, which may change magicly when the command we run
 * creates or modifies environment variables (like "bootp" does).
 */

int run_command (const char *cmd, int flag)
{
	cmd_tbl_t *cmdtp;
	char cmdbuf[CFG_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CFG_CBSIZE];
	char *str = cmdbuf;
	char *argv[CFG_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;

#ifdef DEBUG_PARSER
	printf ("[RUN_COMMAND] cmd[%p]=\"", cmd);
	puts (cmd ? cmd : "NULL");	/* use puts - string may be loooong */
	puts ("\"\n");
#endif

	clear_ctrlc();		/* forget any previous Control C */

	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CFG_CBSIZE) {
		puts ("## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */

#ifdef DEBUG_PARSER
	printf ("[PROCESS_SEPARATORS] %s\n", cmd);
#endif
	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;

			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */
#ifdef DEBUG_PARSER
		printf ("token: \"%s\"\n", token);
#endif

		/* find macros in this token and replace them */
		process_macros (token, finaltoken);

		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}

		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			printf ("Unknown command '%s' - try 'help'\n", argv[0]);
			rc = -1;	/* give up after bad command */
			continue;
		}

		/* found - check max args */
		if (argc > cmdtp->maxargs) {
			printf ("Usage:\n%s\n", cmdtp->usage);
			rc = -1;
			continue;
		}

#if (CONFIG_COMMANDS & CFG_CMD_BOOTD)
		/* avoid "bootd" recursion */
		if (cmdtp->cmd == do_bootd) {
#ifdef DEBUG_PARSER
			printf ("[%s]\n", finaltoken);
#endif
			if (flag & CMD_FLAG_BOOTD) {
				puts ("'bootd' recursion detected\n");
				rc = -1;
				continue;
			} else {
				flag |= CMD_FLAG_BOOTD;
			}
		}
#endif	/* CFG_CMD_BOOTD */

		/* OK - call function to do the command */
		if ((cmdtp->cmd) (cmdtp, flag, argc, argv) != 0) {
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

		/* Did the user stop this? */
		if (had_ctrlc ())
			return 0;	/* if stopped then not repeatable */
	}

	return rc ? rc : repeatable;
}

/****************************************************************************/

#if (CONFIG_COMMANDS & CFG_CMD_RUN)
int do_run (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	for (i=1; i<argc; ++i) {
		char *arg;

		if ((arg = getenv (argv[i])) == NULL) {
			printf ("## Error: \"%s\" not defined\n", argv[i]);
			return 1;
		}
#ifndef CFG_HUSH_PARSER
		if (run_command (arg, flag) == -1)
			return 1;
#else
		if (parse_string_outer(arg,
		    FLAG_PARSE_SEMICOLON | FLAG_EXIT_FROM_LOOP) != 0)
			return 1;
#endif
	}
	return 0;
}
#endif	/* CFG_CMD_RUN */
