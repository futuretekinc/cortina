/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2009 by Cortina Systems Incorporated.                 */
/***********************************************************************/

#include "serial_cs752x.h"
#include "utils.h"
#include "common.h"
#include "config.h"
#include "sdram.h"

#define CPU0_RESET (1<<12)
#define CPU1_RESET (1<<13)

#ifdef CONFIG_MK_FPGA
#define GOLDENGATE_TWD_BASE		(0x1E000600)	/* CPU Timer & watch dog */
#else
#define GOLDENGATE_TWD_BASE		(0xF8000600)	/* CPU Timer & watch dog */
#endif

#define GOLDENGATE_WDT0_BASE	(GOLDENGATE_TWD_BASE)

#define G2_TIMER_LOAD 			0x00
#define G2_TIMER_COUNTER		0x04
#define G2_TIMER_CONTROL		0x08
#define G2_TIMER_INTSTAT		0x0C

#define G2_WDOG_LOAD			0x20
#define G2_WDOG_COUNTER			0x24
#define G2_WDOG_CONTROL			0x28
#define G2_WDOG_INTSTAT			0x2C
#define G2_WDOG_RESETSTAT		0x30
#define G2_WDOG_DISABLE			0x34

#define G2_TIMER_CONTROL_ENABLE		(1 << 0)
#define G2_TIMER_CONTROL_ONESHOT	(0 << 1)
#define G2_TIMER_CONTROL_PERIODIC	(1 << 1)
#define G2_TIMER_CONTROL_IT_ENABLE	(1 << 2)

static unsigned int g2_timer_rate = (200*1024*1024);
static int g2_margin = 10;

#define WD0_ENABLE          (1<<2)
#define WD1_ENABLE          (1<<3)
#define WD_BOTH_ENABLE      (1<<4)

#define MAX_ARGV 32
extern void mmu_cache_on(void);
extern void run_print_meter (void);
extern void init_print_meter(void);
extern void clear_print_meter(void);
extern int do_goto_uboot(void);

static cmd_t command_list[64];
static int total_command;

extern int ram_item_test(char **argv, int arg);

/* debug_Aaron */
int g2_pcie_sbphy_init(int argc, char **argv);
int g2_pcie_read_confs(int argc, char **argv);
#ifdef __AHCI_DIAGNOSTIC__
int g2_sata_initial(int argc, char **argv);
#endif
int g2_usb_test();

int memcpy(void *dest, void *srce, int size )
{
	unsigned int *pSrce, *pDest;

	size = (size + 3)/4;
	pSrce = (unsigned int *)srce;
	pDest= (unsigned int *)dest;

	while (size--){
		*pDest++=*pSrce++;
	}

	return (0);
}

/*
int memcmp(void *buf,void *buf1, int len)
{
	int i;
	char *p1 = (char*) buf;
	char *p2 = (char*) buf1;
	
#ifdef DEBUG
	serial_puts("Compare ");
	serial_put_hex_no_0x(buf);
	serial_puts(" = ");
	serial_put_hex_no_0x(buf1);
	serial_puts("\n\n");
#endif
	for (i = 0; i < len; i++) {
		if (p1[i] != p2[i]) {
	 		return (i+1);
		}
	}
	return 0;
}
*/

void memset(void *buf, int val, int len)
{
	int i;
	char *p = (char*) buf;
	
	for (i = 0 ; i < len; i++) {
		p[i] = val;
	}
	return;
}

int post_strlen(char *s1)
{
	int i = 0;
	
	if(!s1) return 0;
	
	while(*s1++)
		i++;
	return i;
}


int post_strcmp(char *s1,char *s2)
{
	unsigned char c1,c2;

	while(1){
		c1 = *s1++;
		c2 = *s2++;
		if(c1 != c2)
			return 1;
		if(!c1 || !c2)
			break;
	}

	return 0;
}

/**
 * memmove - Copy one area of memory to another
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * Unlike memcpy(), memmove() copes with overlapping areas.
 */
