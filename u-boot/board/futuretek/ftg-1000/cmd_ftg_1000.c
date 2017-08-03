/*
 * (C) Copyright 2001-2010
 * Cortina Systems Inc
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

/*
 * Three commands are added:
 *
 * sb_display <sb-addr>
 *	Display the contents of a superblock at <sb-addr>
 *
 * sb_set_flags <sb-addr> <valid> <commit> <active>
 * 	Set the valid, commit and active flags on the
 *	superblock at <sb-addr>
 *
 * sb_process <kernel-addr> <rootfs-addr> <sb0-addr> <sb1-addr> [...]
 *	Invoke superblock processing.  A suitable superblock is
 *	searched for, starting at the end of the list and working
 *	backwards towards the first one specified (sb0).  A suitable
 *	superblock is one that has v=1, c=1, a=1 and can
 *	successfully copy out its kernel to <kernel-addr> and verify
 *	its md5 checksum; and if the superblock contains a rootfs,
 *	can copy that out to <rootfs-addr> and verify its md5 checksum.
 *
 *	<rootfs-addr> can be specified as '-' on the command line,
 *	in which case, the kernel is assumed to contain its own
 *	initramfs.
 *
 *	If the kernel and rootfs are in mtdparts and not in JFFS2,
 *  	then <kernel-addr> and/or <rootfs-addr> can be either '-' or
 *	an address.  If '-', then the bootm addresses will be flash
 * 	addresses.  If not '-', then <kernel-addr> (and/or rootfs-addr>)
 *	are considered RAM addresses, and the contents of the mtdparts
 *	will be copied to RAM and then checked against the MD5 stored
 *	in the superblock.
 *
 *	The first superblock specified <sb0-addr> is required and
 *	considered the factory superblock containing factory
 *	images.  If every superblock fails, even sb0, then the
 *	kernel (and rootfs) of sb0 will be in RAM at the end of
 *	processing.
 *
 *	This command exports the following environment variables
 *
 *		mtdparts    - A mtdparts specification
 *		kernel_addr - Kernel copy address
 *		rootfs_addr - Rootfs copy address
 *
 *  	Usage:  To use this command in u-boot, you would
 *	construct a u-boot environment like this:
 *
 *	# default u-boot environment
 *	#
 *	setenv basicargs console=ttyAMA mem=128M
 *	setenv ramargs root=/dev/ram rdinit=/etc/preinit
 *	setenv setbootargs setenv bootargs ${basicargs} ${ramargs} 
 *					mtdparts=armflash.0:${mtdparts}
 *	#
 *	# Address in RAM for uImage and rootfs copy
 *	#
 *	setenv kernel_address 210000
 *	setenv rootfs_address 410000
 *	#
 *	# Address in flash of superblocks
 *	#
 *	setenv sb0_address 34080000
 *	setenv sb1_address 340c0000
 *	#
 *	# The superblock process command
 *	#
 *	setenv process sb_process ${kernel_address} ${rootfs_address} 
 *					${sb0_address} ${sb1_address}
 *	#
 *	# The bootm command
 *	#
 *	setenv b bootm ${kernel_address} ${rootfs_address}
 *	#
 *	# Wgat to run on boot up
 *	#
 *	setenv bootcmd run process setbootargs b
 *	setenv bootdelay 5
 *
 */

/*
 * Cortina custom u-boot commands for looking at, changing, and
 * making boot decisions based on a strucutue called the superblock.
 */
#include <common.h>
#include <asm/io.h>
#include <command.h>
#include <malloc.h>

#ifdef CONFIG_ENV_IS_IN_NAND
#include <nand.h>
#endif

#include "csmi_uboot_cmds.h"

/* Global handle to CSMI library */
static csmi_uboot_cmds_handle_t ucmds = NULL;

/* Callbacks needed for flash read, write and copy */
static int read_flash(  void *buffer, ulong addr, int sz );
static int write_flash( void *buffer, ulong addr, int sz );
static int flash2ram( ulong faddr, ulong raddr, int sz );
static int jffs2ram( char *filename, ulong raddr );

extern unsigned int	gs2_flag;

