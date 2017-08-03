#include <common.h>
#include <command.h>
#include <malloc.h>
#include <serial.h>
#include <nand.h>
#include <mmc.h>

#include "everify.h"

OSSLAPI*		osslapi;

#define GOLDENGATE_OTP_BASE		((unsigned char*)0xF5008000)	/* OTP base address*/
#define GOLDENGATE_OTP_SIZE			      0x400	/* OTP size*/
#define	OTP_MAGIC	0x3C

int check_blank(unsigned char *ptr)
{
	if(ptr[GOLDENGATE_OTP_SIZE-1] == 0xA5)
		return 1;

	if(ptr[0] == 0x5A)
		return 1;

	if(ptr[GOLDENGATE_OTP_SIZE-1] == 0 || ptr[0] == 0)
		return 0;

	return 2;
}


void get_element(unsigned char etype, unsigned char *buf, unsigned int *length)

{
	unsigned short i, type, len;
	unsigned char *ptrotp, *pdata, *end;
	
	
	ptrotp = GOLDENGATE_OTP_BASE+GOLDENGATE_OTP_SIZE-1;
	end = GOLDENGATE_OTP_BASE+1;
	pdata = buf;			/* copy payloads to here */
	
	for(;;) {
		
		if(ptrotp - end < 4 || length[0]!=0)
			break;

		if(ptrotp[0] != OTP_MAGIC) {
			ptrotp--;	
			continue;
		}

		type = ptrotp[-1];
		if(type < 1 || type > 32) {
			ptrotp--;	
			continue;
		}

		len = ptrotp[-2] | (ptrotp[-3] << 8);
		if(len > ptrotp - end) {
			ptrotp--;
			continue;
		}

		ptrotp -= 4;

		if(type == etype)
		{
			length[0] = len;
			
			for(i=0;i<len;i++)
				*pdata++ = *ptrotp--;			
		}
		else
			ptrotp -= len;
	}
}

int do_verify(unsigned char *addr, unsigned int len)
{
	unsigned char key[256], *sig;
	unsigned int key_len=0, r=0;
	
	if (len % 0x4000)
	{
		printf("Image length not 16kB alignment\n");
		return -1;
	}
	
	osslapi = ossl_init();
	//if(osslapi == NULL) {
	if(!osslapi) {
		printf("ossl_init fail~~\n");
	}
#if 1
	if(check_blank(GOLDENGATE_OTP_BASE)!=1)
	{
		printf("No boot information. \n");
		return 0;
	}
	
#else	
	unsigned char i, md[32];
	unsigned char test0[7]={0x00, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00};
	unsigned char test1[7]={0x01, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00};
	unsigned char test2[7]={0x02, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00};
	unsigned char test3[7]={0x03, 0x02, 0x80, 0x00, 0x10, 0x00, 0x00};
	
	char nandinfo1[39] = {0x01, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00,  \
    			 0x7e, 0x42, 0xae, 0xa9, 0xa0, 0x8e, 0xef, 0xbe, \
    			 0x82, 0x62, 0x47, 0x9e, 0xe1, 0x43, 0x9f, 0xc4, \
    			 0x76, 0x9a, 0x59, 0xf6, 0xb4, 0xe9, 0xdc, 0x6f, \
    			 0x04, 0xf1, 0x80, 0xb2, 0x4f, 0x06, 0xf1, 0x19, \
    			};
    char nandinfo0[39] = {0x00, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00,  \
    			 0xa0, 0xea, 0x1f, 0x7c, 0x49, 0xf7, 0x24, 0x3e, \
    			 0xdc, 0x72, 0x0c, 0x89, 0x47, 0xbc, 0x6f, 0x37, \
    			 0xe1, 0x97, 0xdc, 0x9e, 0x08, 0xfb, 0x88, 0xc9, \
    			 0x1b, 0x35, 0xcb, 0x49, 0x3e, 0x02, 0x73, 0x28, \
    			};
    char nandinfo2[39] = {0x02, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00,  \
    			 0x36, 0xfa, 0x15, 0xeb, 0xe5, 0xbd, 0x68, 0x70, \
    			 0xd9, 0x8c, 0xc8, 0xe7, 0xc1, 0x50, 0xe4, 0x32, \
    			 0x29, 0xe5, 0x32, 0x37, 0xd9, 0xa1, 0x72, 0xc8, \
    			 0x6b, 0xd7, 0x39, 0xd0, 0x05, 0xfc, 0x41, 0x86, \
    			};
  
    char nandinfo3[39] = {0x03, 0x02, 0x80, 0x00, 0x10, 0x00, 0x00,  \
    			  0xae, 0xfe, 0x50, 0x00, 0x1e, 0xb4, 0x0a, 0xfb, \
    			  0x3e, 0x9d, 0xa8, 0x87, 0xd3, 0xc4, 0xb5, 0xa5, \
    			  0x64, 0xc6, 0xb6, 0xf6, 0x8f, 0x95, 0xc2, 0x69, \
    			  0x87, 0xc2, 0xe2, 0x07, 0x7b, 0x2b, 0xa4, 0x6f, \
    			};		
	memset(md,0,32);
	osslapi->SHA256(test0, 7, md);	
	if (memcmp(md, &nandinfo0[7], 32))
	{
		printf("MD:0\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", md[i]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
		
		printf("-->:0\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", nandinfo0[i+7]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
	}
	
	memset(md,0,32);
	osslapi->SHA256(test1, 7, md);
	if (memcmp(md, &nandinfo1[7], 32))
	{
			
		printf("MD:1\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", md[i]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
		
		printf("-->:1\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", nandinfo1[i+7]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
	}
	memset(md,0,32);
	osslapi->SHA256(test2, 7, md);
	if (memcmp(md, &nandinfo2[7], 32))
	{
		printf("MD:2\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", md[i]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
		
		printf("-->:2\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", nandinfo2[i+7]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
	}
	memset(md,0,32);
	osslapi->SHA256(test3, 7, md);
	if (memcmp(md, &nandinfo3[7], 32))
	{
		printf("MD:3\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", md[i]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
		
		printf("-->:3\n");	
		for(i=0;i<sizeof(md);i++)
		{		
			printf(" 0x%02x,", nandinfo3[i+7]);	
			if (i%8==7)
				printf("\n");
		}
		printf("\n");
	}
	
	
	if(check_blank(GOLDENGATE_OTP_BASE)!=1)
	{
		printf("No boot information. \n");
		return 0;
	}
	else
	{	
		key_len = 0;
		memset(key, 0, 256);
	
		get_element(1, key, &key_len);
		if(key_len == 0)
		{
			printf("No key found !!\n");
			return -1;
		}
		
		sig = (addr + len - 255);
		/*
 		 * returns 0 for success, non-zero error number for failure,
		 * see openssl/include/secboot-ossl.h for error enum
		 */
		r = osslapi->ecdsa_verify(addr, (len-256+1),
				sig, 255,
				key, key_len);
		if(r != 0)
			printf("ERROR: verify %d\n", r);
			
			return r;
	}
#endif	
}



