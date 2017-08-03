/*
 * Copyright (c) 2011 by Cortina Systems Incorporated.
 *
 * Configuration settings for the GoldenGate Board.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

// g2autoconf.h is created automatically during the openwrt
// build process.  It will contain #defines for every enabled
// CONFIG_UBOOT_* option in the openwrt .config file.
//
#define G2_UBOOT_ENV_SIZE 	131072
#define G2_UBOOT_ENV_OFFSET 1048576
#define G2_UBOOT_ENV_RANGE 	1048576

#define CONFIG_CS75XX_NONE_TO_EXT_SWITCH 	1
#define CONFIG_CS75XX_PHY_ADDR_GMAC0 		1
#define CONFIG_UBOOT_CPU_TO_GMAC	 		0
#define CONFIG_MTD_CORTINA_CS752X_NAND 		1
#define CONFIG_CS752X_NAND_ECC_HW_HAMMING_256 1

/*
 * Board info register
 */
#define SYS_ID  (0x10000000)
#define ARM_SUPPLIED_REVISION_REGISTER SYS_ID

#define CONFIG_SYSTEM_CLOCK		(100*1000000)
#define CONFIG_IDENT_STRING		" FutureTek FTM-NICO2"

/*
 * High Level Configuration Options
 */
#define CONFIG_GOLDENGATE		1
#define CONFIG_FM_A9_SMP		1

#define CONFIG_SHOW_BOOT_PROGRESS	1

/* Timer Configuration */
#define CONFIG_SYS_HZ			(1000)

/* GLOBAL_STRAP register bit 0 for secure mode indication */
#define GLOBAL_STRAP			0xf000000c

/* global register 2 for S/W use */
#define	GLOBAL_SOFTWARE2		0xf00000bc

#define CONFIG_CMDLINE_TAG		1 /* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_MISC_INIT_R		1 /* call misc_init_r during start up */

#define CONFIG_MD5

/*
 * Number of bytes reserved for initial data
 */
#define CONFIG_SYS_GBL_DATA_SIZE	128

/*
 * Hardware drivers
 */
#define CONFIG_NET_MULTI

#define CONFIG_GOLDENGATE_SPI		1

/*
 * G2 Serial Configuration
 */
#define CONFIG_GOLDENGATE_SERIAL

#define CONFIG_CONS_INDEX		0

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*
 * Command line configuration.
 */
#define CONFIG_CMD_BDI
#define CONFIG_CMD_ENV
#if defined(CONFIG_MTD_CORTINA_CS752X_NAND) || defined(CONFIG_MTD_CORTINA_CS752X_SERIAL)
#else
#define CONFIG_CMD_FLASH
#endif

#define CONFIG_CMD_IMI
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_LOADB
#define CONFIG_CMD_HTTP

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME
#define CONFIG_BOOTP_SUBNETMASK

#define CONFIG_BOOTDELAY		2

//#define CONFIG_BOOTARGS "root=/dev/mtdblock0 mtdparts=armflash.0:7268k@0x02680000(cramfs) ip=dhcp mem=128M console=ttyAMA0"
//#define CONFIG_BOOTCOMMAND "cp.l 0xe8100000 0xe1600000 0x200000; go 0xe1600000"
#define CONFIG_BOOTCOMMAND "run process setbootargs b"

/*
 * Static configuration when assigning fixed address
 */
#define CONFIG_ETHADDR			00.40.5c.12.34.56	/--* talk on MY local MAC address */
#define CONFIG_NETMASK			255.255.255.0		/--* talk on MY local net */
#define CONFIG_IPADDR			192.168.1.222		/--* static IP I currently own */
#define CONFIG_SERVERIP	    	192.168.1.1			/--* current IP of my dev pc */
#define CONFIG_GATEWAYIP		192.168.1.1			/--* current gateway IP */
//#define CONFIG_BOOTFILE			"openwrt-g2-fpga-zImage" /* file to load */