/* My own internal helper functions */
extern int do_jffs2_fsload(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern int do_mem_cp ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
#if defined(CONFIG_CMD_FLASH)
	extern flash_info_t flash_info[];       /* info for FLASH chips */
#endif	
#if !defined(CONFIG_CS75XX_NONE_TO_EXT_SWITCH)
extern void rtl83xx_disble_phy(void);
extern int qca83xx_disable_phy(void);
extern int bcm53xxx_disable_phy(void);
#endif
extern int g2_phy_disable(int port);

static int flash_find_start_end( ulong start_a, ulong end_a,
				 ulong *sect_start, ulong *sect_end,
				 ulong *pwidth, ulong *cwidth );

/* Called from w/in individual commands, initializes
 * the CSMI library with my callbacks, if not already
 * initialized.
*/
void csmi_init(void) {
	if ( ucmds != NULL ) 
		return;
	ucmds = csmi_uboot_cmds_initialize(
		CONFIG_SYS_FLASH_BASE,	/* from <platform>.h */
		simple_strtoul,		/* native u-boot command */
		setenv, getenv,		/* native u-boot commands */
		malloc, free,		/* native u-boot commands */
		read_flash, write_flash,/* flash read/write */
		flash2ram, jffs2ram );	/* flash copy from mtd/jffs to ram */
}

/*
 * Display a superblock at <address>
 */
int do_sb_display( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	ulong addr;

	csmi_init();

	if ( argc < 2 ) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	addr = simple_strtoul(argv[1], NULL, 16);
	if ( csmi_uboot_cmds_sb_display( ucmds, addr ) == -1 )
		return 1;

	return 0;
}

/*
 * Set the valid, commit and active flags in a superblock
 */
int do_sb_set_flags( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	ulong addr;
	uint  v, c, a;

	csmi_init();
	
	if ( argc < 5 ) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}
	
	addr = simple_strtoul(argv[1], NULL, 16);
	v = simple_strtoul(argv[2], NULL, 16);
	c = simple_strtoul(argv[3], NULL, 16);
	a = simple_strtoul(argv[4], NULL, 16);
	
	if ( csmi_uboot_cmds_sb_set_flags( ucmds, addr, v, c, a ) == -1 )
		return 1;
	
	return 0;
}

/*
 * Main superblock processing command
 */
int do_sb_process( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	unsigned int	gs2;
	
	csmi_init();

	if ( argc < 4 ) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	if ( csmi_uboot_cmds_sb_process( ucmds, argc, argv ) == -1 ) {
		g2_wdt_stop();
		return 1;
	}
	
	if ( gs2_flag == 0) {
	   	gs2 = __raw_readl(GLOBAL_SOFTWARE2);
		if ((gs2 & 0x01)==1) {
			gs2 = gs2 & 0xfffffffe;
		}
		else {
			gs2 = gs2 | 0x01;
		}
	   	__raw_writel(gs2, GLOBAL_SOFTWARE2);
	}
	
	/* Disable PHYs until cs_ni_open() is called in Linux */
	g2_phy_disable(0);
	g2_phy_disable(1);
	g2_phy_disable(2);
#if !defined(CONFIG_CS75XX_NONE_TO_EXT_SWITCH)
#if defined(CONFIG_MK_CUSTOMB)

#warning CUSTOM_BOARD_FIX_ME

#elif defined(CONFIG_MK_REFERENCEQ)
	qca83xx_disable_phy();
#else
	rtl83xx_disble_phy();
#endif	
#endif

   	
	return 0;
}


U_BOOT_CMD(
	   sb_display, 2, 0, do_sb_display,
	   "sb_display - display a multi-image superblock",
	   "address; where address is the location of a superblock"
	   );

U_BOOT_CMD(
	   sb_set_flags, 5, 0, do_sb_set_flags,
	   "sb_set_flags - set the flags in a multi-image superblock",
	   "address valid commit active; where address is the location"	\
	   " of the superblock and valid, commit and active are booleans"
	   );

U_BOOT_CMD(
	   sb_process, 5, 0, do_sb_process,
	   "sb_process - process the superblock(s) for booting",
	   "kernel_addr rootfs_addr sb0_addr [sb1_addr ...]; "
	   );