void * memmove(void * dest,const void *src,int count)
{
	char *tmp, *s;
	
	if (dest <= src) {
		tmp = (char *) dest;
		s = (char *) src;
		while (count--)
	 		*tmp++ = *s++;
	} else {
		tmp = (char *) dest + count;
		s = (char *) src + count;
		while (count--)
	 		*--tmp = *--s;
	}
	
	return dest;
}

/*
 * Scan through an input line and break it into "arguments".  These
 * are space delimited strings.  Return a structure which points to
 * the strings, similar to a Unix program. Multiple commands in the line
 * are separated by ; similar to sh. If we find a semi we stop processing the
 * line, terminate the current command with a null and return the start
 * of the next command in *line. parse() can then be called again to
 * process the next command on the line.
 * Note: original input is destroyed by replacing the delimiters with
 * null ('\0') characters for ease of use.
*/

void parse_prep(char **line, int *argc, char **argv)
{
        char *cp = *line;
        char *pp;
        int indx = 0;
        int semi = 0;

        while (*cp) {
                /* Skip leading spaces */
                while (*cp && *cp == ' ') cp++;
                if (!*cp) {
                        break;  // Line ended with a string of spaces
                }
                if (*cp == ';') {
                        *cp = '\0';
                        semi=1;
                        break;
                }
                
                if (indx < MAX_ARGV) 
                        argv[indx++] = cp;
                else 
                        serial_puts("Err!\n");
                
                while (*cp) {
                        if (*cp == ' ') {
                                *cp++ = '\0';
                                break;
                        } else if (*cp == ';') {
                                break;
                        } else if (*cp == '"') {
                                // Swallow quote, scan till following one
                                if (argv[indx-1] == cp) {
                                        argv[indx-1] = ++cp;
                                }
                                pp = cp;
                                while (*cp && *cp != '"') {
                                        if (*cp == '\\') {
                                                // Skip over escape - allows for escaped '"'
                                                cp++;
                                        }
                                        // Move string to swallow escapes
                                        *pp++ = *cp++;
                                }
                                if (!*cp) {
                                        serial_puts("Err!\n");
                                } else {
                                        if (pp != cp) *pp = '\0';
                                        *cp++ = '\0';
                                        break;
                                }
                        } else {
                                cp++;
                        }
                }
        }
        if (semi) 
                *line = cp + 1;
        else 
                *line = cp;
        
        *argc = indx;
}

void data_dump_w (char *ptext, int len, char *show_addr)
{
	int i;
	unsigned char linebuf[64];
	unsigned char *p;
	unsigned int *p_cur;
	volatile unsigned int val;
	
	memset(linebuf,0,64);
	
	p = linebuf;
	p_cur = (unsigned int *)(((unsigned int)ptext) & 0xfffffffc);
	
	for (i = 0; i < len; i++) {
		if ((i & 0x3) == 0) {
			serial_put_hex_no_0x((unsigned int)show_addr + i * 4);
			serial_putc(' ');
			p += 9;
		}
	        val = p_cur[i];
	        serial_put_hex_no_0x(val);
	        serial_putc(' ');
	        p += 9;
	        if ((i &0x3) == 0x3) {
			serial_putc('\n');
			p = linebuf;
			memset(linebuf, 0, 64);
		}
	}
	if ((i & 0x3) != 0x0) {
		serial_putc('\n');
	}
	return;
}


static int do_dump_memory(int argc, char **argv)
{

        if (argc >= 2) {
            int len = iros_strtol(argv[2]);
            char *starting = (char *) iros_strtol(argv[1]);
            data_dump_w(starting, len, starting);
        }

        return 0;
}

static int do_cache_process(int argc, char **argv)
{

	if (argc == 3) {
		if (tolower(argv[1][0]) == 'i') {
			if (tolower(argv[2][0]) == '0') {
				serial_puts("Disable I-Cache.\n");
				icache_disable();
			} else {
				serial_puts("Enable I-Cache.\n");
				icache_enable();
			}
		} else {
			if (tolower(argv[2][0]) == '0') {
				serial_puts("Disable D-Cache.\n");
				dcache_disable();
			} else {
				serial_puts("Enable D-Cache.\n");
				dcache_enable();
			}
		}
	} else {
    		serial_puts("c i/d-cache enable/disable\n");
    		serial_puts("	i : I-Cache  d : D-Cache \n");
    		serial_puts("	0 : Disable	 1 : Enable \n");
	}
	return 0;
}

