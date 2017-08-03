/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 * sw_reg_rw.c
 *
 * $Id: sw_reg_rw.c,v 1.1.1.1 2011/08/15 05:56:20 ewang Exp $
 *
 * Device Control Commands for Switch Registers Read/Write Utility.
 *
 * $Log: $
 *
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "rtl8370_ioctl.h"		/* ioctl command IDs */
#include "rtl8370_vb.h"		/* RTK_CMD_T */




#define isdigit_1(c)		(c >= '0' && c <= '9') ? 1 : 0
#define ishex_1(c)		((c >= 'A' && c <= 'F') || \
				 (c >= 'a' && c <= 'f')) ? 1 : 0

u_int32_t char2hex_1(u_int8_t c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);
	else
		return (0xffffffff);
}

u_int32_t char2decimal_1(u_int8_t c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else
		return (0xffffffff);
}

u_int32_t str2hex_1(u_int8_t *cp)
{
    u_int32_t value, result;

    result = 0;
    if (*cp=='0' && toupper(*(cp+1))=='X')
    	cp += 2;
    
    while ((value = char2hex_1(*cp)) != 0xffffffff)
    {
          result = result * 16 + value;
          cp++;
    }

    return(result);
}

u_int32_t str2decimal_1(u_int8_t *cp)
{
    u_int32_t value, result;

	result=0;
    while ((value = char2decimal_1(*cp)) != 0xffffffff)
    {
		result= result * 10 + value;
		cp++;
    }

    return(result);
}


u_int32_t str2value_1(u_int8_t *strp)
{
    char	*cp;
    int		is_hex = 0;

    cp = strp;
    if (*cp=='0' && toupper(*(cp+1))=='X')
    {
    	strp += 2;
    	is_hex = 1;
    }
    
    cp = strp;
    while (*cp)
    {
    	if (ishex_1(*cp))
    		is_hex = 1;
		else if (!isdigit_1(*cp))
			return 0;
		cp++;
	}
    
    return (is_hex) ? str2hex_1(strp) : str2decimal_1(strp);
    	
}

/*
 * sw_reg_rw [reg_addr] ([data])
 */

int main(int argc, char **argv)
{
	RTK_CMD_T ioctl_cmd;
	int i, ret = -1;
	unsigned int reg_addr, data;
	int fd;
	char file[] = "/dev/" SWITCH_DEVICE_NAME;

	/* parse all parameters */
	switch (argc) {
	case 3:
		data = str2value_1(argv[2]);
		/* fall through */
	case 2:
		reg_addr = str2value_1(argv[1]);
		break;
	case 1:
	default:
		goto out;
	}

    	if ((fd = open(file, O_SYNC|O_RDWR)) < 0)
        {
		fprintf(stderr, "ERROR: Open switch device (%s) error: \n", file);
		return -1;
        }
    	
	/* handle requests */
	switch (argc) {
	case 2:
		/* sw_reg_rw [reg_addr] */
		/* read specific reg addr */

		printf("Read from reg 0x%04X \n", reg_addr);
		memset(&ioctl_cmd, 0, sizeof(ioctl_cmd));
		ioctl_cmd.cmd = SWITCH_REG_READ;
		ioctl_cmd.para.mdio.phy_addr = 0;
		ioctl_cmd.para.mdio.reg_addr = reg_addr;
		
		ret = ioctl(fd, SIOCDEVPRIVATE, &ioctl_cmd) ;
		if (ret) {
			fprintf(stderr, "Fail to read from file %s, ret = %d\n",
				file, ret);
			goto close_fd;
		}
		
		if (ioctl_cmd.ret) {
			fprintf(stderr, "Fail to read reg %d\n", reg_addr);
			goto close_fd;
		}
		
		printf("reg 0x%04X = 0x%04X\n", reg_addr, 
						ioctl_cmd.para.mdio.data);
		break;
		
	case 3:
		/* sw_reg_rw [reg_addr] [data] */
		/* write data to specific reg addr */

		printf("Write 0x%04X to reg 0x%04X\n", data, reg_addr);
		memset(&ioctl_cmd, 0, sizeof(ioctl_cmd));
		ioctl_cmd.cmd = SWITCH_REG_WRITE;
		ioctl_cmd.para.mdio.phy_addr = 0;
		ioctl_cmd.para.mdio.reg_addr = reg_addr;
		ioctl_cmd.para.mdio.data = data;
		
		ret = ioctl(fd, SIOCDEVPRIVATE, &ioctl_cmd) ;
		if (ret) {
			fprintf(stderr, "Fail to write to file %s, ret = %d\n",
				file, ret);
			goto close_fd;
		}
		
		if (ioctl_cmd.ret) {
			fprintf(stderr, 
				"Fail to write reg %d\n", reg_addr);
			goto close_fd;
		}
		break;		
	default:
		goto close_fd;
	}

	close(fd);
	return 0;

close_fd:
	close(fd);
out:
	fprintf(stderr, "USAGE:\n"
		"%s [reg_addr] ([data])\n"
		"\treg_addr : 0 - 0xFFFF\n"
		"\tdata     : 0 - 0xFFFF\n",
		argv[0]);
	return ret;
}
