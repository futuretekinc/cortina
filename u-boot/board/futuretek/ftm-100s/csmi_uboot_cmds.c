/*
 *
 * Plugin u-boot commands for multi-image booting.
 */
#ifndef _FROM_UBOOT_
#  include <stdio.h>
#  include <string.h>
#endif
#ifdef _FROM_UBOOT_
#  include <common.h>
#endif
#include "csmi_uboot_cmds.h"
#include "csmi_user.h"

static csmi_sb_handle_t read_sb( csmi_uboot_cmds_struct_t *s, unsigned long addr );

csmi_uboot_cmds_handle_t csmi_uboot_cmds_initialize( unsigned long flash_base,
  	strtoul_cb_t	strtoul_a,
  	setenv_cb_t		setenv_a,
  	getenv_cb_t		getenv_a,
  	malloc_cb_t		malloc_a,
  	free_cb_t		free_a,
  	read_flash_cb_t	read_flash_a,
  	write_flash_cb_t	write_flash_a,
  	cp_flash_ram_cb_t	cp_flash_ram_a,
  	cp_jffs2_ram_cb_t	cp_jffs2_ram_a )
{
  	csmi_uboot_cmds_struct_t *s = malloc_a( sizeof(csmi_uboot_cmds_struct_t) );
  	if ( s == NULL ) return NULL;

  	s->flash_base = 	flash_base;
  	s->strtoul =		strtoul_a;
  	s->setenv =		setenv_a;
  	s->getenv =		getenv_a;
  	s->malloc =		malloc_a;
  	s->free =		free_a;
  	s->read_flash =	read_flash_a;
  	s->write_flash =	write_flash_a;
  	s->cp_flash_ram =	cp_flash_ram_a;
  	s->cp_jffs2_ram =	cp_jffs2_ram_a;

  	return( (csmi_uboot_cmds_handle_t) s );
}

int csmi_uboot_cmds_sb_display(csmi_uboot_cmds_handle_t handle, unsigned long sb_address)
{
  	csmi_uboot_cmds_struct_t *s = (csmi_uboot_cmds_struct_t *) handle;
  	csmi_sb_handle_t sb = read_sb( s, sb_address );
  	if ( sb == NULL ) return -1;
  	csmi_sb_print( sb, 0 );
  	csmi_sb_freeup( sb );
  	s->free( sb );
  	return( 1 );
}

int csmi_uboot_cmds_sb_set_flags(csmi_uboot_cmds_handle_t handle,
  	unsigned long sb_address, unsigned long v, unsigned long c, unsigned long a )
{
	csmi_uboot_cmds_struct_t *s = (csmi_uboot_cmds_struct_t *) handle;
	csmi_sb_handle_t sb = read_sb( s, sb_address );

	if ( sb == NULL ) return -1;

	csmi_sb_set_valid(  sb, v );
	csmi_sb_set_commit( sb, c );
	csmi_sb_set_active( sb, a );

	int sz = csmi_sb_size( sb );
	if ( s->write_flash( (void*)sb, sb_address, sz ) != sz ) {
		csmi_sb_freeup( sb );
		s->free( sb );
		return -1;
	}
	csmi_sb_freeup( sb );
	s->free( sb );
	return( 1 );
}

// Support up to three superblocks (two plus factory)
#define SB_SUPPORT 3

// Need a data struct to hold all the conversion data
typedef struct {
  	unsigned long	address;
  	char *s_address;
  	csmi_sb_handle_t sb;
} sbinfo_t;

static sbinfo_t sbinfo[SB_SUPPORT];

int csmi_uboot_cmds_boot_index(void)
{
	unsigned int i;

	/* check valid=1 and active=1 */		
	for (i=0; i<SB_SUPPORT; i++) {
		if (!sbinfo[i].sb)
			break;
		if (csmi_sb_valid(sbinfo[i].sb) && csmi_sb_active(sbinfo[i].sb))
		    return (i);
	}

	/* check valid=1 and commit=1 */		
	for (i=0; i<SB_SUPPORT; i++) {
		if (!sbinfo[i].sb)
			break;
		if (csmi_sb_valid(sbinfo[i].sb) && csmi_sb_commit(sbinfo[i].sb))
		    return (i);
	}

	/* check valid=1 */		
	for (i=0; i<SB_SUPPORT; i++) {
		if (!sbinfo[i].sb)
			break;
		if (csmi_sb_valid(sbinfo[i].sb))
		    return (i);
	}

	return (-1);
}