/* Allow overwrite of ethaddr
 */
#define CONFIG_ENV_OVERWRITE

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP			/* undef to save memory */
#define CONFIG_SYS_CBSIZE		1024		/* Console I/O Buffer Size */

/* Monitor Command Prompt	 */
#define CONFIG_SYS_PROMPT		"FTM-NICO2 # "

/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE	\
			(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		32		/* max number of command args */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE /* Boot Argument Buffer Size */

#define CONFIG_SYS_LOAD_ADDR	0x01600000	/* default load address */

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE			(256 * 1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
	#define CONFIG_STACKSIZE_IRQ		(4 * 1024)	/* IRQ stack */
	#define CONFIG_STACKSIZE_FIQ		(4 * 1024)	/* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS		1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1				0x00000000	/* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE			0x10000000	/* 256 MB */
#define CONFIG_SYS_MEMTEST_START	0x00100000
#define CONFIG_SYS_MEMTEST_END		0xf0000000

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */
/*
 * Note that CONFIG_SYS_MAX_FLASH_SECT allows for a parameter block
 * i.e. the bottom "sector" (bottom boot), or top "sector"
 *      (top boot), is a seperate erase region divided into
 *      4 (equal) smaller sectors. This, notionally, allows
 *      quicker erase/rewrire of the most frequently changed
 *      area......
 *      CONFIG_SYS_MAX_FLASH_SECT is padded up to a multiple of 4
 */
#define CONFIG_SYS_FLASH_BASE	0xe0000000
#define PHYS_FLASH_SIZE			0x08000000	/* 128MB */

#define PHYS_FLASH_1			(CONFIG_SYS_FLASH_BASE)

#define CONFIG_SYS_MAX_FLASH_BANKS	1		/* max number of memory banks */
#define CONFIG_SYS_MAX_FLASH_SECT	(512 * 2)


/*
 * Environment settings
 */
#if defined(CONFIG_CMD_FLASH) || defined(CONFIG_MTD_CORTINA_CS752X_SERIAL)
	#undef 	CONFIG_MTD_CORTINA_CS752X_NAND
#endif

#if defined(CONFIG_MTD_CORTINA_CS752X_NAND)
	#define CONFIG_NAND_CS75XX		1
	#define	CONFIG_NAND_G2			1
	#define	CONFIG_CMD_NAND			1
	#define CONFIG_CS75XX_NAND_REGS_BASE	0xf0050000
	#define CONFIG_SYS_MAX_NAND_DEVICE	1
	#define CONFIG_SYS_NAND_MAX_CHIPS	2
	#define CONFIG_SYS_NAND_BASE		0xE0000000
	#define CONFIG_SYS_NAND_BASE_LIST	{ CONFIG_SYS_NAND_BASE }
	/* #define CONFIG_JFFS2_NAND */
	#define CONFIG_MTD_NAND_VERIFY_WRITE    1
	#define CONFIG_CS75XX_NAND_HWECC
	/* The following parameters defined in g2autocon.h : */
	/*#define	CONFIG_CS75XX_NAND_HWECC_HAMMING_256 */
	/*#define	CONFIG_CS75XX_NAND_HWECC_HAMMING_512 */
	/*#define	CONFIG_CS75XX_NAND_HWECC_BCH_8       */
	/*#define	CONFIG_CS75XX_NAND_HWECC_BCH_12      */

	#define CONFIG_SYS_NO_FLASH
	#undef CONFIG_ENV_IS_IN_FLASH

	#define CONFIG_ENV_IS_IN_NAND
	#define PHYS_FLASH_1			(CONFIG_SYS_FLASH_BASE)

	#ifdef G2_UBOOT_ENV_OFFSET
		#define CONFIG_ENV_OFFSET	(G2_UBOOT_ENV_OFFSET)
	#else
		#define CONFIG_ENV_OFFSET	(0x100000)
	#endif

	#ifdef G2_UBOOT_ENV_SIZE
		#define CONFIG_ENV_SIZE		(G2_UBOOT_ENV_SIZE)
	#else
		#define CONFIG_ENV_SIZE		(0x20000)
	#endif

	#ifdef G2_UBOOT_ENV_RANGE
		#define CONFIG_ENV_RANGE	(G2_UBOOT_ENV_RANGE)
	#else
		#define CONFIG_ENV_RANGE	(0x100000)
	#endif

	#define CONFIG_ENV_SECT_SIZE		(CONFIG_ENV_SIZE)
	#define CONFIG_ENV_ADDR 		(PHYS_FLASH_1 + CONFIG_ENV_OFFSET)
	#define CONFIG_SYS_MONITOR_BASE		(PHYS_FLASH_1)
	#define CONFIG_SYS_MONITOR_LEN		(0x100000)	/* Reserve 1MB */
#endif

#if defined(CONFIG_MTD_CORTINA_CS752X_SERIAL)
	#define CONFIG_CMD_SF
	#define CONFIG_SYS_NO_FLASH
	#undef CONFIG_ENV_IS_IN_FLASH
	#define CONFIG_ENV_IS_IN_SPI_FLASH
	#define PHYS_FLASH_1			(CONFIG_SYS_FLASH_BASE)

	#define CONFIG_ENV_SPI_MAX_HZ 		1000000
	#define CONFIG_ENV_SPI_MODE 		SPI_MODE_3
	#define CONFIG_ENV_SPI_BUS 		0
	#define CONFIG_ENV_SPI_CS 		1

	#ifdef G2_UBOOT_ENV_OFFSET
		#define CONFIG_ENV_OFFSET	(G2_UBOOT_ENV_OFFSET)
	#else
		#define CONFIG_ENV_OFFSET	(0x40000)
	#endif

	#ifdef G2_UBOOT_ENV_SIZE
		#define CONFIG_ENV_SIZE		(G2_UBOOT_ENV_SIZE)
	#else
		#define CONFIG_ENV_SIZE		(0x40000)
	#endif

	#ifdef G2_UBOOT_ENV_RANGE
		#define CONFIG_ENV_RANGE	(G2_UBOOT_ENV_RANGE)
	#else
		#define CONFIG_ENV_RANGE	(0x40000)
	#endif

	#define CONFIG_ENV_SECT_SIZE		(CONFIG_ENV_SIZE)
	#define CONFIG_ENV_ADDR 		(PHYS_FLASH_1 + CONFIG_ENV_OFFSET)
	#define CONFIG_SYS_MONITOR_BASE		(PHYS_FLASH_1)
	#define CONFIG_SYS_MONITOR_LEN		(0x100000)	/* Reserve 1MB */
#endif

#ifdef CONFIG_CMD_FLASH
	#undef CONFIG_SYS_NO_FLASH
	#define CONFIG_ENV_IS_IN_FLASH		1
	#define CONFIG_ENV_SIZE			(0x20000)
	#define CONFIG_ENV_SECT_SIZE		(CONFIG_ENV_SIZE)
	#define CONFIG_ENV_ADDR 		(PHYS_FLASH_1 + 0x00040000)
	#define CONFIG_SYS_MONITOR_BASE		(PHYS_FLASH_1)
	#define CONFIG_SYS_MONITOR_LEN		(256 * 1024)	/* Reserve 256KiB */

	/* Timeout values are in ticks */
	/* Lengthened for bad boards */
	#define CONFIG_SYS_FLASH_ERASE_TOUT	(20 * CONFIG_SYS_HZ) /* Erase Timeout */
	#define CONFIG_SYS_FLASH_WRITE_TOUT	(20 * CONFIG_SYS_HZ) /* Write Timeout */
	#define CONFIG_SYS_FLASH_PROTECTION	/* The devices have real protection */
	#define CONFIG_SYS_FLASH_EMPTY_INFO	/* flinfo indicates empty blocks */

	/*
	 * Use the CFI flash driver for ease of use
	 */
	#define CONFIG_SYS_FLASH_CFI
	#define CONFIG_SYS_FLASH_USE_BUFFER_WRITE 	1	/* Use buffered writes (~10x faster) */
	#define CONFIG_FLASH_CFI_DRIVER

	//#define CONFIG_SYS_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
	#define CONFIG_FLASH_PART_BOOTLOADER0_LOC      (CONFIG_SYS_FLASH_BASE + 0x000000)
	#define CONFIG_FLASH_PART_BOOTENV0_LOC         (CONFIG_SYS_FLASH_BASE + 0x040000)
	#define CONFIG_FLASH_PART_BOOTLOADER1_LOC      (CONFIG_SYS_FLASH_BASE + 0x060000)
	#define CONFIG_FLASH_PART_BOOTENV1_LOC         (CONFIG_SYS_FLASH_BASE + 0x0a0000)
	#define CONFIG_FLASH_PART_SUPER0_LOC           (CONFIG_SYS_FLASH_BASE + 0x0c0000)
	#define CONFIG_FLASH_PART_SUPER1_LOC           (CONFIG_SYS_FLASH_BASE + 0x0e0000)
	#define CONFIG_FLASH_PART_KERNEL0_LOC          (CONFIG_SYS_FLASH_BASE + 0x100000)
	#define CONFIG_FLASH_PART_KERNEL1_LOC          (CONFIG_SYS_FLASH_BASE + 0xb00000)
#endif

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 128 * 1024)

/*
 * I2C Settings
 */
#if 0
	#define CONFIG_SYS_I2C_PORT         	I2C2_BASE_ADDR
	#define CONFIG_HARD_I2C			1 /* To enable I2C support	*/
	#undef	CONFIG_SOFT_I2C			  /* I2C bit-banged		*/
	#define CONFIG_I2C_CMD_TREE    		1
	#define CONFIG_CMD_I2C         		1
	#define CONFIG_SYS_I2C_SPEED		100000//400000		/* I2C speed and slave address	*/
	#define CONFIG_SYS_I2C_SLAVE		0x10
#endif

// If you intend to support "initramfs" style root file system (where
// the rootfs is compiled into the kernel image), then you must define:
#define CONFIG_INITRD_TAG   		1

// Run is needed for multi-image support.  We'll use the run
// command at the u-boot prompt as part of the multi-image
// boot process
#define CONFIG_CMD_RUN


#if defined(CONFIG_MTD_CORTINA_CS752X_NAND) || defined(CONFIG_MTD_CORTINA_CS752X_SERIAL)
#else
	#define CONFIG_CMD_JFFS2
	#define CONFIG_SYS_JFFS2_FIRST_BANK 0
	#define CONFIG_SYS_JFFS2_NUM_BANKS  1
	// #define CONFIG_SYS_JFFS2_SORT_FRAGMENTS
	#define CONFIG_SYS_JFFS_CUSTOM_PART

/*====================================================================*/
/* These two are the critical parameters telling                      */
/* where the jffs2 partition is carved out.  u-boot must              */
/* know where the JFFS2 file system starts and how big it is.         */
/*                                                                    */
/* THESE VALUES MUST MATCH THE VALUES IN                              */
/*   openwrt-2.4.2011-trunk/target/linux/g2/image/flash-config.yml    */
/* for                                                                */
/*   jffs_sector:          136                                        */
/*   jffs_size:            40M                                        */
/*====================================================================*/

	#define CONFIG_JFFS2_PART_OFFSET 	(CONFIG_ENV_SECT_SIZE * 40)
	#define CONFIG_JFFS2_PART_SIZE   	(CONFIG_ENV_SECT_SIZE * 320)
#endif /* CONFIG_MTD_CORTINA_CS752X_NAND */

#define	CONFIG_POWER_HOLD

#endif	/* __CONFIG_H */