/******************************************************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/

#if defined(CONFIG_ENV_IS_IN_NAND)

static int jffs2ram( char *filename, ulong raddr )
{
#ifdef CONFIG_CMD_JFFS2
	static char saddr[9];
	int fs_argc = 3;
	char *fs_argv[3];
	char *sz_env = getenv( "filesize" );
	int sz = 0;

	printf("%s ==> This API does not support yet.\n",__func__,raddr);
	sprintf( saddr, "%lx", raddr );
	fs_argv[0] = "fsload";
	fs_argv[1] = saddr;
	fs_argv[2] = filename;
	
	if ( do_jffs2_fsload( NULL, 0, fs_argc, fs_argv ) != 0 ) {
		return -1;
	}
	
	if ( sz_env )
		sz = (int) simple_strtoul( sz_env, NULL, 16 );
	return sz;
#else
	return 0;
#endif	
}

static int flash2ram( ulong faddr, ulong raddr, int sz )
{
	nand_info_t 	*nand;
	ulong 		off;
	int		ret = 0;

	/* the following commands operate on the current device */
	if (nand_curr_device < 0 || nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[nand_curr_device].name) {
		puts("\nno devices available\n");
		return 1;
	}
	nand = &nand_info[nand_curr_device];

	off = faddr - CONFIG_SYS_NAND_BASE;
	ret = nand_read_skip_bad(nand, off, &sz, (u_char *)raddr);

	return sz;
}

#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)

static int jffs2ram( char *filename, ulong raddr )
{
#ifdef CONFIG_CMD_JFFS2
	static char saddr[9];
	int fs_argc = 3;
	char *fs_argv[3];
	char *sz_env = getenv( "filesize" );
	int sz = 0;

	printf("%s ==> This API does not support yet.\n",__func__,raddr);
	sprintf( saddr, "%lx", raddr );
	fs_argv[0] = "fsload";
	fs_argv[1] = saddr;
	fs_argv[2] = filename;
	
	if ( do_jffs2_fsload( NULL, 0, fs_argc, fs_argv ) != 0 ) {
		return -1;
	}
	
	if ( sz_env )
		sz = (int) simple_strtoul( sz_env, NULL, 16 );
	return sz;
#else
	return 0;
#endif	
}

static int flash2ram( ulong faddr, ulong raddr, int sz )
{
	int 	argc;
	char 	*argv[5];
	char 	source[9];
	char 	target[9];
	char 	szbuf[9];

	if (faddr < CONFIG_SYS_FLASH_BASE) {
		printf("Flash address is invalid.\n");
		return 1;
	} else {
		faddr = faddr - CONFIG_SYS_FLASH_BASE;
	}
	
	sprintf( source, "%lx", faddr );
	sprintf( target, "%lx", raddr );
	sprintf( szbuf,  "%x", sz );
	argc = 5;
	argv[0] = "sf";
	argv[1] = "read";
	argv[2] = target;
	argv[3] = source;
	argv[4] = szbuf;
	do_spi_flash(NULL, 0, argc, argv);
	return sz;
}

#else

static int jffs2ram( char *filename, ulong raddr )
{
#ifdef CONFIG_CMD_JFFS2
	static char saddr[9];
	int fs_argc = 3;
	char *fs_argv[3];
	char *sz_env = getenv( "filesize" );
	int sz = 0;

	printf("%s ==> This API does not support yet.\n",__func__,raddr);
	sprintf( saddr, "%lx", raddr );
	fs_argv[0] = "fsload";
	fs_argv[1] = saddr;
	fs_argv[2] = filename;
	
	if ( do_jffs2_fsload( NULL, 0, fs_argc, fs_argv ) != 0 ) {
		return -1;
	}
	
	if ( sz_env )
		sz = (int) simple_strtoul( sz_env, NULL, 16 );
	return sz;
#else
	return 0;
#endif	
}

static int flash2ram( ulong faddr, ulong raddr, int sz )
{
	static char source[9];
	static char target[9];
	static char szbuf[9];
	int fs_argc = 4;
	char *fs_argv[4];
	
	sprintf( source, "%lx", faddr );
	sprintf( target, "%lx", raddr );
	sprintf( szbuf,  "%x", sz );
	
	#ifdef CP_BYTE
		fs_argv[0] = "cp.b";
		fs_argv[1] = source;
		fs_argv[2] = target;
		fs_argv[3] = szbuf;
	#else
		sprintf( szbuf,  "%x", (sz+3)/4 );
		fs_argv[0] = "cp.l";
		fs_argv[1] = source;
		fs_argv[2] = target;
		fs_argv[3] = szbuf;
	#endif  
  
	if ( do_mem_cp( NULL, 0, fs_argc, fs_argv ) != 0 ) {
		return -1;
	}

	return( sz );
}

