/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 *
 * rtl83xx_main.c
 *
 * $Id: rtl83xx_main.c,v 1.5 2012/05/09 02:47:38 ywu Exp $
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <net.h>
#include <asm/types.h>
#include <config.h>
#include <registers.h>
#include "rtk_api.h"
#include "rtl8367b_asicdrv_port.h"
#include "smi.h"

/* #define SWITCH_DBG	1 */

extern rtk_api_ret_t rtk_switch_init(void);
extern rtk_api_ret_t rtk_port_macForceLinkExt_set(rtk_ext_port_t port, 
	rtk_mode_ext_t mode, rtk_port_mac_ability_t *pPortability);
extern rtk_api_ret_t rtk_port_macForceLinkExt_get(rtk_ext_port_t port, 
	rtk_mode_ext_t *pMode, rtk_port_mac_ability_t *pPortability);
extern rtk_api_ret_t rtk_port_rgmiiDelayExt_set(rtk_ext_port_t port, 
	rtk_data_t txDelay, rtk_data_t rxDelay);
extern rtk_api_ret_t rtk_port_rgmiiDelayExt_get(rtk_ext_port_t port, 
	rtk_data_t *txDelay, rtk_data_t *rxDelay);
/*
extern rtk_api_ret_t rtk_led_operation_set(rtk_led_operation_t mode);
extern rtk_api_ret_t rtk_led_serialMode_set(rtk_led_active_t active);
extern rtk_api_ret_t rtk_led_groupConfig_set(rtk_led_group_t group, 
	rtk_led_congig_t config);
*/
extern ret_t rtl8367b_setAsicRegBit(rtk_uint32 reg, rtk_uint32 bit, 
	rtk_uint32 value);
extern ret_t rtl8367b_setAsicReg(rtk_uint32 reg, rtk_uint32 value);

void rtl83xx_disble_phy(void)
{
	rtk_port_phyEnableAll_set(DISABLED);
	udelay(1000000); /* wait 1 sec */
}