#ifdef CONFIG_CORTINA_DIAG
static int do_mem_test(int argc, char **argv)
{
	unsigned int	iteration;
	unsigned int	ret = 0;
	unsigned int	i,j;

	if (argc == 4) {
		iteration = 1;
	}
	else if (argc == 5) {
		iteration = iros_strtol(argv[4]);
	}
	else {
		serial_puts("t id addr len iter\n");
		serial_puts("	id=0 : Address test, walking ones.\n");
		serial_puts("	   1 : Address test, own address.\n");
		serial_puts("	   2 : Moving inversions, ones and zeros sequential.\n");
		serial_puts("	   3 : Moving inversions, 8 bit wide walking ones and zeros.\n");
		serial_puts("	   4 : Moving inversions, 32 bit shifting pattern.\n");
		serial_puts("	   5 : Moving inversions, random pattern.\n");
		serial_puts("	   6 : Modulo X check, all ones and zeros.\n");
		serial_puts("	   7 : Modulo X check, 8 bit pattern.\n");
		serial_puts("	   8 : Modulo X check, Random pattern.\n");
		serial_puts("	   9 : Do all memtest86 items.\n");
		serial_puts("	  10 : SSN increment byte test.\n");
		serial_puts("	  11 : SSN increment word test.\n");
		serial_puts("	addr : DRAM start address.\n");
		serial_puts("	len  : DRAM test length.\n");
		serial_puts("	iter : DRAM test iteration.\n");
		return 1;
	}

	if (argv[1][0]=='9') {
		for (i=0; i<iteration; i++) {
			serial_puts("\n[Iteration : ");
		    	serial_put_hex((unsigned int)i+1);
		    	serial_puts(" ]\n");
			argv[1][0] = '0';
			for (j=0; j<8; j++) {
				ret = ram_item_test(argv,argc);
				if (ret == 2)	/* CTL-C */
					return 0;
				argv[1][0]++;
			}
		}
	}

	for (i=0; i<iteration; i++) {
		serial_puts("\n[Iteration : ");
	    	serial_put_hex((unsigned int)i+1);
	    	serial_puts(" ]\n");
		ret = ram_item_test(argv,argc);
		if (ret == 2)	/* CTL-C */
			return 0;
	}
	return ret;
}
#endif

void do_help(void)
{
	int i;
	serial_puts("POST\n");
	serial_puts("   cmds:\n");
	
	for(i=0;i<total_command;i++){
		serial_puts(command_list[i].help_msg);
		serial_puts("\n");
	}

}


static int do_set_memory(int argc, char **argv) {
	if ((argv[1] != 0) && (argv[2] != 0)) {
		char *addr = (char *) iros_strtol(argv[1]);
		unsigned int val = iros_strtol(argv[2]);
		/* *(unsigned int *)addr = val; */
		
		*((volatile unsigned int *)(addr)) = val;
	}
	return 0;
}


/* Reset the CPU by setting up GLOBAL_ARM_RESET register */
void reset_cpu (int argc, char **argv)
{
	unsigned int val;
	unsigned int count=0;
	
        /* To reset, use watchdog to reset whole system */
        REG_READ_UINT32(GLOBAL_GLOBAL_CONFIG, val);
        
        /* enable axi & L2 reset */
        val &= ~0x00000200;

        /* wd_enable are exclusive with wd0_reset_subsys_enable */
        val &= ~0x0000000E;

        /* reset remap, all block & subsystem */
        val |= 0x000000F0;
        
	REG_WRITE_UINT32(GLOBAL_GLOBAL_CONFIG, val);
		
	/* Reload the counter */
	REG_WRITE_UINT32(GOLDENGATE_WDT0_BASE+G2_WDOG_LOAD, 10);
	
	/* Enable watchdog - prescale=256, watchdog mode=0, enable=1 */
	REG_WRITE_UINT32(GOLDENGATE_WDT0_BASE + G2_WDOG_CONTROL, 0x0000FF09);
	
	while (1); /* loop forever and wait for reset to happen */
	/*NOT REACHED*/
}

