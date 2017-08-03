/*
 * (C) Copyright 2002
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * System Recovery support
 */
#include <linux/ctype.h>
#include <common.h>
#include <command.h>
#include <post.h>
#include <nand.h>

#define	VERBOSE(...)	{ if (verbose) printf(__VA_ARGS__); }
#define	ERROR(...)	{ if (error) printf(__VA_ARGS__); }

#define	INVALID_UINT	0xFFFFFFFF
#define	SYS_INFO_MAGIC	0x1F3D5B79
#define	SYSINFO_MAX		2
#define	KERNEL_MAX		2
#define	ROOTFS_MAX		2
#define	FLASH_PAGE_SIZE	0x20000

typedef struct
{	
	uint			magic;
	uint			index;
	uint			kernel;
	uint			rootfs;
	image_header_t	header[ROOTFS_MAX];
	uint			crc32;
}	sys_info_t;
	

extern int  _nand_erase(ulong offset, size_t size, int clean, int quite);
extern int  _nand_read(ulong offset, size_t size, void *buff);
extern int  _nand_write(ulong offset, size_t size, void *buff);

static int	kernel_check_and_recovery(void);
static int	rootfs_check_and_recovery(void);
static int	check_kernel(int index, int printout);
static int  recovery_kernel(sys_info_t	*sysinfo);
static int	check_rootfs(int index, int printout);
static int  recovery_rootfs(sys_info_t	*sysinfo);
static uint	getenv_uint(char *name);
static uint	string_to_version(char *s);
static uint	image_get_version(const image_header_t *hdr);
static int	set_kernel_loc(uint loc);
static int	save_rootfs(cmd_tbl_t * cmdtp, int flag, int index, uint addr, uint len);
static int	save_sys_info(sys_info_t *info);
static int	load_sys_info(sys_info_t *info);
static void mem_dump(void *addr, int len);
static nand_info_t *get_nand_info(void);

extern int nand_curr_device;
char	_flash_buff[FLASH_PAGE_SIZE];
int		verbose = 1;
int		error = 1;

int do_recovery(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	kernel_check_and_recovery();
	rootfs_check_and_recovery();
}

int do_savefs(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	if (argc < 3)
	{
		goto error;
	}

	if (strcmp(argv[1], "rootfs") == 0)
	{
		uint addr;
		uint index;
		uint len  = getenv_uint("rootfs_len");
		if (len == 0)
		{
			ERROR("ERROR : RootFS size is invalid!\n");
			goto error;	
		}

		index = simple_strtoul(argv[2], NULL, 16);
		if (index >= ROOTFS_MAX)
		{
			ERROR("ERROR : RootFS index is out of range(0 .. %d)!\n", ROOTFS_MAX);
			goto error;
		}

		if (argc == 4)
		{
			addr = simple_strtoul(argv[3], NULL, 16);	
		}
		else
		{
			addr = getenv_uint("fileaddr");	
		}

		if (addr == 0)
		{
			ERROR("ERROR : RootFS image location is invalid.\n");
			goto error;	
		}

		if (save_rootfs(cmdtp, flag, index, addr, len) != 0)
		{
			ERROR("ERROR : RootFS can't save!\n");
			goto error;	
		}
	}

	return	0;	
error:
	return	1;
}

int	kernel_check_and_recovery(void)
{
	sys_info_t	sysinfo;
	int			i;


	if (load_sys_info(&sysinfo) != 0)
	{
		ERROR("ERROR : SysInfo is invalid!\n");
		goto error;	
	}

	for(i = 0 ; i < KERNEL_MAX ; i++)
	{

		int index = (sysinfo.kernel + i) % KERNEL_MAX;	
		if (check_kernel(index, 0) == 0)
		{
			char		buff[64];

			VERBOSE("Check %s kernel : SUCCESS!\n", (i)?"secondary":"primary");

			sprintf(buff, "kernel_loc%d", index);
			setenv ("kernel_loc", getenv(buff));
			break;
		}
		else
		{
			VERBOSE("Check %s kernel : FAILED!\n", (i)?"secondary":"primary");
		}
	}

	if (i == KERNEL_MAX)
	{
		ERROR("ERROR : All kernel is incorrect.\n");
		goto error;	
	}

	if (i != 0)
	{
		ERROR("ERROR : Primary kernel is invalid!\n");	

		uint	auto_recovery = 0;
		auto_recovery=getenv_uint("auto_recovery");
		if (auto_recovery == 1)
		{
			char		buff[64];
			VERBOSE("Primary kernel is configured auto recovery mode!\n");	
			VERBOSE("Recovery start!.\n");
			if (recovery_kernel(&sysinfo) == 0)
			{
				VERBOSE("Primary kernel recovery is completed !.\n");
				sprintf(buff, "kernel_loc%d", sysinfo.rootfs);
				setenv ("kernel_loc", getenv(buff));
			}
			else
			{
				VERBOSE("Primary kernel recovery is failed!.\n");
			}
		}
	}

	return	0;
error:
	return	1;

}