int rtl83xx_init(void)
{
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;

	mode = MODE_EXT_RGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = SPD_1000M;
	mac_cfg.duplex = FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = DISABLED;
	mac_cfg.rxpause = DISABLED;

	/*
	 * Reset Switch
	 */
	rtl8367b_setAsicReg(0x1322, 1);
	udelay(1000000); /* wait 1 sec */

	/*
	 * Init Switch
	 */
	rtk_switch_init();
	rtk_port_phyEnableAll_set(DISABLED);
#if defined(CONFIG_MK_ENGINEERINGS) || \
	defined(CONFIG_MK_REFERENCES) || \
	defined(CONFIG_MK_WAN) || \
	defined(CONFIG_MK_BHR) || \
	defined(CONFIG_MK_PON)
	rtk_port_macForceLinkExt_set(1, mode, &mac_cfg);
	rtk_port_rgmiiDelayExt_set(1, 1, 7);
#else
	rtk_port_macForceLinkExt_set(0, mode, &mac_cfg);
	rtk_port_rgmiiDelayExt_set(0, 1, 7);
#endif

#if defined(CONFIG_MK_REFERENCE) || \
	defined(CONFIG_MK_REFERENCEB) || \
	defined(CONFIG_MK_ENGINEERING)
	
	/*
	 * Set LED operation mode to serial mode.
	 */
	/* rtk_led_operation_set(LED_OP_SERIAL); */
	rtl8367b_setAsicRegBit(RTL8367B_REG_LED_SYS_CONFIG, 
		RTL8367B_LED_SELECT_OFFSET, 1);
	/*Enable serial CLK mode*/
	rtl8367b_setAsicRegBit(RTL8367B_REG_SCAN0_LED_IO_EN,
		RTL8367B_LED_SERI_CLK_EN_OFFSET, 1);
	/*Enable serial DATA mode*/
	rtl8367b_setAsicRegBit(RTL8367B_REG_SCAN0_LED_IO_EN,
		RTL8367B_LED_SERI_DATA_EN_OFFSET, 1);
	

	/*
	 * LED configuration
	 */
#if defined(CONFIG_MK_REFERENCE)
	rtl8367b_setAsicRegBit(RTL8367B_REG_LED_SYS_CONFIG,
		RTL8367B_SERI_LED_ACT_LOW_OFFSET, 1);
	/*
	 * LED function selection
	 * Reference board
	 *	LED0: not used
	 *	LED1: Link/Act
	 *	LED2: Speed 1000M
	 */
	rtl8367b_setAsicReg(0x1B03, 0x0320);

#elif defined(CONFIG_MK_REFERENCEB)
	rtl8367b_setAsicRegBit(RTL8367B_REG_LED_SYS_CONFIG,
		RTL8367B_SERI_LED_ACT_LOW_OFFSET, 1);
	/*
	 * LED function selection
	 * Reference B board
	 *	LED0: not used
	 *	LED1: not used
	 *	LED2: Link/Act
	 */
	rtl8367b_setAsicReg(0x1B03, 0x0200);
#else /* #elif defined(CONFIG_MK_ENGINEERING) */
	rtl8367b_setAsicRegBit(RTL8367B_REG_LED_SYS_CONFIG,
		RTL8367B_SERI_LED_ACT_LOW_OFFSET, 0);
	/*
	 * LED function selection
	 * Engineering board
	 *	LED0: not used
	 *	LED1: Link/Act
	 *	LED2: Speed 1000M
	 */
	rtl8367b_setAsicReg(0x1B03, 0x0320);
#endif

#else  /* parallel mode */

#if defined(CONFIG_MK_REFERENCES) || \
	defined(CONFIG_MK_WAN) || \
	defined(CONFIG_MK_BHR) || \
	defined(CONFIG_MK_PON)
	/*
	 * LED function selection
	 * Reference S board
	 *	LED0: not used
	 *	LED1: Link/Act
	 *	LED2: not used
	 */
	rtl8367b_setAsicReg(0x1B03, 0x0020);
#else /* #elif defined(CONFIG_MK_ENGINEERINGS) */
	/*
	 * LED function selection
	 * Engineering S board
	 *	LED0: Link/Act
	 *	LED1: Speed 1000M
	 *	LED2: not used
	 */
	rtl8367b_setAsicReg(0x1B03, 0x0032);
#endif /* defined(CONFIG_MK_REFERENCES) */

#endif /* defined(CONFIG_MK_REFERENCE) ||
	defined(CONFIG_MK_REFERENCEB) || 
	defined(CONFIG_MK_ENGINEERING) */
	rtk_port_phyEnableAll_set(ENABLED);
	return 0;
}

#ifdef SWITCH_DBG
int do_sw_init(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	rtl83xx_init();
	return 0;
}

U_BOOT_CMD(
        sw_init,   1,      0,      do_sw_init,
        "to init switch\n",
        "None\n"
);

int do_sw_delay(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int ext_port, tx_delay, rx_delay;
	rtk_api_ret_t ret;

	if (argc < 4) {
		printf("Usage:\nsw_delay %s\n", cmdtp->help);
		return -1;
	}
	ext_port = simple_strtoul(argv[1], NULL, 10);

	ret = rtk_port_rgmiiDelayExt_get(ext_port, &tx_delay, &rx_delay);
	printf("OLD VALUE(ext_port=%d, tx_delay=%d, rx_delay=%d) = %d\n",
		ext_port, tx_delay, rx_delay, ret);

	tx_delay = simple_strtoul(argv[2], NULL, 10);
	rx_delay = simple_strtoul(argv[3], NULL, 10);

	if ((ext_port > 2) || (tx_delay > 1) || (rx_delay > 7)) {
		printf("Usage:\nsw_delay %s\n", cmdtp->help);
		return -1;
	}
	printf("Set ext_port = %d, tx delay = %d, rx delay = %d\n",
		ext_port, tx_delay, rx_delay);
	ret = rtk_port_rgmiiDelayExt_set(ext_port, tx_delay, rx_delay);
	printf("rtk_port_rgmiiDelayExt_set() = %d\n", ret);

	return 0;
}