#endif

int boot_kernel( unsigned long kaddr )
{
	static char s_kaddr[16];
	int 	fs_argc = 2;
	char 	*fs_argv[2];

	sprintf( s_kaddr, "%lx", kaddr );
	fs_argv[0] = "go";
	fs_argv[1] = s_kaddr;

	if ( do_go( NULL, 0, fs_argc, fs_argv ) != 0 ) {
		return -1;
	}

	return( 0 );
}

/*
 * Reading from NOR flash seems to be a simple de-reference, from
 * what I am reading in the u-boot source code.
 */
#if defined(CONFIG_ENV_IS_IN_NAND)

extern int nand_curr_device;

static int read_flash( void *buffer, ulong addr, int sz )
{
	nand_info_t 	*nand;
	ulong 		off;
	int		ret = 0;

	/* the following commands operate on the current device */
	if (nand_curr_device < 0 || nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[nand_curr_device].name) {
		puts("\nno devices available\n");
		return 1;
	}
	nand = &nand_info[nand_curr_device];

	off = addr - CONFIG_SYS_NAND_BASE;
	ret = nand_read_skip_bad(nand, off, &sz, (u_char *)buffer);

	return( sz );
}

static int write_flash( void *buffer, ulong addr, int sz )
{
	nand_erase_options_t 	nand_erase_options;
	nand_info_t 	*nand;
	ulong 		off;
	int		size;
	int		ret = 0;	

	/* the following commands operate on the current device */
	if (nand_curr_device < 0 || nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[nand_curr_device].name) {
		puts("\nno devices available\n");
		return 1;
	}

	nand = &nand_info[nand_curr_device];

	off = addr - CONFIG_SYS_NAND_BASE;
	while (nand_block_isbad(nand, off & ~(nand->erasesize - 1))) {
		off = off + nand->erasesize;
	}

	nand_erase_options.length = (sz + nand->erasesize - 1) & (~(nand->erasesize - 1));
	nand_erase_options.quiet = 0;
	nand_erase_options.jffs2 = 0;
	nand_erase_options.scrub = 0;
	nand_erase_options.offset = off;

	puts ("Erasing Nand...\n");
	if (nand_erase_opts(&nand_info[0], &nand_erase_options))
		return 1;

	/* page alignment */
	size = (sz + nand->writesize - 1) & (~(nand->writesize - 1));
	off = addr - CONFIG_SYS_NAND_BASE;
	ret = nand_write_skip_bad(nand, off, &size, (u_char *)buffer);

	return( sz );
}

#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)

static int read_flash( void *buffer, ulong addr, int sz )
{
	int 	argc;
	char 	*argv[5];
	char 	source[9];
	char 	target[9];
	char 	szbuf[9];
	
	if (addr < CONFIG_SYS_FLASH_BASE) {
		printf("Flash address is invalid.\n");
		return 1;
	} else {
		addr = addr - CONFIG_SYS_FLASH_BASE;
	}
	
	sprintf( source, "%lx", addr );
	sprintf( target, "%lx", buffer );
	sprintf( szbuf,  "%x", sz );
	argc = 5;
	argv[0] = "sf";
	argv[1] = "read";
	argv[2] = target;
	argv[3] = source;
	argv[4] = szbuf;
	do_spi_flash(NULL, 0, argc, argv);
	return( sz );
}

