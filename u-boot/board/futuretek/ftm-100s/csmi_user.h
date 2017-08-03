#ifndef _CSMI_USER_H_
#define _CSMI_USER_H_

// This enum is used to indicate what a file is
//
typedef enum {
  CSMI_FILE_IS_OTHER = 0,
  CSMI_FILE_IS_KERNEL,
  CSMI_FILE_IS_ROOTFS,
  CSMI_FILE_IS_FS,
  CSMI_FILE_IS_DATA
} csmi_file_is_e;

// Flags access
#define MTDPART_FLAG_DONT_EXPORT(wrd) ((wrd >> 0)&0x1)
#define MTDPART_FLAG_SET_DONT_EXPORT(wrd,n) (wrd | (n<<0))

// Parts of the superblock accessible by handles
//
typedef void* csmi_sb_handle_t;
typedef void* csmi_sb_file_handle_t;
typedef void* csmi_sb_mtdpart_handle_t;

// Read a superblock from a file/device
#ifndef _FROM_UBOOT_
extern csmi_sb_handle_t csmi_sb_read_file( int fd );
#endif

// Read a superblock from an address location in memory.
// If dest is NULL, memory is allocated.  If dest is non-NULL
// it is used to hold the copy.
extern csmi_sb_handle_t csmi_sb_read_mem( unsigned char *address, unsigned char *dest );

// Create a new superblock
extern csmi_sb_handle_t csmi_sb_create( void );

// free up any memory allocated
extern void csmi_sb_freeup( csmi_sb_handle_t handle );

// Check the memory at address for a good
// md5 signature.  Returns -1 if bad, 1 if
// good.
//
extern int csmi_sb_check_md5_mem( unsigned char *address, unsigned int size, unsigned char expected_md5[16] );

// Check a passed in magic against the known magic
//
extern int csmi_sb_check_magic( unsigned long magic );

// Calculate and place a new md5 on SB
//
extern void csmi_sb_new_md5( csmi_sb_handle_t handle );

// Flag access
extern unsigned int csmi_sb_valid( csmi_sb_handle_t handle );
extern unsigned int csmi_sb_commit( csmi_sb_handle_t handle );
extern unsigned int csmi_sb_active( csmi_sb_handle_t handle );
extern unsigned int csmi_sb_rsvd( csmi_sb_handle_t handle );

extern void csmi_sb_set_valid( csmi_sb_handle_t handle, unsigned char yesno );
extern void csmi_sb_set_commit( csmi_sb_handle_t handle, unsigned char yesno );
extern void csmi_sb_set_active( csmi_sb_handle_t handle, unsigned char yesno );
extern void csmi_sb_set_rsvd( csmi_sb_handle_t handle, unsigned char yesno );

// Print out an ascii representation
void csmi_sb_print( csmi_sb_handle_t handle, int fd );

// Obtain specific strings that u-boot needs
extern char *csmi_sb_kernel( csmi_sb_handle_t handle ); // pathname to kernel image, or offset if mtdpart
extern char *csmi_sb_rootfs( csmi_sb_handle_t handle ); // pathname to rootfs image, or offset if mtdpart
extern char *csmi_sb_mtdparts( csmi_sb_handle_t handle ); // mtdparts string
extern char *csmi_sb_root( csmi_sb_handle_t handle ); // pathname to files
extern void csmi_sb_set_root( csmi_sb_handle_t handle, char *root );
extern unsigned int csmi_sb_size( csmi_sb_handle_t handle ); // return the size

// Return special file handles
//
extern csmi_sb_file_handle_t csmi_sb_kernel_file_handle( csmi_sb_handle_t handle );
extern csmi_sb_file_handle_t csmi_sb_rootfs_file_handle( csmi_sb_handle_t handle );
extern csmi_sb_file_handle_t csmi_sb_named_file_handle( csmi_sb_handle_t handle, char *name );

extern csmi_sb_mtdpart_handle_t csmi_sb_kernel_mtdpart_handle( csmi_sb_handle_t handle );
extern csmi_sb_mtdpart_handle_t csmi_sb_rootfs_mtdpart_handle( csmi_sb_handle_t handle );
extern csmi_sb_mtdpart_handle_t csmi_sb_named_mtdpart_handle( csmi_sb_handle_t handle, char *name );

