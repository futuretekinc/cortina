
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "g2autoconf.h"

//#define CONFIG_CORTINA_FPGA		1
//#define CONFIG_CORTINA_DIAG		1
//#define CONFIG_CORTINA_NBOOT		1
#define CONFIG_SYS_NO_ICACHE		1
#define CONFIG_SYS_NO_DCACHE		1

#ifdef CONFIG_CORTINA_FPGA
#define	DDR_BASE_ADDR			0xE0000000	/* DDR base address */
#define	FLASH_BASE_ADDR			0xE8000000	/* Flash base address */
#define CORTEX_SCU_BASE			0x1E000000	/* SCU Base */
#define CORTEX_GIC_CPU_BASE		0x1E000100	/* Generic interrupt controller CPU interface */
#define CORTEX_TWD_BASE			0x1E000600	/* CPU Timer & watch dog */
#define CORTEX_GIC_DIST_BASE		0x1E001000	/* Generic interrupt controller distributor */
#else
#define	DDR_BASE_ADDR			0x00000000	/* DDR base address */
#define	FLASH_BASE_ADDR			0xE0000000	/* Flash base address */
#define CORTEX_SCU_BASE			0xF8000000	/* SCU Base */
#define CORTEX_GIC_CPU_BASE		0xF8000100	/* Generic interrupt controller CPU interface */
#define CORTEX_TWD_BASE			0xF8000600	/* CPU Timer & watch dog */
#define CORTEX_GIC_DIST_BASE		0xF8001000	/* Generic interrupt controller distributor */
#endif

#define RRAM0_BASE		0xF6200000	/* RCPU_DRAM0 */
#define RCPU_DRAM0_BASE		0xF6000000
#define RCPU_IRAM0_BASE		0xF6008000
#define RCPU_DRAM1_BASE		0xF6020000
#define RCPU_IRAM1_BASE		0xF6028000
#define RCPU_RRAM0_BASE		0xF6200000
#define RCPU_RRAM1_BASE		0xF6400000


#define	BOOT_FROM_FLASH		1

#ifdef CONFIG_CORTINA_FPGA
#define CONFIG_SYS_HZ			(50*1000000)
#else
#define CONFIG_SYS_HZ			(150*1000000)
#endif

/* global register 2 for S/W use */
#define	GLOBAL_SOFTWARE1	0xf00000b8
#define	GLOBAL_SOFTWARE2	0xf00000bc

/* GLOBAL_STRAP register bit 0 for secure mode indication */
#define GLOBAL_STRAP		0xf000000c

/* packet buffer base address (256KB) */
#define	PACKET_BUFFER_BASE	0xf6a00000

/* Physical DRAM Memory Map */
#define CONFIG_NR_DRAM_BANKS	1				/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1			DDR_BASE_ADDR	/* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE		0x08000000		/* 128 MB */

/* GPIO */
#define	PER_GPIO1_CFG		0xf00701f0
#define	PER_GPIO1_OUT		0xf00701f4

/* these three must match uboot */
#ifdef G2_LOADER_ADDR
  #define UBOOT1_BASE     		(FLASH_BASE_ADDR + G2_LOADER_ADDR)
  #define UBOOT2_BASE     		(FLASH_BASE_ADDR + G2_LOADER2_ADDR)
#else
  #define UBOOT1_BASE     		FLASH_BASE_ADDR
  #define UBOOT2_BASE     		(FLASH_BASE_ADDR + 0x00060000)
#endif
//
//
//#define CFG_ENV_SECT_SIZE  		G2_UBOOT_ENV_SIZE
//#define ENV_HEADER_SIZE    		sizeof(int)
//#define CFG_ENV_ADDR       		(UBOOT1_BASE + (CFG_ENV_SECT_SIZE * 2))
//#define CFG_ENV_ADDR_BK    		(UBOOT2_BASE + (CFG_ENV_SECT_SIZE * 2))
//#define MAX_ENV_CNT        		2

#define CONFIG_STACKSIZE  	(128 * 1024)
//#define CONFIG_SKIP_RELOCATE_UBOOT
#define TEXT_BASE 			(DDR_BASE_ADDR + 0x4000000)

#define PARALLEL_FLASH   	0x0
#define INTERNAL_ROM     	0x1
#define SERIAL_FLASH     	0x2

/* First 64KB is for 1st-Stage Bootloader offset 0x00000000 */
/* Image Table is of size (32bytes * 10) offset  0x00010000 */

#define IMAGE_TABLE_BASE1  	0x30008000 /* 32 KB */
#define IMAGE_TABLE_BASE2  	0x30010000 /* 32 KB */

#define ACTIVE  			0x1
#define COMMIT  			0x2
#define CORRUPT 			0x3

#define BOOT_THRES  		0x3
#define MAX_TBL_CNT 		0x5

#define IMAGE1          	"uboot1"
#define IMAGE1_STR_LEN  	6
#define IMAGE1_SIZE     	"uboot1_size"
#define IMAGE1_CHKSM    	"uboot1_chksm"
#define IMAGE1_STS      	"uboot1_sts"

#define IMAGE2          	"uboot2"
#define IMAGE2_STR_LEN  	6
#define IMAGE2_SIZE     	"uboot2_size"
#define IMAGE2_CHKSM    	"uboot2_chksm"
#define IMAGE2_STS      	"uboot2_sts"

#define PER_IRQ_ENABLE    	0x2c300004
#define GLOBAL_PIN_STS    	0x2c30c020

/* WDT registers */
#define PER_WDT_CTRL      	0x2c300014
#define PER_WDT_PRE_DIV   	0x2c300018

#define PER_WDT_LD        	0x2c300020
#define PER_WDT_LOADE     	0x2c300024
#define PER_WDT_CNT       	0x2c300028
#define PER_WDT_IE        	0x2c30002c
#define PER_WDT_INT       	0x2c300030

#define WDT_CLK_SEL_PRE_SCA (1<<2)
#define WDT_RST_EN      	(1<<1)
#define GLB_WDT_EN      	1
#define WDT_EN          	1
#define RST_EN          	(1<<1)

#endif /* __CONFIG_H__ */