/* Reset the CPU by setting up GLOBAL_ARM_RESET register */
void wfi_cpu (int argc, char **argv)
{
	unsigned int val;
	unsigned int count=0;

	serial_puts("Powering down ARM Core(1/1)\n");
	REG_WRITE_UINT32(0xf000003c, 1);

	serial_puts("Entering WFI for core 0\n");
	wmb();
	asm volatile("WFI");

	serial_puts("Should not get here!\n");
	
	return 0;
}



int cmd_exe_str(int argc, char **argv)
{
	int i;

	for(i=0;i<total_command;i++){
		if(post_strcmp(argv[0],command_list[i].cmd)==0){
			if(command_list[i].func_exe(argc, argv)){
				serial_puts("Syntax :\n");
				serial_puts(command_list[i].help_msg);
			}
			return 0;
		}
	}
	serial_puts("Illegal Command!\n");
	return 1;
}


int do_g(int argc, char **argv)
{
	char *addr = (char *) iros_strtol(argv[1]);
	serial_puts("Jump to ");
	serial_put_hex((unsigned int) addr);
	((void (*)(void))addr)();

	return 0;
}

int do_m(int argc, char **argv)
{
	serial_puts("Setup MMU and enable I/D-cache...\n");
	mmu_setup();
	icache_enable();
	dcache_enable();

	return 0;
}

/*
int cmd_exe(int argc, char **argv)
{
    unsigned char log_enable;

    if (argc >= 1) {
        switch(tolower(argv[0][0])) {
            case 'h':
            {
                do_help();
                break;
            }
            case 'v':
            {
                do_show_version();
                break;
            }
            case 'g':
            {
                char *addr = (char *) iros_strtol(argv[1]);
                serial_puts("Jump to ");
                serial_put_hex((unsigned int) addr);
                ((void (*)(void))addr)();
                break;
            }
            case 'w':
            {
                 do_set_memory(argc,argv);
                break;
            }
            case 'd':
            {
                do_dump_memory(argc, argv);
                break;
            }
            case 'u':
            {
                do_goto_uboot();
                break;
            }

            case 's':
            {
                if (argc > 1) {
                    log_enable = (iros_strtol(argv[1])) ? 1 : 0;
                } else {
                    log_enable = 0;
                }
                cs75xx_sdram_init(log_enable);
                break;
            }

#ifdef CONFIG_CORTINA_DIAG
            case 't':
            {
            	do_mem_test(argc, argv);
                break;
            }
#endif
            case 'c':
            {
            	do_cache_process(argc, argv);
                break;
            }

            case 'm':
            {
                serial_puts("Setup MMU and enable I/D-cache...\n");
				mmu_setup();
				icache_enable();
				dcache_enable();
                break;
            }

            case 'r':
            {
                reset_cpu(argc, argv);
                break;
            }
            default:
                break;
        }
    }
    return 0;
}
*/