// Individual mtdpart and file access
extern csmi_sb_file_handle_t csmi_sb_first_file( csmi_sb_handle_t handle );
extern csmi_sb_file_handle_t csmi_sb_next_file( csmi_sb_handle_t handle );

extern csmi_sb_mtdpart_handle_t csmi_sb_first_mtdpart( csmi_sb_handle_t handle );
extern csmi_sb_mtdpart_handle_t csmi_sb_next_mtdpart( csmi_sb_handle_t handle );

// File access methods
extern char *csmi_sb_file_name( csmi_sb_file_handle_t handle );
extern unsigned int csmi_sb_file_version( csmi_sb_file_handle_t handle );
extern unsigned int csmi_sb_file_size( csmi_sb_file_handle_t handle );
extern csmi_file_is_e csmi_sb_file_is( csmi_sb_file_handle_t handle );
extern char *csmi_sb_file_type( csmi_sb_file_handle_t handle );
extern unsigned int csmi_sb_file_flags( csmi_sb_file_handle_t handle );
extern char *csmi_sb_file_mountpoint( csmi_sb_file_handle_t handle );
extern char *csmi_sb_file_mountopts( csmi_sb_file_handle_t handle );
extern unsigned char *csmi_sb_file_md5( csmi_sb_file_handle_t handle );

extern void csmi_sb_file_set_md5( csmi_sb_file_handle_t handle, unsigned char new_md5[16] );
extern void csmi_sb_file_set_size( csmi_sb_file_handle_t handle, unsigned int new_size );
extern void csmi_sb_file_set_version( csmi_sb_file_handle_t handle, unsigned int new_version );

// Mtdpart access methods
extern char *csmi_sb_mtdpart_name( csmi_sb_mtdpart_handle_t handle );
extern char *csmi_sb_mtdpart_size( csmi_sb_mtdpart_handle_t handle );
extern unsigned int csmi_sb_mtdpart_filesize( csmi_sb_mtdpart_handle_t handle );
extern char *csmi_sb_mtdpart_offset( csmi_sb_mtdpart_handle_t handle );
extern csmi_file_is_e csmi_sb_mtdpart_is( csmi_sb_mtdpart_handle_t handle );
extern char *csmi_sb_mtdpart_type( csmi_sb_mtdpart_handle_t handle );
extern char *csmi_sb_mtdpart_mountpoint( csmi_sb_mtdpart_handle_t handle );
extern char *csmi_sb_mtdpart_mountopts( csmi_sb_mtdpart_handle_t handle );
extern unsigned char *csmi_sb_mtdpart_md5( csmi_sb_mtdpart_handle_t handle );
extern unsigned int csmi_sb_mtdpart_flags( csmi_sb_mtdpart_handle_t handle );

extern void csmi_sb_mtdpart_set_md5( csmi_sb_mtdpart_handle_t handle, unsigned char new_md5[16] );
extern void csmi_sb_mtdpart_set_filesize( csmi_sb_mtdpart_handle_t handle, unsigned int new_size );

// Routines for adding new files and mtdparts
extern csmi_sb_handle_t csmi_sb_add_file
( csmi_sb_handle_t handle,
  char *name, 				// file basename
  char *path, unsigned char md5[],	// full path, or md5 pre-calculated
  unsigned int version,			// version number
  csmi_file_is_e file_is,		// what is it?
  char *type,				// type?
  unsigned int flags,			// flags
  char *mountpoint, char *mountopts 	// optional mount options
  );

extern csmi_sb_handle_t csmi_sb_add_mtdpart
( csmi_sb_handle_t handle,
  char *name, 				// partition name
  char *path, unsigned char md5[],	// full path, or md5 pre-calculated
  char *size,				// partition size: '2M', '256K', ...
  char *offset,				// '0x1080000', ...
  csmi_file_is_e mtdpart_is,		// what is it?
  char *type,				// type?
  unsigned int flags,			// flags
  char *mountpoint, char *mountopts 	// optional mount options
  );

// Return a string rep. of an md5 value
extern char *csmi_sb_md5_string( unsigned char arr[16] );

// Print the what enum as a string
//
extern char *csmi_sb_file_is_str( csmi_file_is_e what );

// Strickly for debug
extern void csmi_sb_check_managed( void );

#endif