int	rootfs_check_and_recovery(void)
{
	sys_info_t	sysinfo;
	char		buff[64];
	int			i;


	if (load_sys_info(&sysinfo) != 0)
	{
		ERROR("ERROR : SysInfo is invalid!\n");
		goto error;	
	}

	for(i = 0 ; i < ROOTFS_MAX ; i++)
	{
		sprintf(buff, "rootfs_name%d", i);
		setenv (buff, "rootfs_standby");
	}

	for(i = 0 ; i < ROOTFS_MAX ; i++)
	{

		int index = (sysinfo.rootfs + i) % ROOTFS_MAX;	

		if (check_rootfs(index, 0) == 0)
		{
			VERBOSE("Check %s rootfs : SUCCESS!\n", (i)?"secondary":"primary");

			setenv ("rootfs_name%d", "rootfs");
			break;
		}
		VERBOSE("Check %s rootfs : FAILED!\n", (i)?"secondary":"primary");
	}

	if (i == ROOTFS_MAX)
	{
		ERROR("ERROR : All rootfs is incorrect.\n");
		goto error;	
	}

	if (i != 0)
	{
		ERROR("ERROR : Primary rootfs is invalid!\n");	

		uint	auto_recovery = 0;
		auto_recovery=getenv_uint("auto_recovery");
		if (auto_recovery == 1)
		{
			char		buff[64];
			VERBOSE("Primary rootfs is configured auto recovery mode!\n");	
			VERBOSE("Recovery start!.\n");
			if (recovery_rootfs(&sysinfo) == 0)
			{
				VERBOSE("Primary rootfs recovery is completed !.\n");
				sprintf(buff, "rootfs_%d_mtd", sysinfo.rootfs);
				setenv ("rootfs_mtd", getenv(buff));
			}
			else
			{
				VERBOSE("Primary rootfs recovery is failed!.\n");
			}
		}
	}

	return	0;
error:
	return	1;

}

uint	getenv_uint(char *name)
{
		char	*val;

		val = getenv(name);
		if (val == 0)
		{
				goto error;
		}

	return	simple_strtoul(val, NULL, 16);
error:

	return	INVALID_UINT;
}

int check_kernel(int index, int printout)
{
	sys_info_t	sysinfo;
	void		*buff= (void*)CONFIG_SYS_LOAD_ADDR;	
	ulong		kernel_loc;
	ulong		kernel_len;
	char		name[64];

	if (load_sys_info(&sysinfo) != 0)
	{
		ERROR("ERROR : System information loading failed!\n");
		goto error;
	}

	sprintf(name, "kernel_loc%d", index);
	kernel_loc= getenv_uint(name);
	if(kernel_loc == INVALID_UINT)
	{
		ERROR("ERROR: can't find location[name = 0x%08x]\n", kernel_loc);	
		goto error;
	}

	kernel_len = getenv_uint("kernel_len");
	if(kernel_len == INVALID_UINT)
	{
		ERROR("ERROR: can't find kernel_len\n");	
		goto error;
	}

	if (_nand_read(kernel_loc, kernel_len, (u_char *)buff) != 0)
	{
		ERROR("ERROR: nand read failed\n");
		goto error;	
	}

	if (genimg_get_format (buff) != IMAGE_FORMAT_LEGACY)
	{
		ERROR("ERROR: Invalid image format!\n");
		goto error;
	}
		
	if (!image_check_hcrc (buff))
	{
		ERROR("ERROR: Invalid head crc!\n");
		goto error;
	}


	if (!image_check_dcrc (buff)) 
	{
		ERROR("ERROR: Invalid kernel image\n");
		goto error;
	} 

	VERBOSE ("Kernel at %08X:\n", (uint)kernel_loc);
	image_print_contents (buff);

	return 0;

error:
	return	1;
}

int recovery_kernel(sys_info_t	*sysinfo)
{
	void*	buff;
	ulong 	primary_loc;
	ulong	secondary_loc;
	uint  	kernel_len;

	if (sysinfo->kernel == 0)
	{
		primary_loc = getenv_uint("kernel_loc0");
		secondary_loc = getenv_uint("kernel_loc1");
	}
	else
	{
		primary_loc = getenv_uint("kernel_loc1");
		secondary_loc = getenv_uint("kernel_loc0");
	}
	kernel_len= getenv_uint("kernel_len");


	buff = (void *)CONFIG_SYS_LOAD_ADDR;

	if ((_nand_read(secondary_loc, kernel_len, buff) != 0) ||
		(_nand_erase(primary_loc, kernel_len, 1, 0) != 0) ||
		(_nand_write(primary_loc, kernel_len, buff) != 0))
	{
		return	1;	
	}

	return	0;
}	


