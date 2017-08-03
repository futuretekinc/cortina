#ifndef _FROM_UBOOT_
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <malloc.h>
#endif

#ifdef _FROM_UBOOT_
#  include <common.h>
#  include <malloc.h>
#endif

#ifdef HAVE_FILE_MD5_SUPPORT
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#endif

#include "csmi_user.h"
#include "csmi_struct.h"
#include "u-boot/md5.h"

static void add_managed( void *child, void *parent );
static void free_managed( void *parent );
static void patch_managed( void *oldp, void *newp );
static void redo_md5( csmi_sb_s *o );

typedef struct {
  	void *child;
  	void *parent;
} managed_element_t;

static managed_element_t *managed_elements = NULL;
static int num_managed_elements = 0;

static csmi_sb_file_s *last_file;
static csmi_sb_mtdpart_s *last_mtdpart;

#ifndef _FROM_UBOOT_
// Read a superblock from a file/device
csmi_sb_handle_t csmi_sb_read_file( int fd )
{
  	unsigned int  magic;
  	unsigned int  size;
  	unsigned char file_md5[16];
  	unsigned char calculated_md5[16];
  	unsigned char *dest;

  	if ( read( fd, &magic, sizeof( unsigned int ) ) != sizeof( unsigned int ) ) {
    	printf( "Failed to read magic number!\n" );
    	return( NULL );
  	}

  	if ( magic != CS_SB_MAGIC ) {
    	printf( "Unknown magic number: expected 0x%08x, found 0x%08x\n",
	    	CS_SB_MAGIC, magic );
    	return( 0 );
  	}

  	if ( read( fd, &size, sizeof( unsigned int ) ) != sizeof( unsigned int ) ) {
    	printf( "Failed to read SB size!\n" );
    	return( NULL );
  	}

  	if ( read( fd, file_md5, sizeof( unsigned char) * 16 ) != sizeof( unsigned char) * 16 ) {
    	printf( "Failed to read MD5 from file!\n" );
  	}
  
  	dest = (unsigned char *) malloc( size );

  	int off = 0;
  	memcpy( dest + off, &magic, sizeof( unsigned int ) );
  	off += sizeof( unsigned int );
  	memcpy( dest + off, &size, sizeof( unsigned int ) );
  	off += sizeof( unsigned int );
  	memcpy( dest + off, file_md5, sizeof( unsigned char ) * 16 );
  	off += sizeof( unsigned char ) * 16;

  	int sz = read( fd, dest + off, size - off );
  	if ( sz != (size - off ) ) {
    	printf( "Failed to read sb from file.  Read %d bytes, expected %d bytes.\n",
	    	sz, size - off );
    	exit( 1 );
  	}

  	unsigned int md5offset = ((sizeof(unsigned int)*2) + (sizeof(unsigned char)*16));

  	if ( csmi_sb_check_md5_mem( dest + md5offset,
			      size - md5offset, file_md5 ) == -1 ) {
    	free( dest );
    	return( NULL );
  	}
  	add_managed( NULL, (void *)dest );

  	return( (csmi_sb_handle_t) dest );
}
#endif

// Read a superblock from an address location in memory
csmi_sb_handle_t csmi_sb_read_mem( unsigned char *address,
				   unsigned char *dest )
{
  	unsigned int  magic;
  	unsigned int  size;
  	unsigned char file_md5[16];

  	memcpy( (unsigned int *)&magic, address, sizeof( unsigned int ) );

  	if ( magic != CS_SB_MAGIC ) {
    	printf( "Unknown magic number: expected 0x%08x, found 0x%08x\n",
	    	CS_SB_MAGIC, magic );
    	return( 0 );
  	}

  	memcpy( (void *)&size, (void *)(address + ( 1 * sizeof( unsigned int ))), 
	  	sizeof( unsigned int ) );

  	memcpy( (void *)&file_md5, (void *)(address + ( 2 * sizeof( unsigned int ))), 
	  	sizeof( unsigned char ) * 16 );

  	unsigned int md5offset = ((sizeof( unsigned int  ) * 2) +
			    (sizeof( unsigned char ) * 16) );

  	if ( csmi_sb_check_md5_mem( address + md5offset,
			      size - md5offset, file_md5 ) == -1 ) {
    	return( NULL );
  	}

  	if ( dest == (unsigned char *)-1 ) {
    	// In place
    	return( (csmi_sb_handle_t) address );
  	}
  	else if ( dest != NULL ) {
    	// app has done a malloc
    	memcpy( dest, address, size );
  	}
  	else {
    	// do our own malloc
    	dest = (unsigned char *) malloc( size );
    	if ( dest == NULL ) {
      		printf( "Failed to malloc %d bytes for SB!\n", size );
      		return( NULL );
    	}
    	memcpy( dest, address, size );
    	add_managed( NULL, (void *)dest );
  	}

  	return( (csmi_sb_handle_t) dest );
}

