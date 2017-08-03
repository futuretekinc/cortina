/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2012 by Cortina Systems Incorporated.                 */
/***********************************************************************/
#include <common.h>
#include <net.h>
#include "ssdk_init.h"
#include "sd.h"
#include "isisc_reg_access.h"

typedef enum {
	FAL_HALF_DUPLEX = 0,
	FAL_FULL_DUPLEX,
	FAL_DUPLEX_BUTT = 0xffff
} fal_port_duplex_t;

typedef enum
{
	FAL_SPEED_10    = 10,
	FAL_SPEED_100   = 100,
	FAL_SPEED_1000  = 1000,
	FAL_SPEED_10000 = 10000,
	FAL_SPEED_BUTT  = 0xffff,
} fal_port_speed_t;

extern int g2_phy_read(unsigned int phy_addr, unsigned int offset);
extern int g2_phy_write(unsigned int phy_addr, unsigned int offset, unsigned int val);

sw_error_t qca_phy_write(a_uint32_t dev_id, a_uint32_t phy_addr, a_uint32_t reg,
			a_uint16_t data)
{
	g2_phy_write(phy_addr, reg, data);
	return 0;
}

sw_error_t qca_phy_read(a_uint32_t dev_id, a_uint32_t phy_addr, a_uint32_t reg,
			a_uint16_t *data)
{
	*data = g2_phy_read(phy_addr, reg);
	return 0;
}

int qca83xx_disable_phy(void)
{
	uint32_t	i;
	uint16_t	phy_val;
	
	for (i = 0; i <= 4; i++) {
		isis_phy_get(0, i, 0, &phy_val);
		phy_val = phy_val | 0x0800;
		isis_phy_set(0, i, 0, phy_val);
	}
	return 0;
}