static int write_flash( void *buffer, ulong addr, int sz )
{
	int 	argc;
	char 	*argv[5];
	char 	source[9];
	char 	target[9];
	char 	szbuf[9];
	int	size;
	
	if (addr < CONFIG_SYS_FLASH_BASE) {
		printf("Flash address is invalid.\n");
		return 1;
	} else {
		addr = addr - CONFIG_SYS_FLASH_BASE;
	}

	size = (sz + CONFIG_ENV_SECT_SIZE - 1) & (~(CONFIG_ENV_SECT_SIZE - 1));
	sprintf( source, "%lx", addr );
	sprintf( szbuf,  "%x", size );	
	argc = 4;
	argv[0] = "sf";
	argv[1] = "erase";
	argv[2] = source;
	argv[3] = szbuf;
	do_spi_flash(NULL, 0, argc, argv);

	sprintf( source, "%lx", addr );
	sprintf( target, "%lx", buffer );
	sprintf( szbuf,  "%x", sz );
	argc = 5;
	argv[0] = "sf";
	argv[1] = "write";
	argv[2] = target;
	argv[3] = source;
	argv[4] = szbuf;
	do_spi_flash(NULL, 0, argc, argv);
	return( sz );
}

#else

static int read_flash( void *buffer, ulong addr, int sz )
{
	memcpy( buffer, (void *)addr, sz );
	return( sz );
}

/*
 * This flash write code uses NOR saveenv() as a reference.
 */
static int write_flash( void *buffer, ulong addr, int sz )
{
	int   rc = 0;
	ulong paddr_start = 0;
	ulong paddr_end = 0;
	char  *savebuf = (char *)buffer;
	int   savebuf_sz = sz;
	
	ulong port_width = 0;	// will be queried from flash chip
	ulong chip_width = 0;	// will be queried from flash chip
	
	if ( addr2info((ulong)addr) != NULL) {
	/*    printf( "%x is a flash address!\n", (uint)addr ); */
	}
	else {
		printf( "%x is NOT a flash address!\n", (uint)addr );
		return 0;
	}
	
	/*
	* Have to un-protect, re-protect the flash sector(s)
	* containing the memory to be written.  This needs
	* to be done on sector size bounderies.
	*/
	if ( flash_find_start_end( (ulong)addr, (ulong)(((ulong) addr)+sz) -1,
	     &paddr_start, &paddr_end, &port_width, &chip_width ) ) {
		return 0;
	}

#ifdef CFG_SUPERBLOCK_SHARES_SPACE
  /*
   * NOTE!!! This technique does not work for me in testing.
   * The malloc fails.  I guess I am not surprized, as the
   * malloc is for a sector, and u-boot itself resides in
   * one sector (in my test env), so it probably doesn't
   * have room to malloc this much memory.
   *
   * SO DON'T TURN THIS SECTION OF THE CODE ON, unless you
   * know what you are doing and test that it works.
   */
  /*
   * Since we are dealing with sector-sized chunks, and the
   * superblock may reside somewhere in the middle and might
   * share its sectors with other data (?) then we need to
   * preserve the other data in the sector(s)...
   */
	savebuf_sz = paddr_end - paddr_start + 1;
	savebuf = malloc( savebuf_sz );
	if ( savebuf == NULL ) {
		printf("Unable to malloc memory to preserve sector(s)"	\
		       " surrounding the superblock!\n" );
		return 0;
	}
	memcpy( (void*)savebuf, (void*)paddr_start, (size_t)savebuf_sz );
	
	/* Now paint our buffer into the savebuf */
	memcpy( (void*)(savebuf+(((ulong)addr)-paddr_start)), (void*)buffer,sz);
#endif

	if ( flash_sect_protect( 0, paddr_start, paddr_end ) ) {
		return 0;
	}
	
	if ( flash_sect_erase( paddr_start, paddr_end ) ) {
		return 0;
	}

  /*
   * Now we can do the write...
   */

  // IMPORTANT!!!
  // The size being written is in bytes, but must be aligned
  // to the natural word size of the flash being used!
  //
	if ( savebuf_sz % chip_width )
		savebuf_sz += chip_width - ( savebuf_sz % chip_width );
	
	rc = flash_write( (char*)savebuf, (ulong)addr, savebuf_sz );
	if ( rc != 0 ) {
		flash_perror( rc );
		return 0;
	}

	/* don't protect kernel and rootfs sectors */
	if ((paddr_start & 0x0FFFFFFF) >= 0x100000) {
		if ( flash_sect_protect( 1, paddr_start, paddr_end ) ) {
			return 0;
		}
	}
	
	return( sz );
}