// Create a new superblock
csmi_sb_handle_t csmi_sb_create( void )
{
  	void *handle = (void *) malloc( sizeof( csmi_sb_s ) );
  	memset( handle, 0, sizeof( csmi_sb_s ) );
  	add_managed( NULL, handle );
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	o->magic = CS_SB_MAGIC;
  	o->size  = sizeof( csmi_sb_s );
  	redo_md5( o );
  	return( handle );
}

// free up any memory allocated
void csmi_sb_freeup( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	free_managed( (void *) o );
}

int csmi_sb_check_magic( unsigned long magic )
{
  	return( magic == CS_SB_MAGIC );
}

// Flag access
unsigned int csmi_sb_valid( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return( o->flags.bit.valid );
}

unsigned int csmi_sb_commit( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return( o->flags.bit.commit );
}

unsigned int csmi_sb_active( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return( o->flags.bit.active );
}

unsigned int csmi_sb_rsvd( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return( o->flags.bit.rsvd );
}

void csmi_sb_set_valid( csmi_sb_handle_t handle, unsigned char yesno )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	o->flags.bit.valid = yesno;
  	redo_md5( o );
}

void csmi_sb_set_commit( csmi_sb_handle_t handle, unsigned char yesno )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	o->flags.bit.commit = yesno;
  	redo_md5( o );
}

void csmi_sb_set_active( csmi_sb_handle_t handle, unsigned char yesno )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	o->flags.bit.active = yesno;
  	redo_md5( o );
}

void csmi_sb_set_rsvd( csmi_sb_handle_t handle, unsigned char yesno )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	o->flags.bit.rsvd = yesno;
  	redo_md5( o );
}

// Print out an ascii representation
void csmi_sb_print( csmi_sb_handle_t handle, int longp )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;

  	printf( "%-15s: 0x%08x\n", "magic", o->magic );
  	printf( "%-15s: %d\n",     "size",  o->size );
  	printf( "%-15s: %s\n",     "md5",   csmi_sb_md5_string( o->md5 ) );
  	printf( "%-15s: valid=%d commit=%d, active=%d\n", "flags",
	  	o->flags.bit.valid, o->flags.bit.commit, o->flags.bit.active );
  	printf( "%-15s: %s\n",     "root",  (o->root ? o->root : "NULL" ) );
  	printf( "%-15s: %d\n",     "#files", o->files );
  	printf( "%-15s: %d\n",     "#mtdparts", o->mtdparts );

  	{
    	char *k = csmi_sb_kernel( handle );
    	printf( "%-15s: %s\n",   "kernel", (k ? k : "NULL" ) );
    	k = csmi_sb_rootfs( handle );
    	printf( "%-15s: %s\n",   "rootfs", (k ? k : "NULL" ) );
  	}

  	if ( o->mtdparts ) {
    	char *k = csmi_sb_mtdparts( handle );
    	printf( "%-15s: %s\n",   "mtdparts", (k ? k : "NULL" ) );
  	}

  	if ( longp == 0 ) 
    	return;

  	if ( o->files ) {
    	printf( "FILES:\n" );
    	csmi_sb_file_handle_t f = csmi_sb_first_file( handle );
    	do {
      		printf( "name=%s\n", csmi_sb_file_name( f ) );
      		printf( "%-15s: %s\n",     "md5",   csmi_sb_md5_string( ((csmi_sb_file_s *)f)->md5 ) );
      		printf( "  %-13s: %d\n",     "size", csmi_sb_file_size( f ) );
      		printf( "  %-13s: 0x%08x\n", "version", csmi_sb_file_version( f ) );
      		printf( "  %-13s: %s\n",     "what", csmi_sb_file_is_str( csmi_sb_file_is( f ) ) );
      		printf( "  %-13s: %s\n",     "type", csmi_sb_file_type( f ) );
      		printf( "  %-13s: 0x%08x\n", "flags", csmi_sb_file_flags( f ) );
      		printf( "  %-13s: %s\n",     "mount", csmi_sb_file_mountpoint( f ) );
      		printf( "  %-13s: %s\n",     "mopts", csmi_sb_file_mountopts( f ) );
      		f = csmi_sb_next_file( handle );
    	} while( f );
  	}

  	if ( o->mtdparts ) {
    	printf( "MTDPARTS:\n" );
    	csmi_sb_mtdpart_handle_t f = csmi_sb_first_mtdpart( handle );
    	do {
      		printf( "name=%s\n", csmi_sb_mtdpart_name( f ) );
      		printf( "%-15s: %s\n",     "md5",   csmi_sb_md5_string( ((csmi_sb_mtdpart_s *)f)->md5 ) );
      		printf( "  %-13s: %d\n",     "filesize", csmi_sb_mtdpart_filesize( f ) );
      		printf( "  %-13s: %s\n",     "size", csmi_sb_mtdpart_size( f ) );
      		printf( "  %-13s: %s\n",     "offset", csmi_sb_mtdpart_offset( f ) );
      		printf( "  %-13s: %s\n",     "what", csmi_sb_file_is_str( csmi_sb_mtdpart_is( f ) ) );
      		printf( "  %-13s: %s\n",     "type", csmi_sb_mtdpart_type( f ) );
      		printf( "  %-13s: 0x%08x\n", "flags", csmi_sb_mtdpart_flags( f ) );
      		printf( "  %-13s: %s\n",     "mount", csmi_sb_mtdpart_mountpoint( f ) );
      		printf( "  %-13s: %s\n",     "mopts", csmi_sb_mtdpart_mountopts( f ) );
      		f = csmi_sb_next_mtdpart( handle );
    	} while( f );
  	}
}

