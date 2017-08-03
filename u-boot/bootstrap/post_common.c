/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2009 by Cortina Systems Incorporated.                 */
/***********************************************************************/

#include "serial_cs752x.h"
#include "common.h"
#include "config.h"

inline int
isupper( int c )
{
        return (('A' <= c) && (c <= 'Z'));
} /* isupper() */


inline int
islower( int c )
{
        return (('a' <= c) && (c <= 'z'));
} /* islower() */


inline int
isdigit( int c )
{
        return ( ('0' <= c) && (c <= '9') );
} /* isdigit() */

inline int
isspace( int c )
{
        return ( (c == ' ') || (c == '\f') || (c == '\n') || (c == '\r') ||
                 (c == '\t') || (c == '\v') );
} /* isspace() */

inline int
isxdigit( int c )
{
        return ( isdigit(c) ||
                 (('a' <= c) && (c <= 'f')) ||
                 (('A' <= c) && (c <= 'F')) );
} /* isxdigit() */

inline int
isalpha( int c )
{
        return ( islower(c) || isupper(c) );
} /* isalpha() */

inline int tolower( int c )
{
        return isupper(c) ? c - 'A' + 'a' : c;
} /* tolower() */

struct platform_clk sys_clk_info;
static int got_clk_info = 0;

unsigned int iros_strtol( const char *nptr)
{
    const char *s = nptr;
    unsigned int acc;
    int c;
    int base = 0;
    int any;

    //
    // Skip white space and pick up leading +/- sign if any.
    // If base is 0, allow 0x for hex and 0 for octal, else
    // assume decimal; if base is already 16, allow 0x.
    //

    do {
        c = *s++;
    } while (isspace(c));

    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    // no overflow check

    for (acc = 0, any = 0;; c = *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }
    return acc;
} // strtol()



void serial_put_hex_no_0x(unsigned int val) {
    int i;
    unsigned int v1, v2;
    v1 = val;
    for (i = 0; i < 8; i++) {
        v2 = (v1 & 0xf0000000)>>28;
        v1 = (v1 << 4);
        if (v2 <= 9) {
                serial_putc('0'+v2);
        } else {
                serial_putc('a' + v2 - 10);
        }
    }
};

void serial_put_hex(unsigned int val) {
    serial_putc('0');
    serial_putc('x');
    serial_put_hex_no_0x(val);
};

void serial_put_binary(char val) {
    int i;
    char v1, v2;
    v1 = val;
    for (i = 0; i < 8; i++) {
        v2 = (v1 & 0x80)>>7;
        v1 = (v1 << 1);
        if (v2 == 0 ) {
            serial_putc('0');
        } else {
            serial_putc('1');
        }
    }
};

void serial_put_decimal(unsigned int val)
{
	char *cp, *buf;
	char temp[16], buff[16];
	int length;
	unsigned char digits[] = "0123456789";

	buf = buff;
	cp = temp;
	do {
		*cp++ = digits[(val % 10)&0xf];
		val = val / 10;
	} while(val != 0);

	length = 0;
	while (cp != temp) {
		*buf++ = *--cp;
		serial_putc(*cp);
		length++;
	}
	*buf = '\0';

}


int user_escape(void)
{
	unsigned char c;

//	if ((serial_getc_with_timeout(&c)==0) && ((c==0x03) || (c==0x1B))) {		/* ^C or ESC */
	if ((serial_getc_with_timeout(&c)==0) && (c==0x03)) {		/* ^C */
    	// User want to break
		return 1;
	}

	return 0;
}

void get_platform_clk(struct platform_clk *clk)
{
#ifdef CONFIG_CORTINA_FPGA
	sys_clk_info.cpu_clk = 400 * 1000000;
	sys_clk_info.axi_clk = 104 * 1000000;
	sys_clk_info.apb_clk = 50 * 1000000;;

	clk->cpu_clk = sys_clk_info.cpu_clk;
	clk->apb_clk = sys_clk_info.apb_clk;
	clk->axi_clk = sys_clk_info.axi_clk;
#else
	unsigned int reg_v;

	if (got_clk_info == 1){
		clk->cpu_clk = sys_clk_info.cpu_clk;
		clk->apb_clk = sys_clk_info.apb_clk;
		clk->axi_clk = sys_clk_info.axi_clk;
		return ;
	}

	reg_v = IO_READ(GLOBAL_STRAP);

	reg_v = (reg_v >> 1) & 0x07;
	switch (reg_v) {
	case 0:
		sys_clk_info.cpu_clk = 400 * 1000000;
		sys_clk_info.apb_clk = 100 * 1000000;
		sys_clk_info.axi_clk = 133333333;
		break;
	case 1:
		sys_clk_info.cpu_clk = 600 * 1000000;
		sys_clk_info.apb_clk = 100 * 1000000;
		sys_clk_info.axi_clk = 150 * 1000000;
		break;
	case 2:
		sys_clk_info.cpu_clk = 700 * 1000000;
		sys_clk_info.apb_clk = 100 * 1000000;
		sys_clk_info.axi_clk = 140 * 1000000;
		break;
	case 3:
		sys_clk_info.cpu_clk = 800 * 1000000;
		sys_clk_info.apb_clk = 100 * 1000000;
		sys_clk_info.axi_clk = 160 * 1000000;
		break;
	case 4:
		sys_clk_info.cpu_clk = 900 * 1000000;
		sys_clk_info.apb_clk = 100 * 1000000;
		sys_clk_info.axi_clk = 150 * 1000000;
		break;
	case 5:
		sys_clk_info.cpu_clk = 750 * 1000000;
		sys_clk_info.apb_clk = 150 * 1000000;
		sys_clk_info.axi_clk = 150 * 1000000;
		break;
	case 6:
		sys_clk_info.cpu_clk = 850 * 1000000;
		sys_clk_info.apb_clk = 170 * 1000000;
		sys_clk_info.axi_clk = 141666667;
		break;
	default:
		serial_puts("Unknow strap pin for cpu clock\n");
		break;
	}

	clk->cpu_clk = sys_clk_info.cpu_clk;
	clk->apb_clk = sys_clk_info.apb_clk;
	clk->axi_clk = sys_clk_info.axi_clk;

	got_clk_info = 1;

#endif
}

