/*
 *  u-boot/drivers/serial/serial_cortina.c
 *
 * Copyright (c) Cortina-Systems Limited 2010.  All rights reserved.
 *                Jason Li <jason.li@cortina-systems.com>
 *
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

/* Simple U-Boot driver for the Cortina UARTs */

//#include <common.h>
//#include <watchdog.h>

#include "serial_cs752x.h"
#include "common.h"

//#ifdef CONFIG_GOLDENGATE
//DECLARE_GLOBAL_DATA_PTR;
//int serial_base;
//#endif


//#define IO_WRITE(addr, val) (*(volatile unsigned int *)(addr) = (val))
//#define IO_READ(addr) (*(volatile unsigned int *)(addr))

/*
 * Cortina G2 has four UARTs, we use the first two, at 115200-8-N-1
 */
#define CONSOLE_PORT CONFIG_CONS_INDEX
#define baudrate CONFIG_BAUDRATE

//#ifdef CONFIG_GOLDENGATE
static unsigned char *port[2];
//#else
//static volatile unsigned char *const port[] = CONFIG_GOLDENGATE_UART_PORTS;
//#endif

#define NUM_PORTS (sizeof(port)/sizeof(port[0]))

static void _cortina_putc (int portnum, char c);
static int _cortina_getc (int portnum);
static int _cortina_tstc (int portnum);
static int _cortina_getc_with_timeout(int portnum, char *c);

#ifdef	DEBUG
#define	cortina_putc(portnum,c) _cortina_putc ((portnum), (c))
#define	cortina_getc(portnum) _cortina_getc ((portnum))
#define	cortina_tstc(portnum)	_cortina_tstc ((portnum))
#define	cortina_getc_with_timeout(portnum, c) _cortina_getc_with_timeout((portnum), (c))
#else
#define	cortina_putc(portnum,c) while(0)
#define	cortina_getc(portnum) _cortina_getc ((portnum))
#define	cortina_tstc(portnum)	_cortina_tstc ((portnum))
#define	cortina_getc_with_timeout(portnum, c) _cortina_getc_with_timeout((portnum), (c))
#endif

int serial_init (void)
{
	unsigned int temp;
	unsigned int sample = 0;
	struct platform_clk clk;

//	serial_base = gd->bd->serial_base;
	port[0] = (void *)CONFIG_SYS_SERIAL0;
	port[1] = (void *)CONFIG_SYS_SERIAL1;

	/*
	 ** First, disable everything.
	 */
	IO_WRITE (port[CONSOLE_PORT] + UCFG, 0x0);

	/*
	 ** Set baud rate, 8 bits, 1 stop bit, no parity
	 **
	 */
	get_platform_clk(&clk);
	temp = clk.apb_clk/baudrate;
	temp = (temp << 8) | UCFG_EN | UCFG_TX_EN |UCFG_RX_EN | UCFG_CHAR_8;
	IO_WRITE (port[CONSOLE_PORT] + UCFG, temp);

	sample = (UART_CLOCK/baudrate)/2;
	sample = (sample < 7) ? 7 : sample;
	IO_WRITE (port[CONSOLE_PORT] + URX_SAMPLE, sample);

	return 0;
}


void serial_putc (const char c)
{
	if (c == '\n')
		cortina_putc (CONSOLE_PORT, '\r');

	cortina_putc (CONSOLE_PORT, c);
}

void serial_puts (const char *s)
{
	while (*s) {
		serial_putc (*s++);
	}
}

int serial_getc (void)
{
	return cortina_getc (CONSOLE_PORT);
}

int serial_getc_with_timeout(char *c)
{
	return cortina_getc_with_timeout(CONSOLE_PORT, c);
}

int serial_tstc (void)
{
	return cortina_tstc (CONSOLE_PORT);
}

void serial_setbrg (void)
{
}

static void _cortina_putc (int portnum, char c)
{
	volatile unsigned int status;

	/* Send the character */
	IO_WRITE (port[portnum] + UTX_DATA, c);

	// Wait for Tx FIFO not full
	do
	{
		status = IO_READ( port[portnum] + UINFO);
	}
	while (!(status & UINFO_TX_FIFO_EMPTY));

}

static int _cortina_getc (int portnum)
{
	unsigned int data;

	/* Wait until there is data in the FIFO */
	while ( IO_READ(port[portnum] + UINFO) & UINFO_RX_FIFO_EMPTY ) ;
	//	WATCHDOG_RESET();

	data = IO_READ (port[portnum] + URX_DATA) & 0xFF;
	return data;
}

static int _cortina_getc_with_timeout(int portnum, char *c)
{
   int counter;

   counter = 10;

   while ( IO_READ(port[portnum] + UINFO) & UINFO_RX_FIFO_EMPTY ){
      counter--;
      if (counter <= 0) {
         return 1;  // timedout
      }
   };

   *c = IO_READ(port[portnum] + URX_DATA);
   return 0;
}

static int _cortina_tstc (int portnum)
{
	unsigned int status;

//	WATCHDOG_RESET();
	status = IO_READ (port[portnum] + UINFO);

	if (status&UINFO_RX_FIFO_EMPTY)		// empty
		return 0;
	else								// something in RX FIFO
		return 1;
}
