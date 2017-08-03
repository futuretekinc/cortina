#ifndef _CSMI_STRUCT_H_
#define _CSMI_STRUCT_H_

#define SHORT_STRING_SZ 32
#define LONG_STRING_SZ  64
typedef char short_string_t[SHORT_STRING_SZ];
typedef char long_string_t[LONG_STRING_SZ];

#define CS_SB_MAGIC	0x44113761

typedef struct {
  unsigned int		magic;		// magic/version number
  unsigned int		size;		// total size of sb, incl. magic and size
  unsigned char		md5[16];	// covers everything after this member
  union {				// state bits
    struct {
      unsigned int valid  :  1;
      unsigned int commit :  1;
      unsigned int active :  1;
      unsigned int rsvd   : 29;
    } bit;
    unsigned int bits;
  } flags;
  long_string_t		root;		// path to files in this bank
  unsigned char		mtdparts;	// number of mtdparts assoc. w/ this bank
  unsigned char		*mtdparts_p;	// pointer to mtdparts list
  unsigned char		files;		// number of files in this bank
  unsigned char		*files_p;	// pointer to files list
} __attribute__((packed)) csmi_sb_s;

typedef struct {
  short_string_t	name;		// partition name
  unsigned char		md5[16];	// md5 for file
  unsigned int		filesize;	// size of file
  short_string_t	size;		// '2M', '256K', etc.
  short_string_t	offset;		// '0x3c000000', etc.
  csmi_file_is_e	what;		// kernel, rootfs, fs, etc
  short_string_t	type;		// raw, jffs2, squashfs, etc.
  unsigned int 		flags;		// DONT_EXPORT, etc
  long_string_t		mountpoint;	// mount point, if mountable
  short_string_t	mountopts;	// mount options, if mountable
  unsigned char		*next;		// pointer to next mtdpart
} __attribute__((packed)) csmi_sb_mtdpart_s;

typedef struct {
  short_string_t	name;		// filename
  unsigned char		md5[16];	// md5 for file
  unsigned int		version;	// version stamp;
  unsigned int		size;		// size of file
  csmi_file_is_e	what;		// kernel, rootfs, fs, etc
  short_string_t	type;		// raw, jffs2, squashfs, etc.
  unsigned int		flags;		// future use
  long_string_t		mountpoint;	// mount point, if mountable
  short_string_t	mountopts;	// mount options, if mountable
  unsigned char		*next;		// pointer to next file
} __attribute__((packed)) csmi_sb_file_s;

#endif
