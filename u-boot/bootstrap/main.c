/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2009 by Cortina Systems Incorporated.                 */
/***********************************************************************/

/* test code here */
/* This is not suitable for production. Only for reference. */
#include "serial_cs752x.h"
#include "config.h"
#include "utils.h"
#include "common.h"
#include "md5.h"
#include "sdram.h"

#define DEBUG 0

#if DEBUG
typedef struct {
   char index;
   char image[32];
   unsigned int image_offset;
   unsigned int size;
   unsigned char chksum[16];
   char boot_flag;
   char wd_cnt;
   char res[5];
} __attribute__ ((packed)) img_tbl_t;

volatile unsigned char *g_env_dat;
volatile unsigned char *g_img_tbl1;
volatile unsigned char *g_img_tbl2;
#endif

static unsigned int	gs2 = 0;

extern unsigned int uboot_entry;
extern unsigned int uboot_image_len;

//#ifdef BOOT_FROM_FLASH
extern char     _exception_handlers[];
extern char     _exception_handlers_end[];
//#endif

void do_show_version(void)
{
	serial_puts("\nCortina Systems Power-On Self-Test 0.99\n");
	serial_puts(" Build Info: ");
	serial_puts(__DATE__);
	serial_putc(' ');
	serial_puts(__TIME__);
	serial_puts("\n");
	return;
}

#if DEBUG
// check if it is "name=val" matching name
char *search_val(char *name, char *p)
{
	int i;
	i = 0;

	while (0 != p[i]) {
		if (name[i] == p[i]) {
			/* matching one char */
			if (0 == name[i+1]){
				if ('=' == p[i+1]) {
					/* found */
					return &p[i+2];
				} else {
					return 0;
				}
			}
			i++;
		} else {
			return 0;
		}
	}
	return 0;
}


char *move_to_next_param(char *p)
{
	if (0 == p[0])
		return 0;

	while (0 != p[1]) {
		p++;
		if (p >= (char *)(g_env_dat + CFG_ENV_SECT_SIZE))
	    		return 0;
	}
	return &p[2];
}

char *getenv (char *name)
{
	char *p;
	char *p_val;

	p = (char *) g_env_dat; /* ? */
	p += 4;
	while (1) {
		p_val = search_val(name, p);
		if (0 != p_val) {
	    		return p_val;
		}
		p = move_to_next_param(p);
		if (0 == p) {
	    		return 0;
		}
	}
}

int env_var_info_get(char *var, unsigned int *val)
{
	char *p;
	int ret = 0;

	p = getenv(var);
	if ( 0 == p) {
		*val = 0;
		ret = -1;
	} else {
		*val = iros_strtol(p);
	}

	return ret;
}

int cmp_img(char *s1, char *s2)
{
	if (memcmp(s1, "u-boot", 6) == 0) {
		if ((memcmp(s2, "u-boot0", 7)==0) ||
  			(memcmp(s2, "u-boot1", 7)==0)) {
 			return 0;
		}
	}

	return 1;
}

int Verify_tbl_chksm(img_tbl_t *tbl)
{
	unsigned char chksum[16];
	unsigned size = MAX_TBL_CNT * sizeof(img_tbl_t);

	serial_puts("Table address ");
	serial_put_hex_no_0x((unsigned int)tbl);
	serial_puts(" Size ");
	serial_put_hex_no_0x(size);
	serial_puts("\n\n");

	/* Calculate the checksum of table */
	md5 ((unsigned char *)tbl, size, chksum);

	if (memcmp(chksum, ((char *)tbl) + size, 16) == 0) {
		return 1;
	}

	return 0;
}

int verify_image_chksm(char idx, img_tbl_t *tbl)
{
	unsigned char chksum[16];

	/* Validate index */
	if ((idx == 0) || (idx > MAX_TBL_CNT)) {
		return 1;
	}

	/* Move reference to particular index */
	img_tbl_t *a=(img_tbl_t *)tbl + (idx -1);

	/* Calculate the checksum of particular image */
	md5 ((unsigned char *)a->image_offset, a->size, &chksum[0]);

	/* Compare the calculated checksum and checksum on
	* table, if successful return 1, on failure return 0 */
	if (memcmp(&chksum[0], &a->chksum[0], 16) != 0) {
		return 0;
	}

	return 1;
}
#endif