// sb root access
void csmi_sb_set_root( csmi_sb_handle_t handle, char *root )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return;
  	strcpy( o->root, root );
  	redo_md5( o );
}

unsigned int csmi_sb_size( csmi_sb_handle_t handle ) 
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return( o->size );
}

// Obtain specific strings that u-boot needs
char *csmi_sb_kernel( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );

  	char *root = o->root;
  	char *basename = NULL;

  	csmi_sb_file_s *f = (csmi_sb_file_s *) csmi_sb_first_file( handle );
  	if ( f ) {
    	do {
      		if ( f->what == CSMI_FILE_IS_KERNEL ) {
				basename = f->name;
				break;
      		}
      		f = (csmi_sb_file_s *) csmi_sb_next_file( handle );
    	} while( f != NULL );
  	}

  	if ( basename == NULL ) {
    	// try mtdparts
    	csmi_sb_mtdpart_s *m = (csmi_sb_mtdpart_s *) csmi_sb_first_mtdpart( handle );
    	if ( m ) {
      		do {
				if ( m->what == CSMI_FILE_IS_KERNEL ) {
	  				basename = m->offset;
	  				break;
				}
				m = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
      		} while( m != NULL );

      		if ( basename != NULL )
				return( basename );
    	}
  	}

  	if ( root == NULL ) return( NULL );
  	if ( basename == NULL ) return( NULL );

  	char *fullpath = (char *)malloc( strlen(root) + strlen(basename) + 2);
  	//  add_managed( NULL, fullpath );
  	add_managed( fullpath, o );
  	sprintf( fullpath, "%s/%s", root, basename );
  	return( fullpath );
}

char *csmi_sb_rootfs( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );

  	char *root = o->root;
  	char *basename = NULL;

  	csmi_sb_file_s *f = (csmi_sb_file_s *) csmi_sb_first_file( handle );
  	if ( f ) {
    	do {
      		if ( f->what == CSMI_FILE_IS_ROOTFS ) {
				basename = f->name;
				break;
      		}
      		f = (csmi_sb_file_s *) csmi_sb_next_file( handle );
    	} while( f != NULL );
  	}

  	if ( basename == NULL ) {
    	// try mtdparts
    	csmi_sb_mtdpart_s *m = (csmi_sb_mtdpart_s *) csmi_sb_first_mtdpart( handle );
    	if ( m ) {
      		do {
				if ( m->what == CSMI_FILE_IS_ROOTFS ) {
	  				basename = m->offset;
	  				break;
				}
				m = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
      		} while( m != NULL );

      		if ( basename != NULL )
				return( basename );
    	}
  	}

  	if ( root == NULL ) return( NULL );
  	if ( basename == NULL ) return( NULL );

  	char *fullpath = (char *)malloc( strlen(root) + strlen(basename) + 2);
  	//  add_managed( NULL, fullpath );
  	add_managed( fullpath, o );
  	sprintf( fullpath, "%s/%s", root, basename );
  	return( fullpath );
}