int init_cmd_list()
{
	int i=0;

	command_list[i].cmd ="h";
	command_list[i].func_exe = (void *)&do_help;
	command_list[i++].help_msg ="    h               - Show help message\n";

	command_list[i].cmd ="w";
	command_list[i].func_exe = (void *)&do_set_memory;
	command_list[i++].help_msg ="    w  addr value   - write memory\n";

	command_list[i].cmd ="d";
	command_list[i].func_exe = (void *)&do_dump_memory;
	command_list[i++].help_msg ="    d  addr len     - dump memory\n";

	command_list[i].cmd ="u";
	command_list[i].func_exe = (void *)&do_goto_uboot;
	command_list[i++].help_msg ="    u               - continue to uboot\n";

/*
    	command_list[i].cmd ="s";
    	command_list[i].func_exe = (void *)&do_s;
    	command_list[i++].help_msg ="    s               - setup sdram\n";
*/

	command_list[i].cmd ="g";
	command_list[i].func_exe = (void *)&do_g;
	command_list[i++].help_msg ="    g  addr         - goto addr\n";

	command_list[i].cmd ="c";
	command_list[i].func_exe = (void *)&do_cache_process;
	command_list[i++].help_msg ="    c  i/d-cache mode  - enable/disable L1 I/D-Cache\n";

	command_list[i].cmd ="m";
	command_list[i].func_exe = (void *)&do_m;
	command_list[i++].help_msg ="    m               - turn on mmu, L1 I-Cache and D-Cache\n";


	command_list[i].cmd ="r";
	command_list[i].func_exe = (void *)&reset_cpu;
	command_list[i++].help_msg ="    r               - reset the board\n";

	command_list[i].cmd ="wfi";
	command_list[i].func_exe = (void *)&wfi_cpu;
	command_list[i++].help_msg ="    wfi             - Put primary core in WFI\n";

	command_list[i].cmd ="g2_start_init";
	command_list[i].func_exe = (void *)&g2_start_init;
	command_list[i++].help_msg ="    g2_start_init [DDR_freq]\n";

#ifdef CONFIG_CORTINA_DIAG
	command_list[i].cmd ="t";
	command_list[i].func_exe = (void *)&do_mem_test;
	command_list[i++].help_msg ="    t id addr len iter	- Memory test by memtest86\n";

	command_list[i].cmd ="g2_eye_loop";
	command_list[i].func_exe = (void *)&g2_eye_loop;
	command_list[i++].help_msg ="    g2_eye_loop 1:R 0:W  addr [value ...]\n";

	command_list[i].cmd ="g2_reg_wr";
	command_list[i].func_exe = (void *)&g2_reg_wr;
	command_list[i++].help_msg ="    g2_reg_wr addr value [next value]\n";

	command_list[i].cmd ="g2_ddr_reg_init";
	command_list[i].func_exe = (void *)&g2_ddr_reg_init;
	command_list[i++].help_msg ="    g2_ddr_reg_init [option...]\n";

	command_list[i].cmd ="g2_ddr_start";
	command_list[i].func_exe = (void *)&g2_ddr_start;
	command_list[i++].help_msg ="    g2_ddr_start\n";

	command_list[i].cmd ="g2_wr_ddr_pat";
	command_list[i].func_exe = (void *)&g2_wr_ddr_pat;
	command_list[i++].help_msg ="    g2_wr_ddr_pat               - Write predefined pattern to DRAM\n";

	command_list[i].cmd ="g2_tr_rd";
	command_list[i].func_exe = (void *)&g2_tr_rd;
	command_list[i++].help_msg ="    g2_tr_rd loop_cnt [A|a]\n";

	command_list[i].cmd ="g2_tr_lat";
	command_list[i].func_exe = (void *)&g2_tr_lat;
	command_list[i++].help_msg ="    g2_tr_lat loop_cnt\n";

	command_list[i].cmd ="g2_tr_gat";
	command_list[i].func_exe = (void *)&g2_tr_gat;
	command_list[i++].help_msg ="    g2_tr_gat loop_cnt [A|a]\n";

	command_list[i].cmd ="g2_tr_wr";
	command_list[i].func_exe = (void *)&g2_tr_wr;
	command_list[i++].help_msg ="    g2_tr_wr loop_cnt [A|a]\n";

	command_list[i].cmd ="g2_ddr_size";
	command_list[i].func_exe = (void *)&g2_ddr_size;
	command_list[i++].help_msg ="    g2_ddr_size		- Detect memory size\n";

	command_list[i].cmd ="g2_ddr_rst";
	command_list[i].func_exe = (void *)&g2_ddr_rst;
	command_list[i++].help_msg ="    g2_ddr_rst		- Reset Controller/PHY\n";

	command_list[i].cmd ="g2_long_test1";
	command_list[i].func_exe = (void *)&g2_long_test1;
	command_list[i++].help_msg ="    g2_long_test1\n";

	command_list[i].cmd ="g2_ddr_freq";
	command_list[i].func_exe = (void *)&g2_ddr_freq;
	command_list[i++].help_msg ="    g2_ddr_freq frequency[1000~236]\n";

	command_list[i].cmd ="g2_start_zq";
	command_list[i].func_exe = (void *)&g2_start_zq;
	command_list[i++].help_msg ="    g2_start_zq [loop] [interval(ms)]\n";

	command_list[i].cmd ="g2_force_zq";
	command_list[i].func_exe = (void *)&g2_force_zq;
	command_list[i++].help_msg ="    g2_force_zq impp impn\n";

	command_list[i].cmd ="g2_start_dll";
	command_list[i].func_exe = (void *)&g2_start_dll;
	command_list[i++].help_msg ="    g2_start_dll\n";

	command_list[i].cmd ="g2_dbg_on";
	command_list[i].func_exe = (void *)&g2_dbg_on;
	command_list[i++].help_msg ="    g2_dbg_on  [1/0:]debug message on/off\n";

	command_list[i].cmd ="g2_ddr_set_default_conf";
	command_list[i].func_exe = (void *)&g2_ddr_set_default_conf;
	command_list[i++].help_msg ="    g2_ddr_set_default_conf [DDR_freq]\n";

	command_list[i].cmd ="g2_ddr_start_and_train";
	command_list[i].func_exe = (void *)&g2_ddr_start_and_train;
	command_list[i++].help_msg ="    g2_ddr_start_and_train\n";

#ifdef CONFIG_CORTINA_DIAG
	/* Jason add for usb scan */
	command_list[i].cmd ="g2_usb_test";
	command_list[i].func_exe = (void *)&g2_usb_test;
	command_list[i++].help_msg ="    g2_usb_test \n";

	/* debug_Aaron on 2011/06/29 for snowbush PHY bring up */
	command_list[i].cmd ="g2_pcie_sbphy_init";
        command_list[i].func_exe = (void *)&g2_pcie_sbphy_init;
        command_list[i++].help_msg ="    g2_pcie_sbphy_init  [0/1/2] {i|e}\n";
#endif

#ifdef __AHCI_DIAGNOSTIC__
	command_list[i].cmd ="g2_sata_init";
        command_list[i].func_exe = (void *)&g2_sata_initial;
        command_list[i++].help_msg ="    g2_sata_init [0-3:] port\n";
#endif

#endif

	serial_puts("Total ");
	serial_put_decimal(i);
	serial_puts(" command provided\n");
	return i;
}

