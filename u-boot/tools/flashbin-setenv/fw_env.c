/*
 * (C) Copyright 2001-2010
 * Cortina Systems Inc.
 *
 * This is a program to write the u-boot env partition into a flash.bin
 * file.  It is based on a hacked up version of fw_printenv, from
 * uboot-envtools.  The original copyright notices have been left
 * intact, and remain true.
*/
/*
 * (C) Copyright 2000-2008
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2008
 * Guennadi Liakhovetski, DENX Software Engineering, lg@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fw_env.h"

#define	CMD_GETENV	"fw_printenv"
#define	CMD_SETENV	"fw_setenv"

#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

struct envdev_s {
	char  *devname;		/* Device name */
	ulong devoff;			/* Device offset */
	ulong env_size;			/* environment size */
	ulong erase_size;		/* device erase size */
	ulong env_sectors;		/* number of environment sectors */
	uint8_t mtd_type;		/* type of the MTD device */
};

static struct envdev_s envdevices[2] =
{
	{
		.mtd_type = 0,
	}, {
		.mtd_type = 0,
	},
};
static int dev_current;

#define DEVNAME(i)    envdevices[(i)].devname
#define DEVOFFSET(i)  envdevices[(i)].devoff
#define ENVSIZE(i)    envdevices[(i)].env_size
#define DEVESIZE(i)   envdevices[(i)].erase_size
#define ENVSECTORS(i) envdevices[(i)].env_sectors
#define DEVTYPE(i)    envdevices[(i)].mtd_type

#define CONFIG_ENV_SIZE ENVSIZE(dev_current)

#define ENV_SIZE      getenvsize()

struct env_image_single {
	uint32_t	crc;	/* CRC32 over data bytes    */
	char		data[];
};

struct env_image_redundant {
	uint32_t	crc;	/* CRC32 over data bytes    */
	unsigned char	flags;	/* active or obsolete */
	char		data[];
};

enum flag_scheme {
	FLAG_NONE,
	FLAG_BOOLEAN,
	FLAG_INCREMENTAL,
};

struct environment {
	void			*image;
	uint32_t		*crc;
	unsigned char		*flags;
	char			*data;
	enum flag_scheme	flag_scheme;
};

static struct environment environment = {
	.flag_scheme = FLAG_NONE,
};

static int HaveRedundEnv = 0;


#define XMK_STR(x)	#x
#define MK_STR(x)	XMK_STR(x)

static char default_environment[] = {
#if defined(CONFIG_BOOTARGS)
	"bootargs=" CONFIG_BOOTARGS "\0"
#endif
#if defined(CONFIG_BOOTCOMMAND)
	"bootcmd=" CONFIG_BOOTCOMMAND "\0"
#endif
#if defined(CONFIG_RAMBOOTCOMMAND)
	"ramboot=" CONFIG_RAMBOOTCOMMAND "\0"
#endif
#if defined(CONFIG_NFSBOOTCOMMAND)
	"nfsboot=" CONFIG_NFSBOOTCOMMAND "\0"
#endif
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
	"bootdelay=" MK_STR (CONFIG_BOOTDELAY) "\0"
#endif
#if defined(CONFIG_BAUDRATE) && (CONFIG_BAUDRATE >= 0)
	"baudrate=" MK_STR (CONFIG_BAUDRATE) "\0"
#endif
#ifdef	CONFIG_LOADS_ECHO
	"loads_echo=" MK_STR (CONFIG_LOADS_ECHO) "\0"
#endif
#ifdef	CONFIG_ETHADDR
	"ethaddr=" MK_STR (CONFIG_ETHADDR) "\0"
#endif
#ifdef	CONFIG_ETH1ADDR
	"eth1addr=" MK_STR (CONFIG_ETH1ADDR) "\0"
#endif
#ifdef	CONFIG_ETH2ADDR
	"eth2addr=" MK_STR (CONFIG_ETH2ADDR) "\0"
#endif
#ifdef	CONFIG_ETH3ADDR
	"eth3addr=" MK_STR (CONFIG_ETH3ADDR) "\0"
#endif
#ifdef	CONFIG_ETH4ADDR
	"eth4addr=" MK_STR (CONFIG_ETH4ADDR) "\0"
#endif
#ifdef	CONFIG_ETH5ADDR
	"eth5addr=" MK_STR (CONFIG_ETH5ADDR) "\0"
#endif
#ifdef	CONFIG_ETHPRIME
	"ethprime=" CONFIG_ETHPRIME "\0"
#endif
#ifdef	CONFIG_IPADDR
	"ipaddr=" MK_STR (CONFIG_IPADDR) "\0"
#endif
#ifdef	CONFIG_SERVERIP
	"serverip=" MK_STR (CONFIG_SERVERIP) "\0"
#endif
#ifdef	CONFIG_SYS_AUTOLOAD
	"autoload=" CONFIG_SYS_AUTOLOAD "\0"
#endif
#ifdef	CONFIG_ROOTPATH
	"rootpath=" MK_STR (CONFIG_ROOTPATH) "\0"
#endif
#ifdef	CONFIG_GATEWAYIP
	"gatewayip=" MK_STR (CONFIG_GATEWAYIP) "\0"
#endif
#ifdef	CONFIG_NETMASK
	"netmask=" MK_STR (CONFIG_NETMASK) "\0"
#endif
#ifdef	CONFIG_HOSTNAME
	"hostname=" MK_STR (CONFIG_HOSTNAME) "\0"
#endif
#ifdef	CONFIG_BOOTFILE
	"bootfile=" MK_STR (CONFIG_BOOTFILE) "\0"
#endif
#ifdef	CONFIG_LOADADDR
	"loadaddr=" MK_STR (CONFIG_LOADADDR) "\0"
#endif
#ifdef	CONFIG_PREBOOT
	"preboot=" CONFIG_PREBOOT "\0"
#endif
#ifdef	CONFIG_CLOCKS_IN_MHZ
	"clocks_in_mhz=" "1" "\0"
#endif
#if defined(CONFIG_PCI_BOOTDELAY) && (CONFIG_PCI_BOOTDELAY > 0)
	"pcidelay=" MK_STR (CONFIG_PCI_BOOTDELAY) "\0"
#endif
#ifdef  CONFIG_EXTRA_ENV_SETTINGS
	CONFIG_EXTRA_ENV_SETTINGS
#endif
	"\0"		/* Termimate struct environment data with 2 NULs */
};