char *csmi_sb_mtdparts( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );

  	char **parts = (char **) malloc( sizeof(char *) * o->mtdparts );
  	int ii = 0;

  	csmi_sb_mtdpart_s *f = (csmi_sb_mtdpart_s *) csmi_sb_first_mtdpart( handle );
  	while( f != NULL ) {
    	if ( f->size == NULL || f->name == NULL ) {
      		f = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
      		continue;
    	}
    	if ( MTDPART_FLAG_DONT_EXPORT(f->flags) ) {
      		f = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
      		continue;
    	}
    	if ( f->offset == NULL ) {
      		int len = strlen( f->size ) + strlen( f->name ) + 3;
      		parts[ii] = (char *) malloc( len );
      		if ( parts[ii] == NULL ) {
				f = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
				continue;
      		}
      		sprintf( parts[ii], "%s(%s)", f->size, f->name );
    	}
    	else {
      		int len = strlen( f->size ) + strlen( f->offset ) + strlen( f->name ) + 4;
      		parts[ii] = (char *) malloc( len );
      		if ( parts[ii] == NULL ) {
				f = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
				continue;
      		}
      		sprintf( parts[ii], "%s@%s(%s)", f->size, f->offset, f->name );
    	}
    	f = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( handle );
    	ii += 1;
  	}

  	int jj = 0;
  	int len = 0;
  	for( jj=0; jj<ii; jj++ ) {
    	len += strlen( parts[jj] );
  	}
  	char *mtdparts = (char *) malloc( len + ii );
  	if ( mtdparts == NULL ) return( NULL );
  	//  add_managed( NULL, mtdparts );
  	add_managed( mtdparts, o );

  	mtdparts[0] = '\0';
  	for( jj=0; jj<ii; jj++ ) {
    	strcat( mtdparts, parts[jj] );
    	if ( jj < (ii-1) ) strcat( mtdparts, "," );
  	}

  	for( jj=0; jj<ii; jj++ ) {
    	free( parts[jj] );
  	}
  	free( parts );

  	return( mtdparts );
}

char *csmi_sb_root( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->root;
}

csmi_sb_file_handle_t csmi_sb_kernel_file_handle( csmi_sb_handle_t handle ) 
{
  	csmi_sb_file_handle_t f = csmi_sb_first_file( handle );
  	while( f ) {
    	if ( csmi_sb_file_is(f) == CSMI_FILE_IS_KERNEL )
      		return f;
    	f = csmi_sb_next_file( handle );
  	}
  	return NULL;
}

csmi_sb_file_handle_t csmi_sb_rootfs_file_handle( csmi_sb_handle_t handle )
{
  	csmi_sb_file_handle_t f = csmi_sb_first_file( handle );
  	while( f ) {
    	if ( csmi_sb_file_is(f) == CSMI_FILE_IS_ROOTFS )
      		return f;
    	f = csmi_sb_next_file( handle );
  	}
  	return NULL;
}

csmi_sb_file_handle_t csmi_sb_named_file_handle( csmi_sb_handle_t handle, char *name )
{
  	csmi_sb_file_handle_t f = csmi_sb_first_file( handle );
  	while( f ) {
    	if ( strcmp( csmi_sb_file_name( f ), name ) == 0 ) 
      		return f;
    	f = csmi_sb_next_file( handle );
  	}
  	return NULL;
}

csmi_sb_mtdpart_handle_t csmi_sb_kernel_mtdpart_handle( csmi_sb_handle_t handle ) 
{
  	csmi_sb_mtdpart_handle_t f = csmi_sb_first_mtdpart( handle );
  	while( f ) {
    	if ( csmi_sb_mtdpart_is(f) == CSMI_FILE_IS_KERNEL )
      		return f;
    	f = csmi_sb_next_mtdpart( handle );
  	}
  	return NULL;
}

