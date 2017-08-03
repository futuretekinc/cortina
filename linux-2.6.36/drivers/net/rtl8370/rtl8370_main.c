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
 * $Id: rtl83xx_main.c,v 1.2.2.6 2011/12/13 07:39:12 ewang Exp $
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/delay.h>	/* mdelay() */
#include <linux/skbuff.h>
#include <linux/phy.h>
#include <linux/switch.h>
#include <linux/stat.h>
#include <linux/netdevice.h>

#include <rtk_api.h>
#include <rtk_api_ext.h>
#include <rtl8370_asicdrv_port.h>	/* speed/duplex/mode */
#include <rtl8370_asicdrv.h>	/* rtl8370_setAsicReg() */
#include <rtl8370_asicdrv_cputag.h>	/* rtl8370_setAsicCputagPosition() */
#include <rtl8370_asicdrv_vlan.h>	/* rtl8370_setAsicVlanFilter() */
//#include <smi.h>		/* smi_read()/smi_write() */
#include "cs752x_eth.h"		/* struct mac_info_t */
#include "rtl8370_ioctl.h"	/* ioctl command IDs */
#include "rtl8370_vb.h"	/* RTK_CMD_T */

#define	DRV_NAME		"rtl8370"
#define	DRV_DESCRIPTION	"RTL8370 gigabit ethernet switch"

#ifdef RTL83XX_DBG
#define PFX     "RTK"
#define MESSAGE(format, args...) printk(KERN_WARNING PFX \
	":%s:%d: " format, __func__, __LINE__ , ## args)
#else
#define MESSAGE(format, args...)
#endif

#define INIT(x) do { \
	if ((ret = x) != 0) { \
		MESSAGE("Initialize %s fail, ret = %d\n", #x, ret); \
		goto fail; \
	} \
} while(0)

/* global variables */
spinlock_t *sw_lock;
int *ni_driver_state; /* 1: ready, 0: initializing */

/* external variables */
extern struct net_device *ni_get_device(unsigned char port_id);
extern int ni_mdio_write(int phy_addr, int reg_addr, u16 value);
extern int ni_mdio_read(int phy_addr, int reg_addr);
extern unsigned char eth_mac[GE_PORT_NUM][6];

static int rtl83xx_sw_init(void)
{

	struct net_device *dev;
	mac_info_t *tp;

	dev = ni_get_device(CPU_PORT_ID);
	if (!dev) {
		printk("%s::Error! No net device.\n", __func__);
		return -EINVAL;
	}

	tp = netdev_priv(dev);

	sw_lock = tp->mdio_lock;
	ni_driver_state = &(tp->ni_driver_state);
	tp->sw_phy_mode = rtk_port_phyEnableAll_set;

	return 0;
}
static int rtl83xx_init_cpu_port(void)
{
	rtk_port_mac_ability_t mac_cfg;
	rtk_mode_ext_t mode;
	rtk_api_ret_t ret = 0;

	mode = MODE_EXT_RGMII;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = SPD_1000M;
	mac_cfg.duplex = FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = DISABLED;
	mac_cfg.rxpause = DISABLED;

	/*
	 * Ref/eng board connects ext0 to G2 in RGMII mode.
	 * valid txDelay for ref board : 0~1, and we choose 1.
	 * valid rxDelay for ref board : 2~7, and we choose 7.
	 * valid txDelay for eng board : 0~1, and we choose 1.
	 * valid rxDelay for eng board : 2~7, and we choose 7.
	 */
	INIT(rtk_port_macForceLinkExt1_set(mode, &mac_cfg));
	INIT(rtk_port_macForceLinkExt0_set(mode, &mac_cfg));
	INIT(rtk_port_rgmiiDelayExt1_set(1, 7));
	INIT(rtk_port_rgmiiDelayExt0_set(1, 7));

#if 0 /* don't init ext port 1 to avoid congestion occurs */
	/*
	 * Ref/eng board connects ext1 to external STB in MII mode.
	 * valid txDelay for ref board : 
	 * valid rxDelay for ref board : 
	 */
	mode = MODE_EXT_MII_MAC;
	mac_cfg.forcemode = MAC_FORCE;
	mac_cfg.speed = SPD_100M;
	mac_cfg.duplex = FULL_DUPLEX;
	mac_cfg.link = PORT_LINKUP;
	mac_cfg.nway = DISABLED;
	mac_cfg.txpause = DISABLED;
	mac_cfg.rxpause = DISABLED;
	INIT(rtk_port_macForceLinkExt0_set(1, mode, &mac_cfg));
	INIT(rtk_port_rgmiiDelayExt_set(1, 1, 7));
#endif

	return 0;
fail:
	MESSAGE("Initializaing switch CPU port fail\n");
	return -EPERM;
}

/*
 * disable_phy_addr0_response
 * workaround to disable response of PHY address 0.
 * It is only valid for Realtek RTL8211 series PHY.
 */
static void disable_phy_addr0_response(unsigned phy_addr)
{
	unsigned int val;

	spin_lock(sw_lock);

	/* REG31 write 0x0007, set to extension page */
	ni_mdio_write(phy_addr, 31, 0x0007);

	/* REG30 write 0x002C, set to extension page 44 */
	ni_mdio_write(phy_addr, 30, 0x002C);

	/* 
	 * REG27 write bit[2] =0
	 * disable response PHYAD=0  function.
	 * we should read REG27 and clear bit[2], and write back.
	 */
	val = ni_mdio_read(phy_addr, 27);
	val &= ~(1 << 2);
	ni_mdio_write(phy_addr, 27, val);

	/* REG31 write 0X0000, back to page0 */
	ni_mdio_write(phy_addr, 31, 0x0007);

	spin_unlock(sw_lock);

}