static int flash_io (int mode);
static char *envmatch (char * s1, char * s2);
static int env_init (void);
static int parse_config (void);

static inline uint32_t getenvsize (void)
{
	uint32_t rc = CONFIG_ENV_SIZE - sizeof (uint32_t);

	if (HaveRedundEnv)
		rc -= sizeof (char);
	return rc;
}

/*
 * Search the environment for a variable.
 * Return the value, if found, or NULL, if not found.
 */
char *fw_getenv (char *name)
{
	char *env, *nxt;

	if (env_init ())
		return NULL;

	for (env = environment.data; *env; env = nxt + 1) {
		char *val;

		for (nxt = env; *nxt; ++nxt) {
			if (nxt >= &environment.data[ENV_SIZE]) {
				fprintf (stderr, "## Error: "
					"environment not terminated\n");
				return NULL;
			}
		}
		val = envmatch (name, env);
		if (!val)
			continue;
		return val;
	}
	return NULL;
}

/*
 * Print the current definition of one, or more, or all
 * environment variables
 */
int fw_printenv (int argc, char *argv[])
{
	char *env, *nxt;
	int i, n_flag;
	int rc = 0;

	if (env_init ())
		return -1;

	if (argc == 1) {		/* Print all env variables  */
		for (env = environment.data; *env; env = nxt + 1) {
			for (nxt = env; *nxt; ++nxt) {
				if (nxt >= &environment.data[ENV_SIZE]) {
					fprintf (stderr, "## Error: "
						"environment not terminated\n");
					return -1;
				}
			}

			printf ("%s\n", env);
		}
		return 0;
	}

	if (strcmp (argv[1], "-n") == 0) {
		n_flag = 1;
		++argv;
		--argc;
		if (argc != 2) {
			fprintf (stderr, "## Error: "
				"`-n' option requires exactly one argument\n");
			return -1;
		}
	} else {
		n_flag = 0;
	}

	for (i = 1; i < argc; ++i) {	/* print single env variables   */
		char *name = argv[i];
		char *val = NULL;

		for (env = environment.data; *env; env = nxt + 1) {

			for (nxt = env; *nxt; ++nxt) {
				if (nxt >= &environment.data[ENV_SIZE]) {
					fprintf (stderr, "## Error: "
						"environment not terminated\n");
					return -1;
				}
			}
			val = envmatch (name, env);
			if (val) {
				if (!n_flag) {
					fputs (name, stdout);
					putc ('=', stdout);
				}
				puts (val);
				break;
			}
		}
		if (!val) {
			fprintf (stderr, "## Error: \"%s\" not defined\n", name);
			rc = -1;
		}
	}

	return rc;
}