csmi_sb_mtdpart_handle_t csmi_sb_rootfs_mtdpart_handle( csmi_sb_handle_t handle )
{
  	csmi_sb_mtdpart_handle_t f = csmi_sb_first_mtdpart( handle );
  	while( f ) {
    	if ( csmi_sb_mtdpart_is(f) == CSMI_FILE_IS_ROOTFS )
      		return f;
    	f = csmi_sb_next_mtdpart( handle );
  	}
 	return NULL;
}

csmi_sb_mtdpart_handle_t csmi_sb_named_mtdpart_handle( csmi_sb_handle_t handle, char *name )
{
  	csmi_sb_mtdpart_handle_t f = csmi_sb_first_mtdpart( handle );
  	while( f ) {
    	if ( strcmp( csmi_sb_mtdpart_name( f ), name ) == 0 ) 
      		return f;
    	f = csmi_sb_next_mtdpart( handle );
  	}
  	return NULL;
}

// Individual mtdpart and file access
csmi_sb_file_handle_t csmi_sb_first_file( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	last_file = NULL;
  	if ( o == NULL ) return( NULL );
  	if ( o->files_p == NULL ) return( NULL );

  	csmi_sb_file_s *f = (csmi_sb_file_s *)((unsigned)o + (unsigned)o->files_p);
  	last_file = f;
  	return( (csmi_sb_file_handle_t) f );
}

csmi_sb_file_handle_t csmi_sb_next_file( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );
  	if ( last_file == NULL ) return( NULL );
  	if ( last_file->next == NULL ) return( NULL );
  	csmi_sb_file_s *f = (csmi_sb_file_s *)((unsigned)o + (unsigned)last_file->next);
  	last_file = f;
  	return( (csmi_sb_file_handle_t) f );
}

csmi_sb_mtdpart_handle_t csmi_sb_first_mtdpart( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	last_mtdpart = NULL;
  	if ( o == NULL ) return( NULL );
  	if ( o->mtdparts_p == NULL ) return( NULL );

  	csmi_sb_mtdpart_s *f = (csmi_sb_mtdpart_s *)( (unsigned char *)o + 
						(unsigned int)o->mtdparts_p);
  	last_mtdpart = f;
  	return( (csmi_sb_mtdpart_handle_t) f );
}

csmi_sb_mtdpart_handle_t csmi_sb_next_mtdpart( csmi_sb_handle_t handle )
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );
  	if ( last_mtdpart == NULL ) return( NULL );
  	if ( last_mtdpart->next == NULL ) return( NULL );
  	csmi_sb_mtdpart_s *f = (csmi_sb_mtdpart_s *)( (unsigned char *)o + 
						(unsigned int)last_mtdpart->next);
  	last_mtdpart = f;
  	return( (csmi_sb_mtdpart_handle_t) f );
}

// File access methods
char *csmi_sb_file_name( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->name;
}

unsigned char *csmi_sb_file_md5( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->md5;
}

unsigned int csmi_sb_file_version( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return o->version;
}

unsigned int csmi_sb_file_flags( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return o->flags;
}

unsigned int csmi_sb_file_size( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return o->size;
}

csmi_file_is_e csmi_sb_file_is( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( CSMI_FILE_IS_OTHER );
  	return o->what;
}

char *csmi_sb_file_type( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->type;
}

char *csmi_sb_file_mountpoint( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->mountpoint;
}

char *csmi_sb_file_mountopts( csmi_sb_file_handle_t handle )
{
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->mountopts;
}

// Mtdpart access methods
char *csmi_sb_mtdpart_name( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->name;
}

char *csmi_sb_mtdpart_size( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->size;
}

unsigned int csmi_sb_mtdpart_filesize( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return o->filesize;
}

char *csmi_sb_mtdpart_offset( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->offset;
}

csmi_file_is_e csmi_sb_mtdpart_is( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( CSMI_FILE_IS_OTHER );
  	return o->what;
}

char *csmi_sb_mtdpart_type( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->type;
}

unsigned int csmi_sb_mtdpart_flags( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( 0 );
  	return o->flags;
}

char *csmi_sb_mtdpart_mountpoint( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->mountpoint;
}

char *csmi_sb_mtdpart_mountopts( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->mountopts;
}

unsigned char *csmi_sb_mtdpart_md5( csmi_sb_mtdpart_handle_t handle )
{
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return( NULL );
  	return o->md5;
}

