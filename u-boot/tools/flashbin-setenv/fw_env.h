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
 * (C) Copyright 2002-2008
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * To build the utility with the run-time configuration
 * uncomment the next line.
 * See included "fw_env.config" sample file (TRAB board)
 * for notes on configuration.
 */
#define CONFIG_BAUDRATE		115200
#define CONFIG_BOOTDELAY	5	/* autoboot after 5 seconds	*/
#define CONFIG_BOOTCOMMAND							\
	"bootp; "								\
	"setenv bootargs root=/dev/nfs nfsroot=${serverip}:${rootpath} "	\
	"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}:${hostname}::off; "	\
	"bootm"

# include <stdint.h>

extern int   fw_printenv (int argc, char *argv[]);
extern char *fw_getenv   (char *name);
extern int   fw_setenv   (int argc, char *argv[]);
extern void  fw_setconfig(char *, unsigned int, unsigned int, unsigned int, unsigned int );

extern uint32_t  uboot_crc32 (uint32_t, const unsigned char *, unsigned);