static int rtl83xx_add_cpu_mac(void)
{
	rtk_api_ret_t ret = 0;
	rtk_l2_ucastAddr_t l2_data;
	rtk_mac_t mac;

	memset(&l2_data, 0, sizeof(rtk_l2_ucastAddr_t));
	memcpy(mac.octet, eth_mac[CPU_PORT_ID], ETHER_ADDR_LEN);

	l2_data.port = SWITCH_CPU_PORT;
	l2_data.is_static = 1;

	INIT(rtk_l2_addr_add(&mac, &l2_data));

	return 0;
fail:
	MESSAGE("Add CPU MAC to switch fail\n");
	return -EPERM;
}
static int rtl83xx_hw_init(void)
{
	rtk_api_ret_t ret = 0;
	unsigned int vendor_id, chip_id, phy_id;
	rtk_svlan_memberCfg_t svlan_cfg;
	rtk_portmask_t portmask, portmask2;
	int i, cnt = 0;
#ifndef RTL83XX_L2_ISOLATION
	unsigned int acl_rule_num;
	rtk_filter_field_t acl_field;
	rtk_filter_cfg_t acl_cfg;
	rtk_filter_action_t acl_act;
#endif

	MESSAGE("CPU_PORT_ID : %d\n", CPU_PORT_ID);
	/* Disable response PHYAD=0 function of RTL8211 series PHY */
	if (CPU_PORT_ID != 0) {
		/* check PHY of Golden Gate port 0 */
		printk("check PHY of Golden Gate port 0\n");
		spin_lock(sw_lock);
		vendor_id = ni_mdio_read(GE_PORT0_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT0_PHY_ADDR, 3);
		spin_unlock(sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT0_PHY_ADDR);
	}
	if (CPU_PORT_ID != 1) {
		/* check PHY of Golden Gate port 1 */
		printk("check PHY of Golden Gate port 1\n");
		spin_lock(sw_lock);
		vendor_id = ni_mdio_read(GE_PORT1_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT1_PHY_ADDR, 3);
		spin_unlock(sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT1_PHY_ADDR);
	}
	printk("check PHY of Golden Gate port 2\n");
	if (CPU_PORT_ID != 2) {
		/* check PHY of Golden Gate port 2 */
		spin_lock(sw_lock);
		vendor_id = ni_mdio_read(GE_PORT2_PHY_ADDR, 2);
		chip_id = ni_mdio_read(GE_PORT2_PHY_ADDR, 3);
		spin_unlock(sw_lock);
		phy_id = vendor_id << 16 | chip_id;
		if ((phy_id & PHY_ID_MASK) == PHY_ID_RTL8211)
			disable_phy_addr0_response(GE_PORT2_PHY_ADDR);
	}

	/*
	 * Reset Switch
	 * It is not necessary since there is no conflict with 
	 * current application in U-BOOT, and we could remove it in the future.
	 */
	rtl8370_setAsicReg(0x1322, 1);
	mdelay(1000); /* wait 1 sec */

	/*
	 * Init switch chip
	 */
	INIT(rtk_switch_init());
	INIT(rtk_port_phyEnableAll_set(DISABLED));
	INIT(rtk_qos_init(8));
	INIT(rtk_vlan_init());
	INIT(rtk_svlan_init());
	INIT(rtk_stp_init());
	INIT(rtk_l2_init());
	INIT(rtk_filter_igrAcl_init());
	INIT(rtk_eee_init());
	//INIT(rtk_igmp_init()); -- XTRA
	//portmask.bits[0] = (1 << SWITCH_CPU_PORT);
	//INIT(rtk_igmp_static_router_port_set(portmask)); -- XTRA
	INIT(rtk_stat_global_reset());
	INIT(rtl83xx_init_cpu_port());
	//INIT(rtl83xx_add_cpu_mac());

	/*
	 * LED configuration
	 */

	portmask.bits[0] = 0x7F;
	INIT(rtk_led_enable_set(LED_GROUP_0, portmask));
	INIT(rtk_led_enable_set(LED_GROUP_1, portmask));
	INIT(rtk_led_enable_set(LED_GROUP_2, portmask));

	INIT(rtk_led_operation_set(LED_OP_PARALLEL));
	//INIT(rtl8370_setAsicReg(0x1B00, 0x1471));
	/*
	 * LED function selection
	 * Engineering board
	 *	LED0: Link/Act
	 *	LED1: Speed 1000M
	 *	LED2: Speed 100M
	 */
	INIT(rtl8370_setAsicReg(0x1B03, 0x0320));
	
	INIT(rtk_led_groupConfig_set(LED_GROUP_0, LED_CONFIG_LINK_ACT));
	INIT(rtk_led_groupConfig_set(LED_GROUP_1, LED_CONFIG_SPD1000));
	INIT(rtk_led_groupConfig_set(LED_GROUP_2, LED_CONFIG_SPD100));
	 
	//INIT(rtk_cpu_enable_set(ENABLED));
	//INIT(rtk_cpu_tagPort_set(SWITCH_CPU_PORT, CPU_INSERT_TO_ALL));
	//INIT(rtk_vlan_init());
#if defined(CONFIG_CORTINA_REFERENCE) || defined(CONFIG_CORTINA_REFERENCE_B)

#ifdef CONFIG_CS752X_VIRTUAL_NI_CPUTAG
	/* 
	 * Postion to insert CPU tag
	 * 1: After entire packet(before CRC field)
	 * 0: After MAC_SA (Default)
	 */
	INIT(rtl8370_setAsicCputagPosition(1));
	INIT(rtk_cpu_enable_set(ENABLED));
	INIT(rtk_cpu_tagPort_set(SWITCH_CPU_PORT, CPU_INSERT_TO_ALL));

#ifdef RTL83XX_L2_ISOLATION
	/* Each LAN port only can forward to CPU port */
	for (i = 0; i < RTK_PHY_ID_MAX + 1; i++) {
		portmask.bits[0] = (1 << SWITCH_CPU_PORT) | (1 << i);
		INIT(rtk_port_isolation_set(i, portmask));

		/* Disable learning */
		INIT(rtk_l2_limitLearningCnt_set(i, 0));
	}

	/* Disable learning */
	INIT(rtk_l2_limitLearningCnt_set(SWITCH_CPU_PORT, 0));
#endif /* RTL83XX_L2_ISOLATION */

#endif /* CONFIG_CS752X_VIRTUAL_NI_CPUTAG */

#ifdef CONFIG_CS752X_VIRTUAL_NI_DBLTAG

#if defined(CONFIG_CS752X_VIRTUAL_ETH0)
#define SVID_START	CONFIG_CS752X_VID_START_ETH0
#define SW_PORT_NUM	CONFIG_CS752X_NR_VIRTUAL_ETH0
#elif defined(CONFIG_CS752X_VIRTUAL_ETH1)
#define SVID_START	CONFIG_CS752X_VID_START_ETH1
#define SW_PORT_NUM	CONFIG_CS752X_NR_VIRTUAL_ETH1
#elif defined(CONFIG_CS752X_VIRTUAL_ETH2)
#define SVID_START	CONFIG_CS752X_VID_START_ETH2
#define SW_PORT_NUM	CONFIG_CS752X_NR_VIRTUAL_ETH2
#endif

#if !defined(SVID_START) || (SVID_START < 1) || (SVID_START >= 4095)
#define SVID_START	1
#endif

#if !defined(SW_PORT_NUM) || (SW_PORT_NUM < 1) || (SW_PORT_NUM > RTK_PHY_ID_MAX + 1)
#define SW_PORT_NUM	1
#endif

	/* enable SVLAN for internal use */
	INIT(rtk_svlan_servicePort_add(SWITCH_CPU_PORT));

	memset(&svlan_cfg, 0, sizeof(rtk_svlan_memberCfg_t));

	for (i = 0; i < SW_PORT_NUM; i++) {
		/*
		 * Create SVLAN rule for a LAN port,
		 * each LAN port only can forward to CPU port
		 */
		svlan_cfg.svid = SVID_START + i;
#ifdef RTL83XX_L2_ISOLATION
		svlan_cfg.memberport = (1 << SWITCH_CPU_PORT) | (1 << i);
		svlan_cfg.untagport = 1 << i;
#else /* RTL83XX_L2_ISOLATION */
		svlan_cfg.memberport = 0x3F;
		svlan_cfg.untagport = 0x1F;
#endif /* RTL83XX_L2_ISOLATION */
		INIT(rtk_svlan_memberPortEntry_set(SVID_START + i, &svlan_cfg));

		/* All packets from a LAN port will be added the SVID */
		INIT(rtk_svlan_defaultSvlan_set(i, SVID_START + i));

	}

#ifdef RTL83XX_L2_ISOLATION
	/* Each LAN port only can forward to CPU port */
	for (i = 0; i < SW_PORT_NUM; i++) {
		portmask.bits[0] = (1 << SWITCH_CPU_PORT) | (1 << i);
		INIT(rtk_port_isolation_set(i, portmask));

		/* Disable learning */
		INIT(rtk_l2_limitLearningCnt_set(i, 0));
	}

	/* Disable learning */
	INIT(rtk_l2_limitLearningCnt_set(SWITCH_CPU_PORT, 0));
#else
	/*
	 * set ACL rules on CPU port
	 * symptom: bridge interface in Linux will duplicate BC/MC/UU to 
	 *	    each virtual interface and then one switch LAN port will 
	 *	    receive up to 4 copies of the same packet.
	 * solution: to redirect incoming packets to one LAN port only and
	 * 	    avoid forwarding duplicate BC/MC/UU to one LAN port
	 */	    
	for (i = 0; i < SW_PORT_NUM; i++) {
		memset(&acl_field, 0, sizeof(rtk_filter_field_t));
		memset(&acl_cfg, 0, sizeof(rtk_filter_cfg_t));
		memset(&acl_act, 0, sizeof(rtk_filter_action_t));

		/* check SVID in packet */
		acl_field.fieldType = FILTER_FIELD_STAG;
		acl_field.filter_pattern_union.stag.vid.dataType =
			FILTER_FIELD_DATA_MASK;
		acl_field.filter_pattern_union.stag.vid.value = SVID_START + i;
		acl_field.filter_pattern_union.stag.vid.mask = 0xFFF;
		INIT(rtk_filter_igrAcl_field_add(&acl_cfg, &acl_field));

		/* only take effect on ingress packet of CPU port */
		acl_cfg.activeport.dataType = FILTER_FIELD_DATA_MASK;
		acl_cfg.activeport.value = (1 << SWITCH_CPU_PORT);
		acl_cfg.activeport.mask = (1 << SWITCH_CPU_PORT);

		/* only care packets with SVID */
		acl_cfg.careTag.tagType[CARE_TAG_STAG].value = ENABLED;
		acl_cfg.careTag.tagType[CARE_TAG_STAG].mask = ENABLED;

		/* redirect packets to specific LAN port */
		acl_act.actEnable[FILTER_ENACT_REDIRECT] = ENABLED;
		acl_act.filterRedirectPortmask = (1 << i);

		INIT(rtk_filter_igrAcl_cfg_add(i, &acl_cfg, &acl_act,
			&acl_rule_num));
	}
#endif /* RTL83XX_L2_ISOLATION */

	INIT(rtk_svlan_unmatch_action_set(UNMATCH_DROP, 0));

#endif /* CONFIG_CS752X_VIRTUAL_NI_DBLTAG */

#endif /*defined(CONFIG_CORTINA_REFERENCE)||defined(CONFIG_CORTINA_ENGINEERING)*/

        //Disable VLAN member set filtering
        INIT(rtl8370_setAsicVlanFilter(DISABLED));

#if defined(CONFIG_CORTINA_REFERENCE_B)
	memset(&phy_ability, 0, sizeof(rtk_port_phy_ability_t));

	/* only disable Full_1000 capability but enable others */
	phy_ability.AutoNegotiation = 1;
	phy_ability.Half_10 = 1;
	phy_ability.Full_10 = 1;
	phy_ability.Half_100 = 1;
	phy_ability.Full_100 = 1;
	phy_ability.FC = 1;
	
	INIT(rtk_port_phyAutoNegoAbility_set(SWITCH_STB_PORT, &phy_ability));
#endif

	INIT(rtk_port_phyEnableAll_set(ENABLED));

	return 0;
fail:
	MESSAGE("Initialization fail\n");
	return -EPERM;
}