int csmi_uboot_cmds_sb_process( csmi_uboot_cmds_handle_t handle, int argc, char *argv[] )
{
	csmi_uboot_cmds_struct_t *s = (csmi_uboot_cmds_struct_t *) handle;
	//  sbinfo_t sbinfo[SB_SUPPORT];
	int have_kaddr = 0;
	int have_raddr = 0;
	ulong kaddr = 0;
	ulong f_kaddr = 0;
	ulong ksize = 0;
	ulong raddr = 0;
	ulong rsize = 0;
	char *s_kaddr = argv[1];
	char *s_raddr = argv[2];
	char new_address_buf[9];
	
	// Usage: process kernel-ram rootfs-ram sb0 [sb1 ...]
	// kernel-ram and rootfs-ram can be '-' meaning we're
	// executing from flash (no ram copy)
	//
	if ( s_kaddr[0] != '-' ) {
		have_kaddr = 1;
		kaddr = s->strtoul( s_kaddr, NULL, 16 );
	}
	
	if ( s_raddr[0] != '-' ) {
		have_raddr = 1;
		raddr = s->strtoul( s_raddr, NULL, 16 );
	}
	
	int i;
	for( i=0; i<SB_SUPPORT; i++ ) {
		sbinfo[i].address = 0; 
		sbinfo[i].s_address = NULL; 
		sbinfo[i].sb = NULL;
		if ( ((i+SB_SUPPORT) <= argc) && argv[i+SB_SUPPORT] ) {
			sbinfo[i].s_address = argv[i+SB_SUPPORT];
			sbinfo[i].address   = s->strtoul( sbinfo[i].s_address, NULL, 16 );
			sbinfo[i].sb = read_sb( s, sbinfo[i].address );
		}
	}
	
	//
	// A superblock is good if valid and ( commit or active ).
	// Active should only be on for one superblock, enforced
	// by system upgrader.
	//
	int good = 0;
	///  for( i=(SB_SUPPORT-1); i>=0; i-- ) {
	///    if ( sbinfo[i].sb ) {
	///      if ( csmi_sb_valid( sbinfo[i].sb ) &&
	///	   ( csmi_sb_commit( sbinfo[i].sb ) || csmi_sb_active( sbinfo[i].sb ) ) ) {
	while ((i = csmi_uboot_cmds_boot_index()) >= 0) {
		// Is there a valid kernel for this sb?  The kernel may be a file name
		// if coming from a JFFS2, or may start with '0x' if its on a MTD
		// partition.
		char *kernel = csmi_sb_kernel( sbinfo[i].sb );
		if ( kernel == NULL ) {
			printf( "BAD: Kernel is missing on SB at %s\n", sbinfo[i].s_address );
			csmi_sb_set_valid(sbinfo[i].sb, 0);
			goto DONE;
		}
	
		if ( kernel[0] == '0' && kernel[1] == 'x' ) {
			// We're booting off of a raw mtd partition
			ulong offset = simple_strtoul( kernel, NULL, 16 );
			ulong flash_addr = s->flash_base + offset;
			sprintf( new_address_buf, "%lx", flash_addr );
			if ( have_kaddr == 0 ) {
				// Booting from flash
				s_kaddr = new_address_buf;
				f_kaddr = flash_addr;
			} else {
				// copy to ram, check md5
				csmi_sb_mtdpart_handle_t h = csmi_sb_kernel_mtdpart_handle( sbinfo[i].sb );
				
				ksize = csmi_sb_mtdpart_filesize( h );
				
				printf( "Kernel: Copy %d bytes from flash address 0x%08x to RAM address 0x%08x ...\n",
					csmi_sb_mtdpart_filesize( h ),
					flash_addr, kaddr );
				if ( s->cp_flash_ram( flash_addr, kaddr,
					  csmi_sb_mtdpart_filesize( h ) ) != csmi_sb_mtdpart_filesize( h ) ) {
					// Copy failed
					printf( "Failed to copy %lx to %lx\n", flash_addr, kaddr );
					csmi_sb_set_valid( sbinfo[i].sb, 0 );
					goto DONE;
				}
				// Check MD5
				printf( "Checking MD5 at address 0x%08x for %d bytes ...\n", kaddr,
				csmi_sb_mtdpart_filesize( h ) );
				if ( csmi_sb_check_md5_mem( (unsigned char *)kaddr,
						csmi_sb_mtdpart_filesize( h ),
						csmi_sb_mtdpart_md5( h ) ) == -1 ) {
					// md5 failed
					printf( "MD5 check failed for %lx\n", kaddr );
					csmi_sb_set_valid( sbinfo[i].sb, 0 );
					goto DONE;
				}
			}
		} else {
			// booting from JFFS2
			csmi_sb_file_handle_t h = csmi_sb_kernel_file_handle( sbinfo[i].sb );
			
			ksize = csmi_sb_file_size( h );
			
			printf( "Kernel: Copy %s to RAM address 0x%08x ...\n", kernel, kaddr );
			if ( s->cp_jffs2_ram( kernel, kaddr ) != csmi_sb_file_size( h ) ) {
				// Copy failed
				printf( "Failed to copy %s to %lx\n", kernel, kaddr );
				csmi_sb_set_valid( sbinfo[i].sb, 0 );
				goto DONE;
			}
		
			// Check MD5
			printf( "Checking MD5 at address 0x%08x for %d bytes ...\n", kaddr,
			csmi_sb_file_size( h ) );
			if ( csmi_sb_check_md5_mem( (unsigned char *)kaddr,
				      csmi_sb_file_size( h ),
				      csmi_sb_file_md5( h ) ) == -1 ) {
				// md5 failed
				printf( "MD5 check failed for %lx\n", kaddr );
				csmi_sb_set_valid( sbinfo[i].sb, 0 );
				goto DONE;
			}
		}
	
		setenv( "kernel_address", s_kaddr );
	
		// ROOTFS
	
		char *rootfs = csmi_sb_rootfs( sbinfo[i].sb );
		if ( rootfs != NULL ) {
	  		if ( rootfs[0] == '0' && rootfs[1] == 'x' ) {
				// MTD
				ulong offset = simple_strtoul( rootfs, NULL, 16 );
				ulong flash_addr = s->flash_base + offset;
				sprintf( new_address_buf, "%lx", flash_addr );
				if ( have_raddr == 0 ) {
					csmi_sb_mtdpart_handle_t h = csmi_sb_rootfs_mtdpart_handle( sbinfo[i].sb );
			
					rsize = csmi_sb_mtdpart_filesize( h );
					// Booting from flash
					s_raddr = new_address_buf;
	    			} else {
	      				// copy to ram, check md5
	      				csmi_sb_mtdpart_handle_t h = csmi_sb_rootfs_mtdpart_handle( sbinfo[i].sb );
	
	      				rsize = csmi_sb_mtdpart_filesize( h );
	
					printf( "Rootfs: Copy %d bytes from flash address 0x%08x to RAM address 0x%08x ...\n",
					      csmi_sb_mtdpart_filesize( h ),
					      flash_addr, raddr );
					if ( s->cp_flash_ram( flash_addr, raddr,
							    csmi_sb_mtdpart_filesize( h ) ) != csmi_sb_mtdpart_filesize( h ) ) {
						// Copy failed
						printf( "Failed to copy %lx to %lx\n", flash_addr, raddr );
						csmi_sb_set_valid( sbinfo[i].sb, 0 );
						goto DONE;
					}
	
					// Check MD5
					printf( "Checking MD5 at address 0x%08x for %d bytes ...\n", raddr,
					      csmi_sb_mtdpart_filesize( h ) );
					if ( csmi_sb_check_md5_mem( (unsigned char *)raddr,
								  csmi_sb_mtdpart_filesize( h ),
								  csmi_sb_mtdpart_md5( h ) ) == -1 ) {
						// md5 failed
						printf( "MD5 check failed for %lx\n", raddr );
						csmi_sb_set_valid( sbinfo[i].sb, 0 );
						goto DONE;
					}
				}
			} else {
				// JFFS
				csmi_sb_file_handle_t h = csmi_sb_rootfs_file_handle( sbinfo[i].sb );
				
				rsize = csmi_sb_file_size( h );
				
				printf( "Rootfs: Copy %s to RAM address 0x%08x ...\n", rootfs, raddr );
				if ( s->cp_jffs2_ram( rootfs, raddr ) != csmi_sb_file_size( h ) ) {
					// Copy failed
					printf( "Failed to copy %s to %lx\n", rootfs, raddr );
					csmi_sb_set_valid( sbinfo[i].sb, 0 );
					goto DONE;
				}
	
				// Check MD5
				printf( "Checking MD5 at address 0x%08x for %d bytes ...\n", raddr,
				      csmi_sb_file_size( h ) );
				if ( csmi_sb_check_md5_mem( (unsigned char *)raddr,
							csmi_sb_file_size( h ), csmi_sb_file_md5( h ) ) == -1 ) {
					// md5 failed
					printf( "MD5 check failed for %lx\n", raddr );
					csmi_sb_set_valid( sbinfo[i].sb, 0 );
					goto DONE;
				}
			}
	  		setenv( "rootfs_address", s_raddr );
	///	}
	// else, rootfs can be initramrd in kernel, not an error
	
			good = 1;
	
DONE:
			setenv ( "mtdparts", csmi_sb_mtdparts( sbinfo[i].sb ) );
			if ( good ) {
				// If the Sb we're booting from is c=1, a=0, then
				// set c=0, a=1 and reset any other sb a bits
				if ( csmi_sb_commit( sbinfo[i].sb ) == 1 &&
				csmi_sb_active( sbinfo[i].sb ) == 0 ) {
					csmi_sb_set_commit( sbinfo[i].sb, 0 );
					csmi_sb_set_active( sbinfo[i].sb, 1 );
					s->write_flash( (void *)sbinfo[i].sb, sbinfo[i].address,
						    csmi_sb_size( sbinfo[i].sb ) );
		
					int j;
					for( j=(SB_SUPPORT-1); j>=0; j-- ) {
						if ( j != i ) {
							if ( csmi_sb_active( sbinfo[j].sb ) ) {
								csmi_sb_set_active( sbinfo[j].sb, 0 );
								s->write_flash( (void *)sbinfo[j].sb, sbinfo[j].address,
									  csmi_sb_size( sbinfo[j].sb ) );
							}
						}
					}
		  		}
		  		break;//Amos
	  // WE ARE DONE, BREAK OUT OF THE LOOP
	///	  break;
			} else if ( csmi_sb_valid( sbinfo[i].sb ) == 0 ) {
				// write back the sb to reset valid bit
				s->write_flash( (void *)sbinfo[i].sb, sbinfo[i].address,
					  csmi_sb_size( sbinfo[i].sb ) );
			}
		} else {
			// invalid flags
			printf( "Skipping SB at %s because v=%d, c=%d, a=%d\n",
				sbinfo[i].s_address, csmi_sb_valid(  sbinfo[i].sb ),
				csmi_sb_commit( sbinfo[i].sb ), csmi_sb_active( sbinfo[i].sb ) );
		}
	}
	///    else {
	// sb[i] == NULL
	///    }
	///  }
	
	if ( good || ( i==0 ) ) {
		printf( "Booting from SB at %s\n", sbinfo[i].s_address );
		good = 1;
	}
	
	for( i=0; i<SB_SUPPORT; i++ ) {
		if ( sbinfo[i].sb ) {
			csmi_sb_freeup( sbinfo[i].sb );
			s->free( sbinfo[i].sb );
		}
	}

	if (!good) {
		printf("There is no valid kernel image!\n");
		return (-1);
	}
	
	return 1;
}