int do_goto_uboot(void)
{
	unsigned int addr;
	unsigned int secure;
	unsigned int u_boot_flash_addr;

	/* read and write GLOBAL_SOFTWARE2 register. */
	/* if GS2 bit0==0, write 1 else write 0. */
   	REG_READ_UINT32(GLOBAL_SOFTWARE2, gs2);
#if DEBUG
	serial_puts("main(): gs2 = ");
	serial_put_hex(gs2);
	serial_puts("\n");
#endif
	if ((gs2 & 0x01)==1) {
		gs2 = gs2 | 0x80;	/* save active u-boot1 to bit7 */
		gs2 = gs2 & 0xfffffffe;
	}
	else {
		gs2 = gs2 & 0xffffff7f;	/* save active u-boot0 to bit7 */
		gs2 = gs2 | 0x01;
	}
#if DEBUG
	serial_puts("main(): gs2 = ");
	serial_put_hex(gs2);
	serial_puts("\n");
#endif
   	REG_WRITE_UINT32(GLOBAL_SOFTWARE2, gs2);

	/* check if uboot is running in non-secure mode. */
	/* if yes, copy u-boot from flash to DRAM memory. */
	REG_READ_UINT32(GLOBAL_STRAP, secure);
	if ((secure & 0x00000001) == 0) {
   		REG_READ_UINT32(GLOBAL_SOFTWARE2, gs2);
#if DEBUG
		serial_puts("gs2 = ");
		serial_put_hex(gs2);
		serial_puts("\n");
#endif
		if ((gs2 & 0x80) == 0) { /* check active u-boot */
#if DEBUG
			serial_puts("U-boot 0...\n ");
#endif
			u_boot_flash_addr = UBOOT1_BASE + uboot_entry;
		}
		else {
#if DEBUG
			serial_puts("U-boot 1...\n ");
#endif
			u_boot_flash_addr = UBOOT2_BASE + uboot_entry;
		}
	} else {
		u_boot_flash_addr = PACKET_BUFFER_BASE + uboot_entry;
	}

	addr = (unsigned int)TEXT_BASE;
	serial_puts("Copy flash code from ");
	serial_put_hex(u_boot_flash_addr);
	serial_puts(" to ");
	serial_put_hex(addr);
	serial_puts(" length = ");
	serial_put_hex(uboot_image_len);
	serial_puts("\n\n");
	memcpy((void *)addr, (void *)u_boot_flash_addr, (unsigned int)uboot_image_len);

	serial_puts("POST: execution transfer to u-boot ");
	serial_put_hex(addr);
	serial_puts("\n\n");
	((void (*)(void))addr)();

	/* Incase of checksum failure return 0 */
	return 0;
}

int cs75xx_sdram_init(int mem_type)
{
	/* TODO: Depending on the mem requirment initialization has to be done */
	denali_sdramdriver_initialize();

	return 1;
}

int cs75xx_mem_test(int mem_type)
{
	char *av[3];
	int  ac = 3;

	av[0] = "tc";
#ifdef CONFIG_CORTINA_FPGA
	av[1] = "0xE0000000";
	av[2] = "0x00010000";
#else
	av[1] = "0x00800000";
	av[2] = "0x00400000";
#endif

   	/* TODO: Memory test has to be done */
	do_high_load_test_new(av, ac);

   	return 1;
}