U_BOOT_CMD(
	sw_delay,	4,	1,	do_sw_delay,
	"set switch tx delay and rx delay",
	"[ext port] [tx delay] [rx delay]\n"
	"ext port : 0 - 2\ntx delay : 0 - 1\nrx delay : 0 - 7\n"
);

int do_sw_link(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned int ext_port, speed, pause_en, nway_en;
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;
	rtk_api_ret_t ret;

	if (argc < 5) {
		printf("Usage:\nsw_link %s\n", cmdtp->help);
		return -1;
	}

	ext_port = simple_strtoul(argv[1], NULL, 10);

	ret = rtk_port_macForceLinkExt_get(ext_port, &mode,&mac_cfg);
	printf("OLD VALUE(ext_port=%d, mode=%d,mac_cfg=[force=%d, speed=%d, "
		"duplex=%d, link=%d, nway=%d, txpause=%d, rxpause=%d]) = %d\n",
		ext_port, mode, mac_cfg.forcemode, mac_cfg.speed,
		mac_cfg.duplex, mac_cfg.link, mac_cfg.nway, mac_cfg.txpause,
		mac_cfg.rxpause, ret);

	speed = simple_strtoul(argv[2], NULL, 10);
	pause_en = simple_strtoul(argv[3], NULL, 10);
	nway_en = simple_strtoul(argv[4], NULL, 10);

	mode = MODE_EXT_RGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = speed;
	mac_cfg.duplex = FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = nway_en;
	mac_cfg.txpause = pause_en;
	mac_cfg.rxpause = pause_en;



	if ((ext_port > 2) || (speed > 2) || (pause_en > 1) || (nway_en > 1)) {
		printf("Usage:\nsw_link %s\n", cmdtp->help);
		return -1;
	}
	printf("Set ext_port = %d, speed = %d, pause_en = %d, nway_en = %d\n",
		ext_port, speed, pause_en, nway_en);
	ret = rtk_port_macForceLinkExt_set(ext_port, mode,&mac_cfg);
	printf("rtk_port_macForceLinkExt_set() = %d\n", ret);

	return 0;
}

U_BOOT_CMD(
	sw_link,	5,	1,	do_sw_link,
	"set switch link speed of CPU port",
	"[ext port] [speed] [pause_en] [nway]\n"
	"ext port : 0 - 2\nspeed : 0 - 2 (10/100/1000)\n"
	"pause_en : 0 - 1 (disable/enable)\nnway : 0 - 1\n"
);
#endif /* SWITCH_DBG */

int do_sw_reg(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	rtk_api_ret_t ret;
	unsigned int reg, val;

	if (argc < 2) {
		printf("Usage:\nsw_reg %s\n", cmdtp->help);
		return -1;
	}
	reg = simple_strtoul(argv[1], NULL, 10);

	if (argc == 2) {
		/* read cmd */
		ret = smi_read(reg, &val);
		if (!ret)
			printf("switch register 0x%04X = 0x%04X\n", reg, val);
		else
			printf("Can't read switch register 0x%04X, ret = %d\n", 
				reg, ret);
		
	} else {
		/* argc > 2*/
		/* write cmd */
		val = simple_strtoul(argv[2], NULL, 10);
		ret = smi_write(reg, val);
		if (!ret)
			printf("switch register 0x%04X = 0x%04X\n", reg, val);
		else
			printf("Can't write switch register 0x%04X as 0x%04X,"
				" ret = %d", reg, val, ret);
	}
	return 0;
}

U_BOOT_CMD(
	sw_reg,	3,	1,	do_sw_reg,
	"read/write switch register",
	"[reg addr] ([value])\n"
);