/*
 * Deletes or sets environment variables. Returns -1 and sets errno error codes:
 * 0	  - OK
 * EINVAL - need at least 1 argument
 * EROFS  - certain variables ("ethaddr", "serial#") cannot be
 *	    modified or deleted
 *
 */
int fw_setenv (int argc, char *argv[])
{
	int i, len;
	char *env, *nxt;
	char *oldval = NULL;
	char *name;

	if (argc < 2) {
		errno = EINVAL;
		return -1;
	}

	if (env_init ())
		return -1;

	name = argv[1];

	/*
	 * search if variable with this name already exists
	 */
	for (nxt = env = environment.data; *env; env = nxt + 1) {
		for (nxt = env; *nxt; ++nxt) {
			if (nxt >= &environment.data[ENV_SIZE]) {
				fprintf (stderr, "## Error: "
					"environment not terminated\n");
				errno = EINVAL;
				return -1;
			}
		}
		if ((oldval = envmatch (name, env)) != NULL)
			break;
	}

	/*
	 * Delete any existing definition
	 */
	if (oldval) {
		if (*++nxt == '\0') {
			*env = '\0';
		} else {
			for (;;) {
				*env = *nxt++;
				if ((*env == '\0') && (*nxt == '\0'))
					break;
				++env;
			}
		}
		*++env = '\0';
	}

	/* Delete only ? */
	if (argc < 3)
		goto WRITE_FLASH;

	/*
	 * Append new definition at the end
	 */
	for (env = environment.data; *env || *(env + 1); ++env);
	if (env > environment.data)
		++env;
	/*
	 * Overflow when:
	 * "name" + "=" + "val" +"\0\0"  > CONFIG_ENV_SIZE - (env-environment)
	 */
	len = strlen (name) + 2;
	/* add '=' for first arg, ' ' for all others */
	for (i = 2; i < argc; ++i) {
		len += strlen (argv[i]) + 1;
	}
	if (len > (&environment.data[ENV_SIZE] - env)) {
		fprintf (stderr,
			"Error: environment overflow, \"%s\" deleted\n",
			name);
		return -1;
	}
	while ((*env = *name++) != '\0')
		env++;
	for (i = 2; i < argc; ++i) {
		char *val = argv[i];

		*env = (i == 2) ? '=' : ' ';
		while ((*++env = *val++) != '\0');
	}

	/* end is marked with double '\0' */
	*++env = '\0';

  WRITE_FLASH:

	/*
	 * Update CRC
	 */
	*environment.crc = uboot_crc32 (0, (uint8_t *) environment.data, ENV_SIZE);

	/* write environment back to flash */
	if (flash_io (O_RDWR)) {
		fprintf (stderr, "Error: can't write fw_env to flash\n");
		return -1;
	}

	return 0;
}

/*
 * Read data from flash at an offset into a provided buffer. On NAND it skips
 * bad blocks but makes sure it stays within ENVSECTORS (dev) starting from
 * the DEVOFFSET (dev) block. On NOR the loop is only run once.
 */
static int flash_read_buf (int dev, int fd, void *buf, size_t count,
			   off_t offset, uint8_t mtd_type)
{
  int rc = 0;
  lseek (fd, offset, SEEK_SET);
  rc = read (fd, buf, count);
  if (rc != count) {
    fprintf (stderr, "Read error on %s: %s\n",
	     DEVNAME (dev), strerror (errno));
    return -1;
  }
  return rc;
}

/*
 * Write count bytes at offset, but stay within ENVSETCORS (dev) sectors of
 * DEVOFFSET (dev). Similar to the read case above, on NOR we erase and write
 * the whole data at once.
 */
static int flash_write_buf (int dev, int fd, void *buf, size_t count,
			    off_t offset, uint8_t mtd_type)
{
  if (lseek (fd, offset, SEEK_SET) == -1) {
    fprintf (stderr,
	     "Seek error on %s: %s\n",
	     DEVNAME (dev), strerror (errno));
    return -1;
  }
  if (write (fd, buf, count) != count) {
    fprintf (stderr, "Write error on %s: %s\n",
	     DEVNAME (dev), strerror (errno));
    return -1;
  }
  return count;
}


static int flash_write (int fd_current, int fd_target, int dev_target)
{
	int rc;

	rc = flash_write_buf (dev_target, fd_target, environment.image,
			      CONFIG_ENV_SIZE, DEVOFFSET (dev_target),
			      DEVTYPE(dev_target));
	if (rc < 0)
		return rc;

	return 0;
}

