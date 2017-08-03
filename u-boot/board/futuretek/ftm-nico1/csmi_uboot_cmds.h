#ifndef _CSMI_UBOOT_CMDS_H_
#define _CSMI_UBOOT_CMDS_H_

// Typedefs for the callbacks we need
//
typedef unsigned long (*strtoul_cb_t)(const char *in, char **rpt, unsigned int base);
typedef int (*setenv_cb_t)(char *name, char *val);
typedef char *(*getenv_cb_t)(char *name);

typedef void *(*malloc_cb_t)(int len);
typedef void (*free_cb_t)(void *ptr);

typedef int (*read_flash_cb_t)(void *buffer, unsigned long address, int len);
typedef int (*write_flash_cb_t)(void *buffer, unsigned long address, int len);

typedef int (*cp_flash_ram_cb_t)(unsigned long flash_addr, unsigned long ram_addr, int len );
typedef int (*cp_jffs2_ram_cb_t)(char *filename, unsigned long ram_addr );

// Library handle
//
typedef struct {
  strtoul_cb_t		strtoul;
  setenv_cb_t		setenv;
  getenv_cb_t		getenv;
  malloc_cb_t		malloc;
  free_cb_t		free;
  read_flash_cb_t	read_flash;
  write_flash_cb_t	write_flash;
  cp_flash_ram_cb_t	cp_flash_ram;
  cp_jffs2_ram_cb_t	cp_jffs2_ram;
  unsigned long		flash_base;
} csmi_uboot_cmds_struct_t;

// Opaque type for callers
//
typedef void *csmi_uboot_cmds_handle_t;

// Apis
//
extern csmi_uboot_cmds_handle_t csmi_uboot_cmds_initialize
( unsigned long         flash_base,
  strtoul_cb_t		strtoul_a,
  setenv_cb_t		setenv_a,
  getenv_cb_t		getenv_a,
  malloc_cb_t		malloc_a,
  free_cb_t		free_a,
  read_flash_cb_t	read_flash_a,
  write_flash_cb_t	write_flash_a,
  cp_flash_ram_cb_t	cp_flash_ram_a,
  cp_jffs2_ram_cb_t	cp_jffs2_ram_a );

extern int csmi_uboot_cmds_sb_display
( csmi_uboot_cmds_handle_t handle,
  unsigned long sb_address );

extern int csmi_uboot_cmds_sb_set_flags
( csmi_uboot_cmds_handle_t handle,
  unsigned long sb_address,
  unsigned long v, unsigned long c, unsigned long a );

extern int csmi_uboot_cmds_sb_process
( csmi_uboot_cmds_handle_t handle,
  int argc, char *argv[] );

#endif