int check_rootfs(int index, int printout)
{
	sys_info_t	sysinfo;
	image_header_t *header;
	void		*buff= (void*)CONFIG_SYS_LOAD_ADDR;	
	ulong		rootfs_loc;
	ulong		rootfs_len;
	char		name[64];

	if (load_sys_info(&sysinfo) != 0)
	{
		ERROR("ERROR : System information loading failed!\n");
		goto error;
	}

	sprintf(name, "rootfs_loc%d", index);
	rootfs_loc= getenv_uint(name);
	rootfs_len= getenv_uint("rootfs_len");
	if(rootfs_loc == INVALID_UINT)
	{
		ERROR("ERROR: can't find location[name = 0x%08x]\n", rootfs_loc);	
		goto error;
	}

	header = &sysinfo.header[index];
	if (genimg_get_format (header) != IMAGE_FORMAT_LEGACY)
	{
		ERROR("ERROR: Invalid image format!\n");
		goto error;
	}
		
	if (!image_check_hcrc (header))
	{
		ERROR("ERROR: Invalid head crc!\n");
		goto error;
	}

	rootfs_len = ntohl(header->ih_size);
	if (_nand_read(rootfs_loc, rootfs_len, &((u_char *)buff)[sizeof(image_header_t)]) != 0)
	{
		ERROR("ERROR: nand read failed\n");
		goto error;	
	}

	memcpy(buff, header, sizeof(image_header_t));
	if (!image_check_dcrc (buff)) 
	{
		ERROR("ERROR: Invalid rootfs image\n");
		goto error;
	} 

	VERBOSE ("Root FS at %08X:\n", (uint)rootfs_loc);
	image_print_contents (header);

	return 0;

error:
	return	1;
}

int recovery_rootfs(sys_info_t	*sysinfo)
{
	void*	buff;
	ulong 	primary_loc;
	ulong	secondary_loc;
	uint  	rootfs_len;

	if (sysinfo->rootfs == 0)
	{
		primary_loc = getenv_uint("rootfs_loc0");
		secondary_loc = getenv_uint("rootfs_loc1");
	}
	else
	{
		primary_loc = getenv_uint("rootfs_loc1");
		secondary_loc = getenv_uint("rootfs_loc0");
	}
	rootfs_len= getenv_uint("rootfs_len");


	buff = (void *)CONFIG_SYS_LOAD_ADDR;

	if ((_nand_read(secondary_loc, rootfs_len, buff) != 0) ||
		(_nand_erase(primary_loc, rootfs_len, 1, 0) != 0) ||
		(_nand_write(primary_loc, rootfs_len, buff) != 0))
	{
		goto error;
	}

	return	0;

error:
	return	1;	
}	

uint string_to_version(char *s)
{
	uint	version;
	char *e;
	int i;

	if (s == NULL)
		return(0);

	for (version=0, i=0; i<4; ++i) {
		uint val = s ? simple_strtoul(s, &e, 10) : 0;
		version <<= 8;
		version |= (val & 0xFF);
		if (s) {
			s = (*e) ? e+1 : e;
		}
	}

	return version;
}

static uint	image_get_version(const image_header_t *hdr)
{
	char *ptr = strchr((char *)hdr->ih_name, '-');
	char buff[16];
	int	n;

	if (ptr == NULL)
	{
		return	0;	
	}

	while((*ptr != 0) && !isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr == 0)
	{
		return	0;	
	}

	memset(buff, 0, sizeof(buff));
	n = 0;
	while((*ptr != 0) && (isdigit(*ptr) || (*ptr == '.')))
	{
		buff[n++] = *ptr;
		ptr++;
	}

	return string_to_version(buff);
}

static int set_kernel_loc(uint loc)
{
	char	buff[64];

	sprintf(buff, "0x%08x", (unsigned int)loc);
	setenv("kernel_loc", buff); 

	return	0;
}