static int flash_read (int fd)
{
  int rc = 0;

	DEVTYPE(dev_current) = 0;

	rc = flash_read_buf (dev_current, fd, environment.image, CONFIG_ENV_SIZE,
			     DEVOFFSET (dev_current), 0);

	return (rc != CONFIG_ENV_SIZE) ? -1 : 0;
}

static int flash_io (int mode)
{
	int fd_current, fd_target, rc, dev_target;

	/* dev_current: fd_current, erase_current */
	fd_current = open (DEVNAME (dev_current), mode);
	if (fd_current < 0) {
		fprintf (stderr,
			 "Can't open %s: %s\n",
			 DEVNAME (dev_current), strerror (errno));
		return -1;
	}

	if (mode == O_RDWR) {
		if (HaveRedundEnv) {
			/* switch to next partition for writing */
			dev_target = !dev_current;
			/* dev_target: fd_target, erase_target */
			fd_target = open (DEVNAME (dev_target), mode);
			if (fd_target < 0) {
				fprintf (stderr,
					 "Can't open %s: %s\n",
					 DEVNAME (dev_target),
					 strerror (errno));
				rc = -1;
				goto exit;
			}
		} else {
			dev_target = dev_current;
			fd_target = fd_current;
		}

		rc = flash_write (fd_current, fd_target, dev_target);

		if (HaveRedundEnv) {
			if (close (fd_target)) {
				fprintf (stderr,
					"I/O error on %s: %s\n",
					DEVNAME (dev_target),
					strerror (errno));
				rc = -1;
			}
		}
	} else {
		rc = flash_read (fd_current);
	}

exit:
	if (close (fd_current)) {
		fprintf (stderr,
			 "I/O error on %s: %s\n",
			 DEVNAME (dev_current), strerror (errno));
		return -1;
	}

	return rc;
}

/*
 * s1 is either a simple 'name', or a 'name=value' pair.
 * s2 is a 'name=value' pair.
 * If the names match, return the value of s2, else NULL.
 */

static char *envmatch (char * s1, char * s2)
{

	while (*s1 == *s2++)
		if (*s1++ == '=')
			return s2;
	if (*s1 == '\0' && *(s2 - 1) == '=')
		return s2;
	return NULL;
}

/*
 * Prevent confusion if running from erased flash memory
 */
static int env_init (void)
{
	int crc0, crc0_ok;
	void *addr0;

	struct env_image_single *single;
	struct env_image_redundant *redundant;

	if (parse_config ())		/* should fill envdevices */
		return -1;

	addr0 = calloc (1, CONFIG_ENV_SIZE);
	if (addr0 == NULL) {
		fprintf (stderr,
			"Not enough memory for environment (%ld bytes)\n",
			CONFIG_ENV_SIZE);
		return -1;
	}

	/* read environment from FLASH to local buffer */
	environment.image = addr0;

	if (HaveRedundEnv) {
		redundant = addr0;
		environment.crc		= &redundant->crc;
		environment.flags	= &redundant->flags;
		environment.data	= redundant->data;
	} else {
		single = addr0;
		environment.crc		= &single->crc;
		environment.flags	= NULL;
		environment.data	= single->data;
	}

	dev_current = 0;
	if (flash_io (O_RDONLY))
		return -1;

	crc0 = uboot_crc32 (0, (uint8_t *) environment.data, ENV_SIZE);
	crc0_ok = (crc0 == *environment.crc);
	if (!HaveRedundEnv) {
		if (!crc0_ok) {
			fprintf (stderr,
				"Warning: Bad CRC, using default environment\n");
			memcpy(environment.data, default_environment, sizeof default_environment);
		}
	}
	return 0;
}


static int parse_config ()
{
	struct stat st;

	if (stat (DEVNAME (0), &st)) {
		fprintf (stderr,
			"Cannot access flash.bin file %s: %s\n",
			DEVNAME (0), strerror (errno));
		return -1;
	}
	return 0;
}

void fw_setconfig( char *name, 
		   unsigned int devoffset,
		   unsigned int envsize,
		   unsigned int devesize,
		   unsigned int sectors )
{
  DEVNAME(0)    = strdup( name );
  DEVOFFSET(0)  = devoffset;
  ENVSIZE(0)    = envsize;
  DEVESIZE(0)   = devesize;
  ENVSECTORS(0) = sectors;
  HaveRedundEnv = 0;
}