// Routines for adding new files and mtdparts
csmi_sb_handle_t csmi_sb_add_file( 
	csmi_sb_handle_t handle,
  	char *name, 				// file basename
  	char *path, unsigned char file_md5[],	// full path, or md5 pre-calculated
  	unsigned int version,			// version number
  	csmi_file_is_e file_is,		// what is it?
  	char *type,				// type?
  	unsigned int flags,			// flags
  	char *mountpoint, char *mountopts 	// optional mount options
  	)
{
  /**
     get size
     realloc += sizeof file_t  (read_mem w/ dest won't work!)
     cast new mem, bzero, and write info (and file md5)
     find last file, modify next (next=size)
     change size
     change num_files (or get rid of)
     redo global md5 (and get rid of pack ...)
     patch up managed_elements with new pointer, if different
  **/
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );

  	csmi_sb_handle_t new = (csmi_sb_handle_t) malloc( o->size + sizeof( csmi_sb_file_s ) );

  	if ( new == NULL ) {
    	printf( "Failed to realloc memory for new file!\n" );
    	return( NULL );
  	}

  	memset( new, 0, o->size + sizeof( csmi_sb_file_s ) );
  	memcpy( new, handle, o->size );
  	free( handle );

  	o = (csmi_sb_s *)new;

  	csmi_sb_file_s *f = (csmi_sb_file_s *)(new+o->size);
  	memset( f, 0, sizeof( csmi_sb_file_s ) );

  	if ( name )
    	strcpy( f->name, name );
  	f->version = version;
  	f->what = file_is;
  	f->flags = flags;
  	if ( type )
    	strcpy( f->type, type );
  	if ( mountpoint )
    	strcpy( f->mountpoint, mountpoint );
  	if ( mountopts )
    	strcpy( f->mountopts, mountopts );

  	if ( path == NULL ) {
    	memcpy( f->md5, file_md5, 16 );
  	}
  	else {
#ifdef HAVE_FILE_MD5_SUPPORT
    	// Calculate md5 based on file ...
    	int fd = open( path, O_RDONLY );
    	if ( fd < 0 ) {
      		printf( "Cannot access %s to calculate MD5!\n", path );
      		return( NULL );
    	}
    	unsigned char *buf = (unsigned char *) malloc( 1024 );
    	if ( buf == NULL ) {
      		printf( "Failed to allocate buffer for file-based md5 gen!\n" );
      		return( NULL );
    	}
    	f->size = md5_file( fd, 1024, buf, f->md5 );
    	free( buf );
    	close( fd );
#endif    
  	}

  	// patch up linked list
  	//
  	csmi_sb_file_s *F = (csmi_sb_file_s *) csmi_sb_first_file( new );
  	if ( F == NULL ) {
    	o->files_p = (unsigned char *)o->size;
  	}
  	else {
    	do {
      		if ( F->next == 0 ) {
				F->next = (unsigned char *)o->size;
				break;
      		}
      		F = (csmi_sb_file_s *) csmi_sb_next_file( new );
    	} while( F != NULL );
  	}

  	o->size += sizeof( csmi_sb_file_s );
  	o->files += 1;

  	// calculate global md5
  	//
  	redo_md5( o );

  	// patched managed_elements
  	//
  	patch_managed( handle, new );

  	return( new );
}

