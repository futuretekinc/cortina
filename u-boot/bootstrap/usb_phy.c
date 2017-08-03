#include "common.h"

#if (CONFIG_CORTINA_DIAG==1)

int g2_usb_test()
{
	unsigned int reg_v;

	IO_WRITE( 0xf0000014 , 0x00000FAA);
	udelay(10000);
	IO_WRITE( 0xf0000090 , 0x00000000);
	udelay(10000);

	IO_WRITE( 0xf4000010 , 0x00010005);
	IO_WRITE( 0xf4000018 , 0x00000037);
	IO_WRITE( 0xf4000024 , 0xE5F80000);
	IO_WRITE( 0xf4000028 , 0xE5F70000);
	IO_WRITE( 0xf4000054 , 0x00001000);
	IO_WRITE( 0xf4000058 , 0x00001000);

	udelay(10000);
	IO_WRITE( 0xf4000050 , 0x00000001);

	udelay(10000);
	IO_WRITE( 0xf4000054 , 0x00001103);
	udelay(10000);
	IO_WRITE( 0xf4000054 , 0x00001001);
	udelay(10000);
	IO_WRITE( 0xf4000058 , 0x00001103);
	udelay(10000);
	IO_WRITE( 0xf4000058 , 0x00001001);

	/* Port 0 */
	reg_v = IO_READ(0xF4000054);
	if(reg_v & 0x4)
		serial_puts("USB Port 0 PASS\n");
	else
		serial_puts("USB Port 0 FAIL\n");

	/* Port 1 */
	reg_v = IO_READ(0xF4000058);
	if(reg_v & 0x4)
		serial_puts("USB Port 1 PASS\n");
	else
		serial_puts("USB Port 1 FAIL\n");

	return 0;
}

#endif /* CONFIG_CORTINA_DIAG */