int qca83xx_init(void)
{
	ssdk_init_cfg cfg;
	garuda_init_spec_cfg chip_spec_cfg;
	int rv, i;
	uint32_t val;
	uint16_t phy_data;
	unsigned char 	cpu_mac[6];

	memset(&cfg, 0, sizeof(ssdk_init_cfg));
	memset(&chip_spec_cfg, 0, sizeof(garuda_init_spec_cfg));
	/* memset(&cpu_mac, 0 , sizeof(cpu_mac)); */

	cfg.cpu_mode = HSL_CPU_1;
	cfg.reg_mode = HSL_MDIO;
	cfg.nl_prot = 30;
	cfg.chip_type = CHIP_ISISC;
	cfg.chip_spec_cfg = &chip_spec_cfg;
	cfg.reg_func.mdio_set = qca_phy_write;
	cfg.reg_func.mdio_get = qca_phy_read;
	cfg.reg_func.header_reg_set = NULL;
	cfg.reg_func.header_reg_get = NULL;

	rv = sd_init(0, &cfg);
	rv = rv | isis_reg_access_init(0, cfg.reg_mode);
	if (rv != 0)
	    printf("\n####### QCA SSDK init failed! (code: %d) ########\n", rv);

	/* Do software reset */
	isis_reg_get(0, 0x0, (uint8_t *) &val, sizeof(uint32_t));
	val |= 0x80000000;
	isis_reg_set(0, 0x0, (uint8_t *) &val, sizeof(uint32_t));
	for (i=0; i<10000; i++) {
		isis_reg_get(0, 0x0, (uint8_t *) &val, sizeof(uint32_t));
		if ((val & 0x80000000) == 0)
			break;
		udelay(10);
	}

	/* Workaround for S17c */
	val = 1;
	isis_reg_field_set(0, 0x8, 24, 1, (uint8_t *) &val, sizeof(uint32_t));

	/* Configure MAC0 as RGMII mode */
	/*
	   Set tx/rx delay for MAC0 
	   The MAC0 timing control is in the Port0 PAD Mode Control Register 
	   (offset 0x0004):
	   1. Bit 24: enable the timing delay for the output path. 
	      Set 1 to add 2ns delay in 1G mode. 
	      (Need to enable 0x8[24] to take effect)
	   2. Bit [21:20]: select the delay time for the output path in 10/100
	      mode.
	   3. Bit 25: enable the timing delay for the input path in 1G mode.
	   4. Bit [23:22]: select the delay time for the input path in 1G mode.
		00: 0.2ns
		01: 1.2ns
		10: 2.1ns
		11: 3.1ns
	 */
	val = 0x40000000;
	isis_reg_set(0, 0x10, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x007f7f7f;
	isis_reg_set(0, 0x624, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x07600000;
	isis_reg_set(0, 0x4, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x01000000;
	isis_reg_set(0, 0x8, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x7e;
	isis_reg_set(0, 0x7c, (uint8_t *) &val, sizeof(uint32_t));

	/* Configure MAC6 as RGMII mode */
	val = 0x40000000;
	isis_reg_set(0, 0x10, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x07600000;
	isis_reg_set(0, 0xc, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x7e;
	isis_reg_set(0, 0x94, (uint8_t *) &val, sizeof(uint32_t));

	/* Set PHY4(MAC5) as a single PHY */
	val = 0x40000000;
	isis_reg_set(0, 0x10, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x00020000;
	isis_reg_set(0, 0xc, (uint8_t *) &val, sizeof(uint32_t));
	val = 0;
	isis_reg_set(0, 0x90, (uint8_t *) &val, sizeof(uint32_t));
	val = 0;
	isis_reg_set(0, 0x94, (uint8_t *) &val, sizeof(uint32_t));


	/* Add CPU MAC address to MAC0 */
	eth_getenv_enetaddr("ethaddr",cpu_mac);
		
	val = cpu_mac[2] << 24 | cpu_mac[3] << 16 | 
		cpu_mac[4] << 8 | cpu_mac[5];
	isis_reg_set(0, 0x0600, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x88010000 | cpu_mac[0] << 8 | cpu_mac[1];
	isis_reg_set(0, 0x0604, (uint8_t *) &val, sizeof(uint32_t));
	val = 0xf;
	isis_reg_set(0, 0x0608, (uint8_t *) &val, sizeof(uint32_t));
	val = 0x80000002;
	isis_reg_set(0, 0x060c, (uint8_t *) &val, sizeof(uint32_t));

	/* Set speed, tx/rx flow control and force link up for MAC0 */
	/*
	isis_reg_get(0, 0x7c, (uint8_t *) &val, sizeof(uint32_t));
	val = (1<<9) | (1<<12) | (1<<6) | (2);
	isis_reg_set(0, 0x7c, (uint8_t *) &val, sizeof(uint32_t));
	*/
	val = 0xcf33cf33;
	isis_reg_set(0, 0x50, (uint8_t *) &val, sizeof(uint32_t));
	val = 0xcf33cf33;
	isis_reg_set(0, 0x54, (uint8_t *) &val, sizeof(uint32_t));
	val = 0xcf33cf33;
	isis_reg_set(0, 0x58, (uint8_t *) &val, sizeof(uint32_t));

	/* Reset to enable all PHYs */
	for (i = 0; i <= 4; i++) {
		isis_phy_get(0, i, 0, &phy_data);
		phy_data = phy_data | 0x8000 | 0x3100;
		isis_phy_set(0, i, 0, phy_data);
	}
	udelay(50000);

	/* Workaround for QCA8337(N)-1L3E */
	for (i = 0; i <= 4; i++) {
		isis_phy_set(0, i, 0x1d, 0x3d);
		isis_phy_set(0, i, 0x1e, 0x68a0);
	}

	/* Set delay of PHY#4 (for single PHY mode) */
	isis_phy_set(0, 4, 0x1d, 0x12);
	isis_phy_set(0, 4, 0x1e, 0x4c0c);

	isis_phy_set(0, 4, 0x1d, 0x0);
	isis_phy_set(0, 4, 0x1e, 0x82ee);

	isis_phy_set(0, 4, 0x1d, 0x5);
	isis_phy_set(0, 4, 0x1e, 0x3d46);

	isis_phy_set(0, 4, 0x1d, 0xb);
	isis_phy_set(0, 4, 0x1e, 0xbc20);

	return 0;
}