csmi_sb_handle_t csmi_sb_add_mtdpart( 
	csmi_sb_handle_t handle,
  	char *name, 				// partition name
  	char *path, unsigned char file_md5[],	// full path, or md5 pre-calculated
  	char *size,				// '2M', '256K', ...
  	char *offset,				// '0x1080000', ...
  	csmi_file_is_e mtdpart_is,		// what is it?
  	char *type,				// type?
  	unsigned int flags,			// flags
  	char *mountpoint, char *mountopts 	// optional mount options
  	)
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	if ( o == NULL ) return( NULL );

  	csmi_sb_handle_t new = (csmi_sb_handle_t) malloc( o->size + sizeof( csmi_sb_mtdpart_s ) );

  	if ( new == NULL ) {
    	printf( "Failed to realloc memory for new mtdpart!\n" );
    	return( NULL );
  	}

  	memset( new, 0, o->size + sizeof( csmi_sb_mtdpart_s ) );
  	memcpy( new, handle, o->size );
  	free( handle );

  	o = (csmi_sb_s *)new;

  	csmi_sb_mtdpart_s *f = (csmi_sb_mtdpart_s *)(new+o->size);
  	memset( f, 0, sizeof( csmi_sb_mtdpart_s ) );

  	if ( name )
    	strcpy( f->name, name );
  	if ( size )
    	strcpy( f->size, size );
  	if ( offset )
    	strcpy( f->offset, offset );
  	if ( type )
    	strcpy( f->type, type );
  	if ( mountpoint )
    	strcpy( f->mountpoint, mountpoint );
  	if ( mountopts )
    	strcpy( f->mountopts, mountopts );
  	f->what = mtdpart_is;
  	f->flags = flags;

  	if ( path == NULL ) {
    	memcpy( f->md5, file_md5, 16 );
  	}
  	else {
#ifdef HAVE_FILE_MD5_SUPPORT
    	// Calculate md5 based on file ...
    	int fd = open( path, O_RDONLY );
    	if ( fd < 0 ) {
      		printf( "Cannot access %s to calculate MD5!\n", path );
      		return( NULL );
    	}
    	unsigned char *buf = (unsigned char *) malloc( 1024 );
    	if ( buf == NULL ) {
      		printf( "Failed to allocate buffer for file-based md5 gen!\n" );
      		return( NULL );
    	}
    	f->filesize = md5_file( fd, 1024, buf, f->md5 );
    	free( buf );
    	close( fd );
#endif    
  	}

  	// patch up linked list
  	//
  	csmi_sb_mtdpart_s *F = (csmi_sb_mtdpart_s *) csmi_sb_first_mtdpart( new );

  	if ( F == NULL ) {
    	o->mtdparts_p = (unsigned char *)o->size;
  	}
  	else {
    	do {
      		if ( F->next == 0 ) {
				F->next = (unsigned char *)o->size;
				break;
      		}
      		F = (csmi_sb_mtdpart_s *) csmi_sb_next_mtdpart( new );
    	} while( F != NULL );
  	}

  	o->size += sizeof( csmi_sb_mtdpart_s );
  	o->mtdparts += 1;

  	// calculate global md5
  	//
  	redo_md5( o );

  	// patched managed_elements
  	//
  	patch_managed( handle, new );

  	return( new );
}

// Check the memory at address for a good
// md5 signature.
//
int csmi_sb_check_md5_mem( unsigned char *address, 
			   unsigned int size, 
			   unsigned char expected_md5[16] )
{
  	unsigned char calculated_md5[16];

  	md5( address, size, calculated_md5 );
  	if ( memcmp( (void *)expected_md5, (void *)calculated_md5, 16 ) != 0 ) {
    	printf( "MD5 signature at %08x for %d bytes does not match!\n",
	    	(unsigned int) address, size );
    	printf( "  Expected:   %s\n", csmi_sb_md5_string( expected_md5 ) );
    	printf( "  Calculated: %s\n", csmi_sb_md5_string( calculated_md5 ) );
    	return( -1 );
  	}

  	return( 1 );
}

// Return a string rep. of an md5 value
char *csmi_sb_md5_string( unsigned char arr[16] )
{
  	static char str[33];
  	int i;
  	str[0] = '\0';
  	for(i=0; i<16; i++) {
    	sprintf( str + (i*2), "%02x", arr[i] );
  	}
  	return( str );
}

// Print the what enum as a string
//
char *csmi_sb_file_is_str( csmi_file_is_e what )
{
  	switch( what ) {
  	case CSMI_FILE_IS_KERNEL: return "CSMI_FILE_IS_KERNEL"; break;
  	case CSMI_FILE_IS_ROOTFS: return "CSMI_FILE_IS_ROOTFS"; break;
  	case CSMI_FILE_IS_FS:     return "CSMI_FILE_IS_FS"; break;
  	case CSMI_FILE_IS_DATA:   return "CSMI_FILE_IS_DATA"; break;
  	case CSMI_FILE_IS_OTHER:  return "CSMI_FILE_IS_OTHER"; break;
  	default: return "Invalid ENUM value!";
  	}
}

void csmi_sb_new_md5( csmi_sb_handle_t handle ) 
{
  	csmi_sb_s *o = (csmi_sb_s *) handle;
  	redo_md5( o );
}

/**************************************************************************/