//#ifdef BOOT_FROM_FLASH
int wake_up_2nd_cpu(unsigned char sgi)
{
	unsigned char *srce ;
	unsigned char *targ ;
	int i,size;
	unsigned int tmp;

//#ifdef BOOT_FROM_FLASH

	size = (unsigned int)(&_exception_handlers_end[0] - &_exception_handlers[0]) + 0x20;
	srce = (unsigned char *)(&_exception_handlers[0]);
	targ = (unsigned char *)RCPU_RRAM1_BASE;
	while(size--) {
		*targ = *srce;
		targ++; srce++;
	}
//#endif

	IO_WRITE(GLOBAL_SOFTWARE1, RCPU_RRAM1_BASE);

	/* Flush to HW register */
	IO_READ(GLOBAL_SOFTWARE1);

	/* Config GIC */
	/* 1. Set priority of interrupt for IPI
	 * 	 Only 0~16 be configured.
	 */
//	for(i=0;i<16;i+=4)
//		IO_WRITE(CORTEX_GIC_DIST_BASE + 0x400 + i*4/4 ,0xa0a0a0a0);

	/* Disable all interrupt */
//	for(i=0;i<64;i+=32)
//		IO_WRITE(CORTEX_GIC_DIST_BASE + 0x180 + i*4/32 ,0xFFFFFFFF);

	/* Enable Distribution Control */
	IO_WRITE(CORTEX_GIC_DIST_BASE ,0x1);

	/* Primask  */
	IO_WRITE(CORTEX_GIC_CPU_BASE + 0x04 ,0xf0);
	/* Enable Processor interface */
	IO_WRITE(CORTEX_GIC_CPU_BASE ,0x1);

	/* Send Soft INT0 to CPU1 */
	IO_WRITE(CORTEX_GIC_DIST_BASE + 0xf00 , 0x00020000|sgi);

	for(i=0;i<1000000;i++){
		tmp = IO_READ(GLOBAL_SOFTWARE1);
		if(tmp==0)
			break;
	}

	if(i<1000000){
		serial_puts("2nd CPU Switch to RRAM1 successfully\n");
	}
	else
		serial_puts("2nd CPU wake up fail\n");

	return 0;
}
//#endif

int main (void)
{
   	char line[128];
   	unsigned int hit_key;
   	unsigned int time_stamp;

	time_stamp = IO_READ(0xf0000000);
	if(time_stamp&0x00008000){	/* A1 chip */
		/* External Reset */
		time_stamp = IO_READ(0xf00000c0);
		time_stamp |= 0x400;
		IO_WRITE(0xf00000c0 , time_stamp);
		time_stamp = IO_READ(0xf00000c0);

		/* Switch ref_clk to 25MHz */
		time_stamp = IO_READ(0xf0000010);
		IO_WRITE(0xf0000010 , time_stamp | 0x00040000);

		/* Release external reset */
		time_stamp = IO_READ(0xf00000c0);
		time_stamp &= ~0x400;
		IO_WRITE(0xf00000c0 , time_stamp);
		time_stamp = IO_READ(0xf00000c0);
	}

	timer_init();

   	serial_init();

//#ifdef BOOT_FROM_FLASH
	wake_up_2nd_cpu(1);
//#endif

   	do_show_version();

	hit_key = 0;

	serial_puts("Hit key [m] before reset/power_on to enter DDR memory config and POST prompt\n");

	time_stamp = get_timer(0);

	memset(line, 0, sizeof(line));
	serial_getc_with_timeout(line);
	memset(line, 0, sizeof(line));
	serial_getc_with_timeout(line);
	if (('m' == line[0]) || ('M' == line[0]))
		hit_key = 1;

#if 0	// Reduce boot time
	while (1) {
		memset(line, 0, sizeof(line));
		if (1 == serial_getc_with_timeout(line)) {
			// timed out
		} else if ('m' == line[0]) {
			hit_key = 1;
			break;
		}
		if( get_timer(time_stamp) > 3000 )
			break;
	}
#endif

   	if ((0 != hit_key)) {
   		post_cli();
   	} else {
		/* Initialize SDRAM and do memory testing, if testing passes transfer
		* control to Stage 2 boot loader */
		serial_puts("Setup DDR, do memory check, and then go to UBOOT\n");
	  	if (cs75xx_sdram_init(0) && cs75xx_mem_test(0))
			do_goto_uboot();
   	}

   	return 0;
}