int	load_sys_info(sys_info_t *info)
{
	sys_info_t  lastinfo = { .index = INVALID_UINT };
	int			i;
	nand_info_t	*nand_info;
	uint		sysinfo_loc;
	uint		sysinfo_len;

	nand_info = get_nand_info();
	if (nand_info == NULL)
	{
		ERROR("ERROR : Can't find NAND Flash Info.\n");
		goto error;

	}

	sysinfo_loc = getenv_uint("sysinfo_loc");	
	if (sysinfo_loc == INVALID_UINT)
	{
		ERROR("ERROR : Cant' find system info. location!\n");	
		goto error;
	}

	sysinfo_len = getenv_uint("sysinfo_len");	
	if (sysinfo_loc == INVALID_UINT)
	{
		ERROR("ERROR : Cant' find system info. length!\n");	
		goto error;
	}


	for(i = 0 ; i < sysinfo_len / nand_info->erasesize ; i++)
	{
		uint	sysinfo_size = sizeof(sys_info_t);

		if (_nand_read(sysinfo_loc, sysinfo_size, _flash_buff) != 0)
		{
			ERROR("ERROR: nand read failed\n");
			goto error;	
		}

		sys_info_t *sysinfo = (sys_info_t *)_flash_buff;
		if (sysinfo->crc32 == crc32(0, (const uchar *)sysinfo, sizeof(sys_info_t) - sizeof(uint)))
		{
			if ((lastinfo.index == INVALID_UINT) || (lastinfo.index < sysinfo->index))
			{
				memcpy(&lastinfo, sysinfo, sizeof(sys_info_t));
			}
		}

		sysinfo_loc += nand_info->erasesize;
	}

	if (lastinfo.index == INVALID_UINT)
	{
		goto error;
	}

	memcpy(info, &lastinfo, sizeof(sys_info_t));

	return	0;

error:
	return	1;
}

int	save_sys_info(sys_info_t *info)
{
	uint		i, loc;
	uint		sysinfo_loc;
	uint		sysinfo_len;

	sysinfo_loc = getenv_uint("sysinfo_loc");	
	sysinfo_len = getenv_uint("sysinfo_len");	

	info->index++;
	info->crc32 = crc32(0, (const unsigned char *)info, sizeof(sys_info_t) - sizeof(uint));

	loc = sysinfo_loc + FLASH_PAGE_SIZE * (info->index % (sysinfo_len / FLASH_PAGE_SIZE));

	memcpy(_flash_buff, info, sizeof(sys_info_t));

	_nand_erase(loc, FLASH_PAGE_SIZE, 1, 0);
	_nand_write(loc, FLASH_PAGE_SIZE, _flash_buff);

	return	0;
}

int	save_rootfs(cmd_tbl_t * cmdtp, int flag, int index, uint addr, uint len)
{
	sys_info_t	sys_info;
	char		buf[64];
	uint		offset, size;
	int			i;

	if (load_sys_info(&sys_info) != 0)
	{
		memset(&sys_info, 0, sizeof(sys_info));
		sys_info.magic = SYS_INFO_MAGIC;
	}

	if (index >= ROOTFS_MAX)
	{
		goto error;		
	}

	sprintf(buf, "rootfs_loc%d", index);
	offset = getenv_uint(buf);	
	size = getenv_uint("rootfs_len");	

	if (offset == INVALID_UINT || size == INVALID_UINT)
	{
		ERROR("ERROR : Invalid offset or size\n");
		goto error;	
	}
	
	_nand_erase(offset, size, 1, 0);
	_nand_write(offset, size, (void *)(addr + sizeof(image_header_t)));

	memcpy(&sys_info.header[index], (void *)addr, sizeof(image_header_t));
	sys_info.rootfs = index;


	save_sys_info(&sys_info);

	return	0;

error:
	return	1;
}

void mem_dump(void *addr, int len)
{
	int	i;

	printf("memdump(0x%08x, 0x%08x)\n", addr, len);
	for(i = 0 ; i < len ; i++)
	{
		printf("%02x ", ((uchar *)addr)[i]);
		if ((i+1) % 8 == 0)
		{
			printf("\n");	
		}
	}

}

nand_info_t *get_nand_info(void)
{
	/* the following commands operate on the current device */
	if (nand_curr_device < 0 || 
		nand_curr_device >= CONFIG_SYS_MAX_NAND_DEVICE ||
   	 	!nand_info[nand_curr_device].name) 
	{  
		ERROR("ERROR: no devices available\n");
		return	NULL;
	}

	return &nand_info[nand_curr_device];
}

/***************************************************/

U_BOOT_CMD(
	recovery,	CONFIG_SYS_MAXARGS,	0,	do_recovery,
	"perform board recovery",
	"    - system recovery\n"
);


U_BOOT_CMD(
	savefs,	CONFIG_SYS_MAXARGS,	0,	do_savefs,
	"save Root FS",
	"    - save Root File System images\n"
);