void post_cli(void) {

    int res = 0;
    char *command;
    char line[128];
    int cli_argc;
    char *cli_argv[MAX_ARGV];

    do_show_version();

    total_command = init_cmd_list();
    do_help();

    serial_puts("POST> ");

    while (1) {
        memset(line, 0, sizeof(line));
        memset(cli_argv, 0, sizeof(char *) * MAX_ARGV);
        extern int _rb_gets(char *buf, int buflen, int timeout);

        if (_GETS_TIMEOUT != _rb_gets(line, sizeof(line), 1000000 /* milli seconds */)) {

            command = (char *)&line;
            if(post_strlen(command) == 0)
                serial_puts("POST> ");

            while (post_strlen(command) > 0) {

                serial_putc('\n');
                serial_puts("do command:");
                serial_puts(line);
                serial_putc('\n');

                parse_prep(&command, &cli_argc, &cli_argv[0]);
//                res = cmd_exe(cli_argc, cli_argv);
                res = cmd_exe_str(cli_argc, cli_argv);
                serial_putc('\n');
                serial_puts("POST> ");
            }

        }

    }

    // never come here
    return;
}

int post_cmd_in_function(char *command)
{
	int res = 0;
	int cli_argc;
    char *cli_argv[MAX_ARGV];

	memset(cli_argv, 0, sizeof(char *) * MAX_ARGV);

	if(post_strlen(command) == 0)
		return 0;

	while (post_strlen(command) > 0) {
                serial_puts("do command:");
                serial_puts(command);
                serial_putc('\n');

                parse_prep(&command, &cli_argc, &cli_argv[0]);
                res = cmd_exe_str(cli_argc, cli_argv);
                serial_putc('\n');
	}

    return res;
}