static int flash_find_start_end( ulong start_a, ulong end_a,
				 ulong *addr_first, ulong *addr_last,
				 ulong *port_width, ulong *chip_width )
{
	int found = 0;
	ulong bank = 0;
	
	*addr_first = start_a;
	*addr_last  = end_a;
	
	for (bank = 0; bank < CONFIG_SYS_MAX_FLASH_BANKS && !found; ++bank){
		int i;
		flash_info_t *info = &flash_info[bank];
		for (i = 0; i < info->sector_count && !found; ++i){
		/* get the end address of the sector */
			ulong sector_end_addr;
			if (i == info->sector_count - 1) {
				sector_end_addr = info->start[0] + info->size-1;
			} else {
				sector_end_addr =  info->start[i+1] - 1;
			}
			if (*addr_last <= sector_end_addr && 	\
			    *addr_last >= info->start[i]){
				/* sector found */
				found = 1;
				/* adjust *addr_last if necessary */
				if (*addr_last < sector_end_addr){
	  				*addr_last = sector_end_addr;
				}
			}
		} /* sector */
	} /* bank */
	
	if (!found){
		/* error, addres not in flash */
		printf("Error: end address (0x%08lx) not in flash!\n", \
			*addr_last);
		return 1;
	}
	
	found = 0;
	for (bank = 0; bank < CONFIG_SYS_MAX_FLASH_BANKS && !found; ++bank){
		int i;
		flash_info_t *info = &flash_info[bank];
		for (i = 0; i < info->sector_count && !found; ++i){
			/* get the end address of the sector */
			ulong sector_end_addr;
			if (i == info->sector_count - 1) {
				sector_end_addr = info->start[0] + info->size-1;
			} else {
				sector_end_addr = info->start[i+1] - 1;
			}
	
			if ( *addr_first >= info->start[i] &&
	   		*addr_first <= sector_end_addr ) {
				found = 1;
				*addr_first = info->start[i];
	
				*port_width = info->portwidth;
				*chip_width = info->chipwidth;
			}
		} /* sector */
	} /* bank */
	
	if (!found){
		/* error, addres not in flash */
		printf("Error: start address (0x%08lx) not in flash!\n", \	
			*addr_first);
		return 1;
	}
	
	return 0;
}
#endif

int upgrade_flash( void *buffer, ulong addr, int sz, uint psz )
{
#ifdef CONFIG_ENV_IS_IN_NAND
	nand_erase_options_t 	nand_erase_options;
	nand_info_t 	*nand;
	ulong 		off;
	uint		size;
	uint		bad_size = 0;
	int		ret = 0;	

	/* the following commands operate on the current device */
	if (nand_curr_device < 0 || nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
	    !nand_info[nand_curr_device].name) {
		puts("\nno devices available\n");
		return 1;
	}

	nand = &nand_info[nand_curr_device];

	/* calculate bad block size */
	off = addr - CONFIG_SYS_NAND_BASE;
	while (off < (addr - CONFIG_SYS_NAND_BASE + psz)) {
		if (nand_block_isbad(nand, off & ~(nand->erasesize - 1)))
			bad_size += nand->erasesize;
		off = off + nand->erasesize;
	}
	
	if ((sz + bad_size) > psz) {
		printf("Good NAND blocks size is too small to write the image");
		return 1;
	}

	off = addr - CONFIG_SYS_NAND_BASE;
	while (nand_block_isbad(nand, off & ~(nand->erasesize - 1))) {
		off = off + nand->erasesize;
	}

	nand_erase_options.length = ((psz - bad_size) + nand->erasesize - 1) 
			& (~(nand->erasesize - 1));
	nand_erase_options.quiet = 0;
	nand_erase_options.jffs2 = 0;
	nand_erase_options.scrub = 0;
	nand_erase_options.offset = off;

	printf("Erasing Nand...offset=%x length=%x\n",
			nand_erase_options.offset, nand_erase_options.length);
	if (nand_erase_opts(&nand_info[0], &nand_erase_options))
		return 1;

	/* page alignment */
	size = (sz + nand->writesize - 1) & (~(nand->writesize - 1));
	off = addr - CONFIG_SYS_NAND_BASE;
	ret = nand_write_skip_bad(nand, off, &size, (u_char *)buffer);

	return( sz );
#else
	return write_flash(buffer, addr, sz);
#endif
}