static long rtl83xx_ioctl(struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	RTK_CMD_T rtk_cmd;
	rtk_api_ret_t ret = 0;
	int i;

	if (cmd != SIOCDEVPRIVATE) {
	    MESSAGE("It is not private command (0x%X). cmd = (0x%X)\n",
		    SIOCDEVPRIVATE, cmd);
	    return -EOPNOTSUPP;
	}

	if (copy_from_user((void *)&rtk_cmd, argp, sizeof(rtk_cmd))) {
	    MESSAGE("Copy from user space fail\n");
	    return -EFAULT;
	}

#ifdef CONFIG_CORTINA_FPGA
	if (rtk_cmd.cmd < RTK_MAX) {
		printk("Receive IOCTL command %d\n", rtk_cmd.cmd);
		rtk_cmd.ret = RT_ERR_OK;
		return 0;
	}
#endif

	switch (rtk_cmd.cmd) {
	case RTK_SWITCH_INIT:
		ret = rtk_switch_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_SWITCH_MAX_PKTLEN_SET:
		ret = rtk_switch_maxPktLen_set(
			rtk_cmd.para.switch_max_pktlen.len);
		rtk_cmd.ret = ret;
		break;
	case RTK_SWITCH_MAX_PKTLEN_GET:
		ret = rtk_switch_maxPktLen_get(
			&rtk_cmd.para.switch_max_pktlen.len);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_IGR_BW_SET:
		ret = rtk_rate_igrBandwidthCtrlRate_set(
			rtk_cmd.para.rate_igr_bw.port,
			rtk_cmd.para.rate_igr_bw.rate,
			rtk_cmd.para.rate_igr_bw.ifg_include,
			rtk_cmd.para.rate_igr_bw.fc_enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_IGR_BW_GET:
		ret = rtk_rate_igrBandwidthCtrlRate_get(
			rtk_cmd.para.rate_igr_bw.port,
			&rtk_cmd.para.rate_igr_bw.rate,
			&rtk_cmd.para.rate_igr_bw.ifg_include,
			&rtk_cmd.para.rate_igr_bw.fc_enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_BW_SET:
		ret = rtk_rate_egrBandwidthCtrlRate_set(
			rtk_cmd.para.rate_egr_bw.port,
			rtk_cmd.para.rate_egr_bw.rate,
			rtk_cmd.para.rate_egr_bw.ifg_include
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_BW_GET:
		ret = rtk_rate_egrBandwidthCtrlRate_get(
			rtk_cmd.para.rate_egr_bw.port,
			&rtk_cmd.para.rate_egr_bw.rate,
			&rtk_cmd.para.rate_egr_bw.ifg_include
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_EN_SET:
		ret = rtk_rate_egrQueueBwCtrlEnable_set(
			rtk_cmd.para.rate_egr_qbw_en.port,
			rtk_cmd.para.rate_egr_qbw_en.queue,
			rtk_cmd.para.rate_egr_qbw_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_EN_GET:
		ret = rtk_rate_egrQueueBwCtrlEnable_get(
			rtk_cmd.para.rate_egr_qbw_en.port,
			rtk_cmd.para.rate_egr_qbw_en.queue,
			&rtk_cmd.para.rate_egr_qbw_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_SET:
		ret = rtk_rate_egrQueueBwCtrlRate_set(
			rtk_cmd.para.rate_egr_qbw.port,
			rtk_cmd.para.rate_egr_qbw.queue,
			rtk_cmd.para.rate_egr_qbw.index
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_RATE_EGR_QBW_GET:
		ret = rtk_rate_egrQueueBwCtrlRate_get(
			rtk_cmd.para.rate_egr_qbw.port,
			rtk_cmd.para.rate_egr_qbw.queue,
			&rtk_cmd.para.rate_egr_qbw.index
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_CTRL_RATE_SET:
		ret = rtk_storm_controlRate_set(
			rtk_cmd.para.storm_ctrl_rate.port,
			rtk_cmd.para.storm_ctrl_rate.storm_type,
			rtk_cmd.para.storm_ctrl_rate.rate,
			rtk_cmd.para.storm_ctrl_rate.ifg_include,
			rtk_cmd.para.storm_ctrl_rate.mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_CTRL_RATE_GET:
		ret = rtk_storm_controlRate_get(
			rtk_cmd.para.storm_ctrl_rate.port,
			rtk_cmd.para.storm_ctrl_rate.storm_type,
			&rtk_cmd.para.storm_ctrl_rate.rate,
			&rtk_cmd.para.storm_ctrl_rate.ifg_include,
			rtk_cmd.para.storm_ctrl_rate.mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_BYPASS_SET:
		ret = rtk_storm_bypass_set(
			rtk_cmd.para.storm_bypass.type,
			rtk_cmd.para.storm_bypass.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STORM_BYPASS_GET:
		ret = rtk_storm_bypass_get(
			rtk_cmd.para.storm_bypass.type,
			&rtk_cmd.para.storm_bypass.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_INIT:
		ret = rtk_qos_init(
			rtk_cmd.para.qos.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_SEL_SET:
		ret = rtk_qos_priSel_set(
			&rtk_cmd.para.qos_pri_sel.priDec
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_SEL_GET:
		ret = rtk_qos_priSel_get(
			&rtk_cmd.para.qos_pri_sel.priDec
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_PRI_REMAP_SET:
		ret = rtk_qos_1pPriRemap_set(
			rtk_cmd.para.qos_dot1p_pri_remap.dot1p_pri,
			rtk_cmd.para.qos_dot1p_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_PRI_REMAP_GET:
		ret = rtk_qos_1pPriRemap_get(
			rtk_cmd.para.qos_dot1p_pri_remap.dot1p_pri,
			&rtk_cmd.para.qos_dot1p_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_PRI_REMAP_SET:
		ret = rtk_qos_dscpPriRemap_set(
			rtk_cmd.para.qos_dscp_pri_remap.dscp,
			rtk_cmd.para.qos_dscp_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_PRI_REMAP_GET:
		ret = rtk_qos_dscpPriRemap_get(
			rtk_cmd.para.qos_dscp_pri_remap.dscp,
			&rtk_cmd.para.qos_dscp_pri_remap.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PORT_PRI_SET:
		ret = rtk_qos_portPri_set(
			rtk_cmd.para.qos_port_pri.port,
			rtk_cmd.para.qos_port_pri.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PORT_PRI_GET:
		ret = rtk_qos_portPri_get(
			rtk_cmd.para.qos_port_pri.port,
			&rtk_cmd.para.qos_port_pri.int_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_QUE_NUM_SET:
		ret = rtk_qos_queueNum_set(
			rtk_cmd.para.qos_que_num.port,
			rtk_cmd.para.qos_que_num.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_QUE_NUM_GET:
		ret = rtk_qos_queueNum_get(
			rtk_cmd.para.qos_que_num.port,
			&rtk_cmd.para.qos_que_num.queue_num
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_MAP_SET:
		ret = rtk_qos_priMap_set(
			rtk_cmd.para.qos_pri_map.queue_num,
			&rtk_cmd.para.qos_pri_map.pri2qid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_PRI_MAP_GET:
		ret = rtk_qos_priMap_get(
			rtk_cmd.para.qos_pri_map.queue_num,
			&rtk_cmd.para.qos_pri_map.pri2qid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_SCHE_QUE_SET:
		ret = rtk_qos_schedulingQueue_set(
			rtk_cmd.para.qos_sche_que.port,
			&rtk_cmd.para.qos_sche_que.qweights
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_SCHE_QUE_GET:
		ret = rtk_qos_schedulingQueue_get(
			rtk_cmd.para.qos_sche_que.port,
			&rtk_cmd.para.qos_sche_que.qweights
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_EN_SET:
		ret = rtk_qos_1pRemarkEnable_set(
			rtk_cmd.para.qos_dot1p_remark_en.port,
			rtk_cmd.para.qos_dot1p_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_EN_GET:
		ret = rtk_qos_1pRemarkEnable_get(
			rtk_cmd.para.qos_dot1p_remark_en.port,
			&rtk_cmd.para.qos_dot1p_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_SET:
		ret = rtk_qos_1pRemark_set(
			rtk_cmd.para.qos_dot1p_remark.int_pri,
			rtk_cmd.para.qos_dot1p_remark.dot1p_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DOT1P_REMARK_GET:
		ret = rtk_qos_1pRemark_get(
			rtk_cmd.para.qos_dot1p_remark.int_pri,
			&rtk_cmd.para.qos_dot1p_remark.dot1p_pri
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_EN_SET:
		ret = rtk_qos_dscpRemarkEnable_set(
			rtk_cmd.para.qos_dscp_remark_en.port,
			rtk_cmd.para.qos_dscp_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_EN_GET:
		ret = rtk_qos_dscpRemarkEnable_get(
			rtk_cmd.para.qos_dscp_remark_en.port,
			&rtk_cmd.para.qos_dscp_remark_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_SET:
		ret = rtk_qos_dscpRemark_set(
			rtk_cmd.para.qos_dscp_remark.int_pri,
			rtk_cmd.para.qos_dscp_remark.dscp
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_QOS_DSCP_REMARK_GET:
		ret = rtk_qos_dscpRemark_get(
			rtk_cmd.para.qos_dscp_remark.int_pri,
			&rtk_cmd.para.qos_dscp_remark.dscp
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_AN_ABILITY_SET:
		ret = rtk_port_phyAutoNegoAbility_set(
			rtk_cmd.para.port_phy_an_ability.port,
			&rtk_cmd.para.port_phy_an_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_AN_ABILITY_GET:
		ret = rtk_port_phyAutoNegoAbility_get(
			rtk_cmd.para.port_phy_an_ability.port,
			&rtk_cmd.para.port_phy_an_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_FORCE_ABILITY_SET:
		ret = rtk_port_phyForceModeAbility_set(
			rtk_cmd.para.port_phy_force_ability.port,
			&rtk_cmd.para.port_phy_force_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_FORCE_ABILITY_GET:
		ret = rtk_port_phyForceModeAbility_get(
			rtk_cmd.para.port_phy_force_ability.port,
			&rtk_cmd.para.port_phy_force_ability.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHY_STATUS_GET:
		ret = rtk_port_phyStatus_get(
			rtk_cmd.para.port_phy_status.port,
			&rtk_cmd.para.port_phy_status.linkStatus,
			&rtk_cmd.para.port_phy_status.speed,
			&rtk_cmd.para.port_phy_status.duplex
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACFORCELINKEXT_SET:
		ret = rtk_port_macForceLinkExt0_set(
			rtk_cmd.para.port_mac_force_link_ext.mode,
			&rtk_cmd.para.port_mac_force_link_ext.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACFORCELINKEXT_GET:
		ret = rtk_port_macForceLinkExt0_get(
			&rtk_cmd.para.port_mac_force_link_ext.mode,
			&rtk_cmd.para.port_mac_force_link_ext.ability
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_MACSTATUS_GET:
		ret = rtk_port_macStatus_get(
			rtk_cmd.para.port_mac_status.port,
			&rtk_cmd.para.port_mac_status.status
			);
		rtk_cmd.ret = ret;
		break;

	case RTK_PORT_PHYREG_SET:
		ret = rtk_port_phyReg_set(
			rtk_cmd.para.port_phy_reg.port,
			rtk_cmd.para.port_phy_reg.reg,
			rtk_cmd.para.port_phy_reg.data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_PHYREG_GET:
		ret = rtk_port_phyReg_get(
			rtk_cmd.para.port_phy_reg.port,
			rtk_cmd.para.port_phy_reg.reg,
			&rtk_cmd.para.port_phy_reg.data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ADMIN_STATE_SET:
		ret = rtk_port_adminEnable_set(
			rtk_cmd.para.port_admin_state.port,
			rtk_cmd.para.port_admin_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ADMIN_STATE_GET:
		ret = rtk_port_adminEnable_get(
			rtk_cmd.para.port_admin_state.port,
			&rtk_cmd.para.port_admin_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ISOLATION_SET:
		ret = rtk_port_isolation_set(
			rtk_cmd.para.port_isolation.port,
			rtk_cmd.para.port_isolation.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ISOLATION_GET:
		ret = rtk_port_isolation_get(
			rtk_cmd.para.port_isolation.port,
			&rtk_cmd.para.port_isolation.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT0_SET:
		ret = rtk_port_rgmiiDelayExt0_set(
			rtk_cmd.para.port_rgmii_delay.txDelay,
			rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT0_GET:
		ret = rtk_port_rgmiiDelayExt0_get(
			&rtk_cmd.para.port_rgmii_delay.txDelay,
			&rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT1_SET:
		ret = rtk_port_rgmiiDelayExt1_set(
			rtk_cmd.para.port_rgmii_delay.txDelay,
			rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT1_GET:
		ret = rtk_port_rgmiiDelayExt1_get(
			&rtk_cmd.para.port_rgmii_delay.txDelay,
			&rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT_SET:
		ret = rtk_port_rgmiiDelayExt0_set(
			rtk_cmd.para.port_rgmii_delay.txDelay,
			rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_RGMIIDELAYEXT_GET:
		ret = rtk_port_rgmiiDelayExt0_get(
			&rtk_cmd.para.port_rgmii_delay.txDelay,
			&rtk_cmd.para.port_rgmii_delay.rxDelay
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ENABLE_ALL_SET:
		ret = rtk_port_phyEnableAll_set(
			rtk_cmd.para.port_enable_all.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_PORT_ENABLE_ALL_GET:
		ret = rtk_port_phyEnableAll_get(
			&rtk_cmd.para.port_enable_all.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_INIT:
		ret = rtk_vlan_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_SET:
		ret = rtk_vlan_set(
			rtk_cmd.para.vlan.vid,
			rtk_cmd.para.vlan.mbrmsk,
			rtk_cmd.para.vlan.untagmsk,
			rtk_cmd.para.vlan.fid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_GET:
		ret = rtk_vlan_get(
			rtk_cmd.para.vlan.vid,
			&rtk_cmd.para.vlan.mbrmsk,
			&rtk_cmd.para.vlan.untagmsk,
			&rtk_cmd.para.vlan.fid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PVID_SET:
		ret = rtk_vlan_portPvid_set(
			rtk_cmd.para.vlan_pvid.port,
			rtk_cmd.para.vlan_pvid.pvid,
			rtk_cmd.para.vlan_pvid.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PVID_GET:
		ret = rtk_vlan_portPvid_get(
			rtk_cmd.para.vlan_pvid.port,
			&rtk_cmd.para.vlan_pvid.pvid,
			&rtk_cmd.para.vlan_pvid.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_IGRFILTER_EN_SET:
		ret = rtk_vlan_portIgrFilterEnable_set(
			rtk_cmd.para.vlan_port_igrfilter_en.port,
			rtk_cmd.para.vlan_port_igrfilter_en.igr_filter
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_IGRFILTER_EN_GET:
		ret = rtk_vlan_portIgrFilterEnable_get(
			rtk_cmd.para.vlan_port_igrfilter_en.port,
			&rtk_cmd.para.vlan_port_igrfilter_en.igr_filter
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_AFT_SET:
		ret = rtk_vlan_portAcceptFrameType_set(
			rtk_cmd.para.vlan_port_aft.port,
			rtk_cmd.para.vlan_port_aft.accept_frame_type
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_PORT_AFT_GET:
		ret = rtk_vlan_portAcceptFrameType_get(
			rtk_cmd.para.vlan_port_aft.port,
			&rtk_cmd.para.vlan_port_aft.accept_frame_type
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_BASED_PRI_SET:
		ret = rtk_vlan_vlanBasedPriority_set(
			rtk_cmd.para.vlan_based_pri.vid,
			rtk_cmd.para.vlan_based_pri.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_BASED_PRI_GET:
		ret = rtk_vlan_vlanBasedPriority_get(
			rtk_cmd.para.vlan_based_pri.vid,
			&rtk_cmd.para.vlan_based_pri.priority
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_TAGMODE_SET:
		ret = rtk_vlan_tagMode_set(
			rtk_cmd.para.vlan_tagmode.port,
			rtk_cmd.para.vlan_tagmode.tag_mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_TAGMODE_GET:
		ret = rtk_vlan_tagMode_get(
			rtk_cmd.para.vlan_tagmode.port,
			&rtk_cmd.para.vlan_tagmode.tag_mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_MBR_FILTER_SET:
		ret = rtl8370_setAsicVlanFilter(
			rtk_cmd.para.vlan_mbr_filter.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_VLAN_MBR_FILTER_GET:
		ret = rtl8370_getAsicVlanFilter(
			&rtk_cmd.para.vlan_mbr_filter.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_INIT:
		ret = rtk_stp_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_MSTP_STATE_SET:
		ret = rtk_stp_mstpState_set(
			rtk_cmd.para.stp_mstp_state.msti,
			rtk_cmd.para.stp_mstp_state.port,
			rtk_cmd.para.stp_mstp_state.stp_state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STP_MSTP_STATE_GET:
		ret = rtk_stp_mstpState_get(
			rtk_cmd.para.stp_mstp_state.msti,
			rtk_cmd.para.stp_mstp_state.port,
			&rtk_cmd.para.stp_mstp_state.stp_state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_INIT:
		ret = rtk_l2_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_ADD:
		ret = rtk_l2_addr_add(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_ADDR_GET:
		ret = rtk_l2_addr_get(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
#if 0	// XTRA
	case RTK_L2_ADDR_NEXT_GET:
		ret = rtk_l2_addr_next_get(
			rtk_cmd.para.l2_addr.read_method,
			rtk_cmd.para.l2_addr.port,
			&rtk_cmd.para.l2_addr.address,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
#endif
	case RTK_L2_ADDR_DEL:
		ret = rtk_l2_addr_del(
			&rtk_cmd.para.l2_addr.mac,
			&rtk_cmd.para.l2_addr.l2_data
			);
		rtk_cmd.ret = ret;
		break;
#if 0	// XTRA
	case RTK_L2_MCADDR_ADD:
		ret = rtk_l2_mcastAddr_add(
			&rtk_cmd.para.l2_mcaddr.mac,
			rtk_cmd.para.l2_mcaddr.ivl,
			rtk_cmd.para.l2_mcaddr.cvid_fid,
			rtk_cmd.para.l2_mcaddr.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_GET:
		ret = rtk_l2_mcastAddr_get(
			&rtk_cmd.para.l2_mcaddr.mac,
			rtk_cmd.para.l2_mcaddr.ivl,
			rtk_cmd.para.l2_mcaddr.cvid_fid,
			&rtk_cmd.para.l2_mcaddr.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_NEXT_GET:
		ret = rtk_l2_mcastAddr_next_get(
			&rtk_cmd.para.l2_mcaddr.address,
			&rtk_cmd.para.l2_mcaddr.mac,
			&rtk_cmd.para.l2_mcaddr.ivl,
			&rtk_cmd.para.l2_mcaddr.cvid_fid,
			&rtk_cmd.para.l2_mcaddr.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_MCADDR_DEL:
		ret = rtk_l2_mcastAddr_del(
			&rtk_cmd.para.l2_mcaddr.mac,
			rtk_cmd.para.l2_mcaddr.ivl,
			rtk_cmd.para.l2_mcaddr.cvid_fid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_ENABLE_SET:
		ret = rtk_l2_agingEnable_set(
			rtk_cmd.para.l2_aging_en.port,
			rtk_cmd.para.l2_aging_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_ENABLE_GET:
		ret = rtk_l2_agingEnable_get(
			rtk_cmd.para.l2_aging_en.port,
			&rtk_cmd.para.l2_aging_en.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_LMT_SET:
		ret = rtk_l2_limitLearningCnt_set(
			rtk_cmd.para.l2_lrn_lmt.port,
			rtk_cmd.para.l2_lrn_lmt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_LMT_GET:
		ret = rtk_l2_limitLearningCnt_get(
			rtk_cmd.para.l2_lrn_lmt.port,
			&rtk_cmd.para.l2_lrn_lmt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_ACTION_SET:
		ret = rtk_l2_limitLearningCntAction_set(
			rtk_cmd.para.l2_lrn_action.port,
			rtk_cmd.para.l2_lrn_action.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_ACTION_GET:
		ret = rtk_l2_limitLearningCntAction_get(
			rtk_cmd.para.l2_lrn_action.port,
			&rtk_cmd.para.l2_lrn_action.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LRN_CNT_GET:
		ret = rtk_l2_learningCnt_get(
			rtk_cmd.para.l2_lrn_cnt.port,
			&rtk_cmd.para.l2_lrn_cnt.mac_cnt
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_FLOOD_PORTS_SET:
		ret = rtk_l2_floodPortMask_set(
			rtk_cmd.para.l2_flood_ports.flood_type,
			rtk_cmd.para.l2_flood_ports.flood_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_FLOOD_PORTS_GET:
		ret = rtk_l2_floodPortMask_get(
			rtk_cmd.para.l2_flood_ports.flood_type,
			&rtk_cmd.para.l2_flood_ports.flood_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LOCALPKT_PMT_SET:
		ret = rtk_l2_localPktPermit_set(
			rtk_cmd.para.l2_localpkt_pmt.port,
			rtk_cmd.para.l2_localpkt_pmt.permit
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_LOCALPKT_PMT_GET:
		ret = rtk_l2_localPktPermit_get(
			rtk_cmd.para.l2_localpkt_pmt.port,
			&rtk_cmd.para.l2_localpkt_pmt.permit
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_SET:
		ret = rtk_l2_aging_set(
			rtk_cmd.para.l2_aging.aging_time
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_L2_AGING_GET:
		ret = rtk_l2_aging_get(
			&rtk_cmd.para.l2_aging.aging_time
			);
		rtk_cmd.ret = ret;
		break;
#endif
	case RTK_SVLAN_INIT:
		ret = rtk_svlan_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_ADD:
		ret = rtk_svlan_servicePort_add(
			rtk_cmd.para.svlan_svc_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_GET:
		ret = rtk_svlan_servicePort_get(
			&rtk_cmd.para.svlan_svc_port.svlan_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_SVC_PORT_DEL:
		ret = rtk_svlan_servicePort_del(
			rtk_cmd.para.svlan_svc_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_MBRPORT_ENTRY_SET:
		ret = rtk_svlan_memberPortEntry_set(
			rtk_cmd.para.svlan_mbrport_entry.svid_idx,
			&rtk_cmd.para.svlan_mbrport_entry.svlan_cfg
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_MBRPORT_ENTRY_GET:
		ret = rtk_svlan_memberPortEntry_get(
			rtk_cmd.para.svlan_mbrport_entry.svid_idx,
			&rtk_cmd.para.svlan_mbrport_entry.svlan_cfg
			);
		rtk_cmd.ret = ret;
		break;
#if 0	// XTRA
	case RTK_SVLAN_DEF_SVID_SET:
		ret = rtk_svlan_defaultSvlan_set(
			rtk_cmd.para.svlan_def_svid.port,
			rtk_cmd.para.svlan_def_svid.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_DEF_SVID_GET:
		ret = rtk_svlan_defaultSvlan_get(
			rtk_cmd.para.svlan_def_svid.port,
			&rtk_cmd.para.svlan_def_svid.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_UNMATCH_ACTION_SET:
		ret = rtk_svlan_unmatch_action_set(
			rtk_cmd.para.svlan_unmatch_act.action,
			rtk_cmd.para.svlan_unmatch_act.svid
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_SVLAN_UNMATCH_ACTION_GET:
		ret = rtk_svlan_unmatch_action_get(
			&rtk_cmd.para.svlan_unmatch_act.action,
			&rtk_cmd.para.svlan_unmatch_act.svid
			);
		rtk_cmd.ret = ret;
		break;
#endif
	case RTK_CPU_ENABLE_SET:
		ret = rtk_cpu_enable_set(
			rtk_cmd.para.cpu_tag.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_ENABLE_GET:
		ret = rtk_cpu_enable_get(
			&rtk_cmd.para.cpu_tag.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_PORT_SET:
		ret = rtk_cpu_tagPort_set(
			rtk_cmd.para.cpu_tag_port.port,
			rtk_cmd.para.cpu_tag_port.mode
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_PORT_GET:
		ret = rtk_cpu_tagPort_get(
			&rtk_cmd.para.cpu_tag_port.port,
			&rtk_cmd.para.cpu_tag_port.mode
			);
		rtk_cmd.ret = ret;
		break;
#if 0	// XTRA
	case RTK_CPU_TAG_POSITION_SET:
		ret = rtl8370_setAsicCputagPosition(
			rtk_cmd.para.cpu_tag_position.position
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_CPU_TAG_POSITION_GET:
		ret = rtl8370_getAsicCputagPosition(
			&rtk_cmd.para.cpu_tag_position.position
			);
		rtk_cmd.ret = ret;
		break;
#endif
	case RTK_MIRROR_PORT_BASED_SET:
		ret = rtk_mirror_portBased_set(
			rtk_cmd.para.mirror_portbased.mirroring_port,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_BASED_GET:
		ret = rtk_mirror_portBased_get(
			&rtk_cmd.para.mirror_portbased.mirroring_port,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask,
			&rtk_cmd.para.mirror_portbased.mirrored_rx_portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_ISO_SET:
		ret = rtk_mirror_portIso_set(
			rtk_cmd.para.mirror_portiso.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_MIRROR_PORT_ISO_GET:
		ret = rtk_mirror_portIso_get(
			&rtk_cmd.para.mirror_portiso.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_RESET:
		ret = rtk_stat_global_reset();
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_RESET:
		ret = rtk_stat_port_reset(
			rtk_cmd.para.stat_port.port
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_GET:
		ret = rtk_stat_global_get(
			rtk_cmd.para.stat_global.cntr_idx,
			&rtk_cmd.para.stat_global.cntr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_GLOBAL_GETALL:
		ret = rtk_stat_global_getAll(
			&rtk_cmd.para.stat_global.global_cntrs
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_GET:
		ret = rtk_stat_port_get(
			rtk_cmd.para.stat_port.port,
			rtk_cmd.para.stat_port.cntr_idx,
			&rtk_cmd.para.stat_port.cntr
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_STAT_PORT_GETALL:
		ret = rtk_stat_port_getAll(
			rtk_cmd.para.stat_port.port,
			&rtk_cmd.para.stat_port.port_cntrs
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_INIT:
		ret = rtk_filter_igrAcl_init();
		rtk_cmd.ret = ret;
		break;
	/* We don't support to add acl filter field from user space.
	 * User should prepare filter fields as an array and use RTK_ACL_CFG_ADD
	 */
/*
	case RTK_ACL_FIELD_ADD:
		ret = rtk_filter_igrAcl_field_add(
			&rtk_cmd.para.acl_field.filter_cfg,
			&rtk_cmd.para.acl_field.filter_field
			);
		rtk_cmd.ret = ret;
		break;
*/
	/*
	 * User should prepare acl filter fields in array and assign field_num.
	 */
	case RTK_ACL_CFG_ADD:
		for (i = 0; i < rtk_cmd.para.acl_cfg.field_num; i++) {
			ret = rtk_filter_igrAcl_field_add(
				&rtk_cmd.para.acl_cfg.filter_cfg,
				&rtk_cmd.para.acl_cfg.filter_field[i]
				);
			if (ret) {
				rtk_cmd.ret = ret;
				break;
			}

		}
		ret = rtk_filter_igrAcl_cfg_add(
			rtk_cmd.para.acl_cfg.filter_id,
			&rtk_cmd.para.acl_cfg.filter_cfg,
			&rtk_cmd.para.acl_cfg.action,
			&rtk_cmd.para.acl_cfg.ruleNum
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_DEL:
		ret = rtk_filter_igrAcl_cfg_del(
			rtk_cmd.para.acl_cfg.filter_id
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_DELALL:
		ret = rtk_filter_igrAcl_cfg_delAll();
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_CFG_GET:
		ret = rtk_filter_igrAcl_cfg_get(
			rtk_cmd.para.acl_cfg.filter_id,
			&rtk_cmd.para.acl_cfg.filter_cfg_raw,
			&rtk_cmd.para.acl_cfg.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_UMACTION_SET:
		ret = rtk_filter_igrAcl_unmatchAction_set(
			rtk_cmd.para.acl_umaction.port,
			rtk_cmd.para.acl_umaction.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_UMACTION_GET:
		ret = rtk_filter_igrAcl_unmatchAction_get(
			rtk_cmd.para.acl_umaction.port,
			&rtk_cmd.para.acl_umaction.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_STATE_SET:
		ret = rtk_filter_igrAcl_state_set(
			rtk_cmd.para.acl_state.port,
			rtk_cmd.para.acl_state.state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_ACL_STATE_GET:
		ret = rtk_filter_igrAcl_state_get(
			rtk_cmd.para.acl_state.port,
			&rtk_cmd.para.acl_state.state
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_EEE_INIT:
		ret = rtk_eee_init();
		rtk_cmd.ret = ret;
		break;
#if 0	// XTRA
	case RTK_IGMP_INIT:
		ret = rtk_igmp_init();
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATE_SET:
		ret = rtk_igmp_state_set(
			rtk_cmd.para.igmp_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATE_GET:
		ret = rtk_igmp_state_get(
			&rtk_cmd.para.igmp_state.enable
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATIC_ROUTER_PORT_SET:
		ret = rtk_igmp_static_router_port_set(
			rtk_cmd.para.igmp_router_port.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_STATIC_ROUTER_PORT_GET:
		ret = rtk_igmp_static_router_port_get(
			&rtk_cmd.para.igmp_router_port.portmask
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_PROTOCOL_SET:
		ret = rtk_igmp_protocol_set(
			rtk_cmd.para.igmp_protocol.port,
			rtk_cmd.para.igmp_protocol.protocol,
			rtk_cmd.para.igmp_protocol.action
			);
		rtk_cmd.ret = ret;
		break;
	case RTK_IGMP_PROTOCOL_GET:
		ret = rtk_igmp_protocol_get(
			rtk_cmd.para.igmp_protocol.port,
			rtk_cmd.para.igmp_protocol.protocol,
			&rtk_cmd.para.igmp_protocol.action
			);
		rtk_cmd.ret = ret;
		break;
#endif
	/******* MDIO READ/WRITE CMD **************/
	case PHY_REG_READ:
		spin_lock(sw_lock);
		rtk_cmd.para.mdio.data = ni_mdio_read(
			rtk_cmd.para.mdio.phy_addr,
			rtk_cmd.para.mdio.reg_addr
			);
		spin_unlock(sw_lock);
		rtk_cmd.ret = 0;
		break;
	case PHY_REG_WRITE:
		spin_lock(sw_lock);
		ret = ni_mdio_write(
			rtk_cmd.para.mdio.phy_addr,
			rtk_cmd.para.mdio.reg_addr,
			rtk_cmd.para.mdio.data
			);
		spin_unlock(sw_lock);
		rtk_cmd.ret = ret;
		break;
#if 1
	case SWITCH_REG_READ:
		ret = smi_read(
			rtk_cmd.para.mdio.reg_addr,
			&rtk_cmd.para.mdio.data
			);
		rtk_cmd.ret = ret;
		break;
	case SWITCH_REG_WRITE:
		ret = smi_write(
			rtk_cmd.para.mdio.reg_addr,
			rtk_cmd.para.mdio.data
			);
		rtk_cmd.ret = ret;
		break;
#endif
	default:
		rtk_cmd.ret = -EPERM;
	}

	if (copy_to_user(argp, (void *)&rtk_cmd, sizeof(rtk_cmd))) {
	    MESSAGE("Copy to user space fail\n");
	    return -EFAULT;
	}

	return 0;
} /* rtl83xx_ioctl */

static int rtl83xx_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int rtl83xx_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations rtl83xx_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = rtl83xx_ioctl,
	.open = rtl83xx_open,
	.release = rtl83xx_release,
};

static struct miscdevice rtl83xx_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SWITCH_DEVICE_NAME,
	.fops = &rtl83xx_fops,
};

static int __init rtl83xx_module_init(void)
{
	int ret = 0;

	printk(KERN_INFO DRV_NAME ": " DRV_DESCRIPTION ", " DRV_VERSION "\n");
	ret = rtl83xx_sw_init();
	if (ret) {
		MESSAGE("Switch software init error: %x\n", ret);
		goto out;
	}

	ret = rtl83xx_hw_init();
	if (ret) {
		MESSAGE("Switch hardware init error: %x\n", ret);
		goto out;
	}

	ret = misc_register(&rtl83xx_miscdev);
	if (ret) {
		MESSAGE("Switch misc device register error: %x\n", ret);
		goto out;
	}

	return 0;
out:
	return ret;
}

module_init(rtl83xx_module_init);

static void __exit rtl83xx_module_exit(void)
{
	misc_deregister(&rtl83xx_miscdev);
}

module_exit(rtl83xx_module_exit);

MODULE_AUTHOR("Eric Wang <eric.wang@cortina-systems.com>");
MODULE_LICENSE("GPL");