/*
 * P R I V A T E  F U N C T I O N S
 */

csmi_sb_handle_t read_sb( csmi_uboot_cmds_struct_t *s, unsigned long addr )
{
  	unsigned long hdr[2];  // magic and size

  	if ( s->read_flash( (void *)hdr, addr, sizeof(unsigned long) * 2 ) !=
    (sizeof( unsigned long) * 2) ) {
    	printf( "Failed to fetch magic and size from 0x%lx.\n", addr );
    	return NULL;
  	}

  	if ( csmi_sb_check_magic( hdr[0] ) == 0 ) {
    	printf( "Magic number check failed for 0x%lx.\n", addr );
    	return NULL;
  	}

  	unsigned char *dest = (unsigned char *)s->malloc( hdr[1] );
  	if ( dest == NULL ) {
    	printf( "Failed to malloc memory to hold 0x%lx.\n", addr );
    	return NULL;
  	}

  	if ( s->read_flash( (void *)dest, addr, hdr[1] ) != hdr[1] ) {
    	s->free( dest );
    	printf( "Failed to read SB at 0x%lx\n", addr );
    	return NULL;
  	}

  	csmi_sb_handle_t sb = csmi_sb_read_mem( dest, (unsigned char *)-1 );
  	if ( sb == NULL ) {
    	s->free( dest );
    	printf( "Failed to find a good SB at 0x%lx\n", addr );
    	return NULL;
  	}
  	return( sb );
}