static void redo_md5( csmi_sb_s *o )
{
  	unsigned int md5offset = ((sizeof( unsigned int  ) * 2) +
			    (sizeof( unsigned char ) * 16) );
  	md5( (unsigned char *)o + md5offset, o->size - md5offset, o->md5 );
}

static void add_managed( void *c, void *p )
{
  	if ( managed_elements == NULL ) {
    	managed_elements = ( managed_element_t * ) calloc( 64, sizeof ( managed_element_t ) );
  	}
  	if ( num_managed_elements >= 64 ) {
    	printf( "Exceeded number of managed elements!\n" );
    	return;
  	}
  	managed_elements[num_managed_elements].child = c;
  	managed_elements[num_managed_elements].parent = p;
#ifdef DEBUG_MANAGED
  	int i = num_managed_elements;
  	printf( "Adding: %-2d: child=%08x parent=%08x\n", i,
	  	(unsigned)( (unsigned)managed_elements[i].child ? (unsigned)managed_elements[i].child : 0xdeaddead ),
	  	(unsigned)( (unsigned)managed_elements[i].parent ? (unsigned)managed_elements[i].parent : 0xdeaddead ) );
#endif
  	num_managed_elements += 1;
}

static void free_managed( void *p ) 
{
  	int i;
  	if ( managed_elements ) {
    	for( i=0; i<num_managed_elements; i++ ) {
      		if ( managed_elements[i].parent ) {
				if ( managed_elements[i].parent == p ) {
#ifdef DEBUG_MANAGED
	  				printf( "Deleting: %-2d: child=%08x parent=%08x\n", i,
		  				(unsigned)( (unsigned)managed_elements[i].child ? (unsigned)managed_elements[i].child : 0xdeaddead ),
		  				(unsigned)( (unsigned)managed_elements[i].parent ? (unsigned)managed_elements[i].parent : 0xdeaddead ) );
#endif
	  				if ( managed_elements[i].child != NULL ) {
	    				free( managed_elements[i].child );
	    				managed_elements[i].child = NULL;
	    				managed_elements[i].parent = NULL;
	  				}
	  				else {
	    				free( managed_elements[i].parent );
	    				managed_elements[i].parent = NULL;
	  				}
				}
      		}
    	}
  	}
}

static void patch_managed( void *old, void *new )
{
  	int i;
  	if ( managed_elements ) {
    	for( i=0; i<num_managed_elements; i++ ) {
      		if ( managed_elements[i].parent ) {
				if ( managed_elements[i].parent == old ) {
	  				managed_elements[i].parent = new;
				}
      		}
    	}
  	}
}

void csmi_sb_check_managed( void ) 
{
  	int i;
  	if ( managed_elements ) {
    	for( i=0; i<num_managed_elements; i++ ) {
      		if ( ((unsigned) managed_elements[i].child == 0) &&
	   		((unsigned) managed_elements[i].parent == 0) ) {
				continue;
      		}
      		printf( "%-2d: child=%08x parent=%08x\n", i,
	      		(unsigned)( (unsigned)managed_elements[i].child ? (unsigned)managed_elements[i].child : 0xdeaddead ),
	      		(unsigned)( (unsigned)managed_elements[i].parent ? (unsigned)managed_elements[i].parent : 0xdeaddead ) );
    	}
  	}	
  	else {
    	printf( "No managed elements.\n" );
  	}
}

void csmi_sb_file_set_md5( csmi_sb_file_handle_t handle, unsigned char new_md5[16] ) {
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return;
  	memcpy( o->md5, new_md5, 16 );
}

void csmi_sb_file_set_size( csmi_sb_file_handle_t handle, unsigned int new_size ) {
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return;
  	o->size = new_size;
}

void csmi_sb_file_set_version( csmi_sb_file_handle_t handle, unsigned int new_version ) {
  	csmi_sb_file_s *o = (csmi_sb_file_s *) handle;
  	if ( o == NULL ) return;
  	o->version = new_version;
}

void csmi_sb_mtdpart_set_md5( csmi_sb_mtdpart_handle_t handle, unsigned char new_md5[16] ) {
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return;
  	memcpy( o->md5, new_md5, 16 );
}

void csmi_sb_mtdpart_set_filesize( csmi_sb_mtdpart_handle_t handle, unsigned int new_size ) {
  	csmi_sb_mtdpart_s *o = (csmi_sb_mtdpart_s *) handle;
  	if ( o == NULL ) return;
  	o->filesize = new_size;
}

