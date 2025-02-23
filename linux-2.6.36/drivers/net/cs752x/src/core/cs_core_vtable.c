/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2010 by Cortina Systems Incorporated.                 */
/***********************************************************************/
/*
 * cs_core_vtable.c
 *
 * $Id$
 *
 * It contains the implementation of applicationed based classification
 * by utilizing vtable framework.
 */

#include <linux/spinlock.h>
#include <linux/if_ether.h>
#include <linux/in6.h>
#include "cs_core_vtable.h"
#include "cs_core_logic.h"
#include "cs_fe.h"
#include "cs_fe_mc.h"

#ifdef CONFIG_CS752X_PROC
#include "cs752x_proc.h"
extern u32 cs_ne_core_logic_debug;
#define DBG(x) {if (cs_ne_core_logic_debug & CS752X_CORE_LOGIC_CORE_VTABLE) x;}
#else
#define DBG(x) { }
#endif

#ifdef CONFIG_CS752X_ACCEL_KERNEL
#include "cs_hw_accel_manager.h"
#endif

u64 apptype_hashmask_tbl[CORE_APP_TYPE_MAX] = {
	0,	/* CORE_FWD_APP_TYPE_NONE */
	CS_HASHMASK_L2_MCAST,		/* CORE_FWD_APP_TYPE_L2_MCAST */
	CS_HASHMASK_L3_MCAST,		/* CORE_FWD_APP_TYPE_L3_MCAST */
	CS_HM_MAC_SA_MASK,		/* CORE_FWD_APP_TYPE_SA_CHECK */
	CS_HASHMASK_L2_DSCP,		/* CORE_FWD_APP_TYPE_L2_FLOW */  //Bug#40322
	CS_HASHMASK_L3_FLOW,		/* CORE_FWD_APP_TYPE_L3_GENERIC */
	CS_HASHMASK_L3_IPSEC_FLOW,	/* CORE_FWD_APP_TYPE_L3_IPSEC */
	CS_HASHMASK_RE,			/* CORE_FWD_APP_TYPE_IPSEC_FROM_RE */
	CS_HASHMASK_IPSEC_FROM_CPU,	/* CORE_FWD_APP_TYPE_IPSEC_FROM_CPU */
	CS_HASHMASK_LOGICAL_PORT,	/* CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT */
	CS_HASHMASK_L4_SPORT_BY_LOGICAL_PORT,		/* CORE_FWD_APP_TYPE_L4_SPORT */
	CS_HASHMASK_L4_DPORT_BY_LOGICAL_PORT,		/* CORE_FWD_APP_TYPE_L4_DPORT */

	/* BUG#39672: WFO NEC related features (Mutliple BSSID) */
	CS_HASHMASK_PE_RECIDX, 				/* CORE_FWD_APP_TYPE_PE_RECIDX */
	CS_HASHMASK_IP_PROT,		/* CORE_FWD_APP_TYPE_IP_PROT */
#ifdef CONFIG_CS75XX_MTU_CHECK	
	/* NEC MTU CHECK Requirement */	
	CS_HASHMASK_L3_MTU_IPOE,	/* CORE_FWD_APP_TYPE_L3_MTU_IPOE */
	CS_HASHMASK_L3_MTU_PPPOE,	/* CORE_FWD_APP_TYPE_L3_MTU_PPPOE */
	CS_HASHMASK_L3_MTU_IPLIP,	/* CORE_FWD_APP_TYPE_L3_MTU_IPLIP */
	CS_HASHMASK_L3_MTU_IPSEC,	/* CORE_FWD_APP_TYPE_L3_MTU_IPSEC */
#endif
	CS_HASHMASK_MCAST_TO_DEST,	/* CORE_FWD_APP_TYPE_MCAST_TO_DEST */
	CS_HASHMASK_MCAST_WITHOUT_SRC,	/* CORE_FWD_APP_TYPE_MCAST_WITHOUT_SRC */
	CS_HASHMASK_MCAST_WITH_SRC,	/* CORE_FWD_APP_TYPE_MCAST_WITH_SRC */
	CS_HASHMASK_IPLIP_WAN,		/* CORE_FWD_APP_TYPE_IPLIP_WAN */
	CS_HASHMASK_IPLIP_LAN,		/* CORE_FWD_APP_TYPE_IPLIP_LAN */
	0,				/* CORE_QOS_APP_TYPE_NONE = CORE_FWD_APP_TYPE_MAX */
	CS_HASHMASK_L2,			/* CORE_QOS_APP_TYPE_L2_QOS_1*/
	0,				/* CORE_QOS_APP_TYPE_L2_QOS_2*/
	CS_HASHMASK_L3_FLOW,		/* CORE_QOS_APP_TYPE_L3_QOS_GENERIC*/
	CS_HASHMASK_L3_MCAST,		/* CORE_QOS_APP_TYPE_L3_QOS_MULTICAST*/
	0,				/* CORE_QOS_APP_TYPE_L4_QOS_NAT*/
};

cs_core_vtable_def_hashmask_info_t vtable_def_hm_info[CORE_VTABLE_TYPE_MAX] =
{
	/* CORE_VTABLE_TYPE_NONE */
	{0, 0,
		{CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
		        CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
	/* CORE_VTABLE_TYPE_BCAST */
	{1, 0,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
	/* CORE_VTABLE_TYPE_L2_MCAST */
#ifdef SA_CHECK_ENABLE
	{2, 0,
		{CORE_FWD_APP_TYPE_SA_CHECK, CORE_FWD_APP_TYPE_L2_MCAST,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#else
	{1, 0,
		{CORE_FWD_APP_TYPE_L2_MCAST,
			CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#endif
	/* CORE_VTABLE_TYPE_L3_MCAST_V4 */
#ifdef SA_CHECK_ENABLE
	{6, 0,
		{CORE_FWD_APP_TYPE_SA_CHECK, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_MCAST_WITHOUT_SRC, CORE_FWD_APP_TYPE_MCAST_WITH_SRC,
			CORE_FWD_APP_TYPE_MCAST_TO_DEST, CORE_FWD_APP_TYPE_IP_PROT,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#else
	{5, 1,
		{CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_MCAST_WITHOUT_SRC, CORE_FWD_APP_TYPE_MCAST_WITH_SRC,
			CORE_FWD_APP_TYPE_MCAST_TO_DEST, CORE_FWD_APP_TYPE_IP_PROT,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_NONE}},
#endif
	/* CORE_VTABLE_TYPE_L3_MCAST_V6 */
#ifdef SA_CHECK_ENABLE
	{5, 0,
		{CORE_FWD_APP_TYPE_SA_CHECK, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_MCAST_WITHOUT_SRC, CORE_FWD_APP_TYPE_MCAST_WITH_SRC,
			CORE_FWD_APP_TYPE_MCAST_TO_DEST,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#else
	{4, 1,
		{CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_MCAST_WITHOUT_SRC, CORE_FWD_APP_TYPE_MCAST_WITH_SRC,
			CORE_FWD_APP_TYPE_MCAST_TO_DEST,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_NONE}},
#endif

	/* CORE_VTABLE_TYPE_L2_FLOW */
#ifdef SA_CHECK_ENABLE
	{3, 1,
		{CORE_FWD_APP_TYPE_SA_CHECK,
			CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_L2_FLOW,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L2_QOS_1, CORE_QOS_APP_TYPE_NONE}},
#else
	{2, 1,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_L2_FLOW, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L2_QOS_1, CORE_QOS_APP_TYPE_NONE}},
#endif

	/* CORE_VTABLE_TYPE_L3_IPLIP */
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
#ifdef CONFIG_CS75XX_MTU_CHECK
	{6, 1,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_IPLIP_WAN,
			CORE_FWD_APP_TYPE_L3_GENERIC, 
			CORE_FWD_APP_TYPE_L3_MTU_IPLIP,
			CORE_FWD_APP_TYPE_L3_MTU_PPPOE,
			CORE_FWD_APP_TYPE_L3_MTU_IPOE,
			CORE_QOS_APP_TYPE_L3_QOS_GENERIC,
			CORE_QOS_APP_TYPE_NONE}},
#else
	{4, 1,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_IPLIP_WAN,
			CORE_FWD_APP_TYPE_IPLIP_LAN,
			CORE_FWD_APP_TYPE_L3_GENERIC,
			CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L3_QOS_GENERIC,
			CORE_QOS_APP_TYPE_NONE}},
#endif
#endif

	/* CORE_VTABLE_TYPE_L3_FLOW */
#ifdef SA_CHECK_ENABLE
	{6, 1,
		{CORE_FWD_APP_TYPE_SA_CHECK,
			CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_L4_SPORT, CORE_FWD_APP_TYPE_L4_DPORT,
			CORE_FWD_APP_TYPE_L3_GENERIC,
			CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_QOS_APP_TYPE_L3_QOS_GENERIC, CORE_QOS_APP_TYPE_NONE}},
#elif defined(CONFIG_CS75XX_MTU_CHECK)
	{6, 1,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_L3_GENERIC,
			CORE_FWD_APP_TYPE_L3_MTU_PPPOE,
			CORE_FWD_APP_TYPE_L3_MTU_IPOE,
			CORE_FWD_APP_TYPE_L3_MTU_IPSEC,
			CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_QOS_APP_TYPE_L3_QOS_GENERIC,
			CORE_QOS_APP_TYPE_NONE}},
#else
	{5, 1,
		{CORE_FWD_APP_TYPE_SEPARATE_LOGICAL_PORT,
			CORE_FWD_APP_TYPE_L4_SPORT, CORE_FWD_APP_TYPE_L4_DPORT,
			CORE_FWD_APP_TYPE_L3_GENERIC,
			CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_L3_QOS_GENERIC, CORE_QOS_APP_TYPE_NONE}},
#endif
	/* CORE_VTABLE_TYPE_RE0 */
#ifdef CONFIG_CS75XX_WFO
	{5, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L2_FLOW,
			CORE_FWD_APP_TYPE_L2_MCAST, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, 
			CORE_QOS_APP_TYPE_L2_QOS_1, CORE_QOS_APP_TYPE_L3_QOS_MULTICAST}},
#else
	{1, 0,
		{CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#endif
	/* CORE_VTABLE_TYPE_RE1_IPLIP */
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
#ifdef CONFIG_CS75XX_WFO
	{5, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPLIP_WAN, CORE_FWD_APP_TYPE_L2_FLOW,
			CORE_FWD_APP_TYPE_L2_MCAST, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, 
			CORE_QOS_APP_TYPE_L2_QOS_1, CORE_QOS_APP_TYPE_L3_QOS_MULTICAST}},
#else
	{1, 0,
		{CORE_FWD_APP_TYPE_IPLIP_WAN, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#endif
#endif
	/* CORE_VTABLE_TYPE_RE1 */
#ifdef CONFIG_CS75XX_WFO
	{5, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L2_FLOW,
			CORE_FWD_APP_TYPE_L2_MCAST, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, 
			CORE_QOS_APP_TYPE_L2_QOS_1, CORE_QOS_APP_TYPE_L3_QOS_MULTICAST}},
#else
	{1, 0,
		{CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#endif
	/* CORE_VTABLE_TYPE_CPU */
	{1, 0,
		{CORE_FWD_APP_TYPE_IPSEC_FROM_CPU, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
	/* CORE_VTABLE_TYPE_ARP */
	{0, 0,
		{ CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
	/* CORE_VTABLE_TYPE_ICMPV6 */
	{1, 0,
		{ CORE_FWD_APP_TYPE_IP_PROT, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_FWD_APP_TYPE_NONE, CORE_FWD_APP_TYPE_NONE,
			CORE_QOS_APP_TYPE_NONE, CORE_QOS_APP_TYPE_NONE}},
#ifdef CONFIG_CS75XX_WFO
#ifdef CONFIG_CS75XX_MTU_CHECK
	/* CORE_VTABLE_TYPE_RE0 WFO_L3*/
	{6, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */	
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_FWD_APP_TYPE_L3_MTU_IPSEC, 
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_L3_QOS_GENERIC}},
	/* CORE_VTABLE_TYPE_RE1 WFO_L3*/
	{6, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */	
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_FWD_APP_TYPE_L3_MTU_IPSEC, 
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_L3_QOS_GENERIC}},
#else
	/* CORE_VTABLE_TYPE_RE0 WFO_L3*/
	{5, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */	
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_FWD_APP_TYPE_NONE, 
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_L3_QOS_GENERIC}},
	/* CORE_VTABLE_TYPE_RE1 WFO_L3*/
	{5, 2,
		/* BUG#39672: WFO NEC related features (Mutliple BSSID) */	
		{CORE_FWD_APP_TYPE_PE_RECIDX, CORE_FWD_APP_TYPE_IPSEC_FROM_RE, CORE_FWD_APP_TYPE_L3_MCAST,
			CORE_FWD_APP_TYPE_L3_GENERIC, CORE_FWD_APP_TYPE_L3_IPSEC,
			CORE_FWD_APP_TYPE_NONE, 
			CORE_QOS_APP_TYPE_L3_QOS_MULTICAST, CORE_QOS_APP_TYPE_L3_QOS_GENERIC}},
#endif
#endif
};

cs_vtable_t *vtable_list[CORE_VTABLE_TYPE_MAX];
spinlock_t vtable_lock[CORE_VTABLE_TYPE_MAX];
/* Hash mask table, one application type per hm entry, fixed. */
static u8 app_hm_table[CORE_APP_TYPE_MAX][2];
#define APP_HM_TABLE_INVALID	0xff

#define CORE_VTABLE_VIRTURAL_MAC_MAX 10

typedef struct cs_core_vtable_virtual_mac_info_s {
	char dev_name[256];
	char addr[ETH_ALEN];
	u32 an_bng_mac_idx;
} cs_core_vtable_virtual_mac_info_t;

cs_core_vtable_virtual_mac_info_t
	vtable_virtual_mac_list[CORE_VTABLE_VIRTURAL_MAC_MAX];

static void set_mac_swap_order(unsigned char *dest,
		unsigned char *src, int len)
{
	int i;
	for (i=0; i<len; i++)
		*(dest+len-1-i) = *(src+i);
}

int cs_core_vtable_virtual_mac_add(char * dev_name, const unsigned char *addr) {
	int i;
	int free_idx = -1;
	int ret;
	fe_an_bng_mac_entry_t abm_entry;
	set_mac_swap_order(abm_entry.mac, (unsigned char *) addr, ETH_ALEN);
	abm_entry.sa_da = 0; /* 0: DA, 1: SA */
	abm_entry.pspid = 0;
	abm_entry.pspid_mask = 1; /*don't check pspid*/
	abm_entry.valid = 1;

	DBG(printk("%s %s %pM\n", __func__, dev_name, addr));

#ifdef CONFIG_CS752X_ACCEL_KERNEL
	cs_hw_accel_mgr_delete_flow_based_hash_entry();
#endif

	for (i = 0; i < CORE_VTABLE_VIRTURAL_MAC_MAX; i++) {
		if (memcmp(dev_name, vtable_virtual_mac_list[i].dev_name,
			strlen(dev_name)) == 0) {

			ret = cs_fe_table_set_entry(FE_TABLE_AN_BNG_MAC,
				vtable_virtual_mac_list[i].an_bng_mac_idx, &abm_entry);
			if (ret == 0)
				memcpy(vtable_virtual_mac_list[i].addr, addr, ETH_ALEN);
			DBG(printk("%s set i=%d idx=%d ret=%d \n", __func__, i,
				vtable_virtual_mac_list[i].an_bng_mac_idx, ret));
			return ret;
		} else if (free_idx == -1) {
			if (vtable_virtual_mac_list[i].an_bng_mac_idx == 0xffff)
				free_idx = i;
		}
	}

	/*cannot find the entry, then insert one*/
	if (free_idx == -1)
		return -1;

	ret = cs_fe_table_add_entry(FE_TABLE_AN_BNG_MAC, &abm_entry,
					&vtable_virtual_mac_list[free_idx].an_bng_mac_idx);
	DBG(printk("%s add i=%d idx=%d ret=%d \n", __func__, free_idx,
				vtable_virtual_mac_list[free_idx].an_bng_mac_idx, ret));
	if (ret != 0) {
		printk("%s err in add FE_TABLE_AN_BNG_MAC entry ret=%d\n",
			__func__, ret);
		vtable_virtual_mac_list[free_idx].an_bng_mac_idx = 0xffff;
	} else {
		memcpy(vtable_virtual_mac_list[free_idx].dev_name, dev_name,
			strlen(dev_name));
		memcpy(vtable_virtual_mac_list[free_idx].addr, addr, ETH_ALEN);
	}

	return ret;
}

int cs_core_vtable_virtual_mac_del_by_dev(char * dev_name) {
	int i;
	int ret;

	DBG(printk("%s %s \n", __func__, dev_name));

	for (i = 0; i < CORE_VTABLE_VIRTURAL_MAC_MAX; i++) {
		if (memcmp(dev_name, vtable_virtual_mac_list[i].dev_name,
			strlen(dev_name)) == 0) {
			ret = cs_fe_table_del_entry_by_idx(FE_TABLE_AN_BNG_MAC,
				vtable_virtual_mac_list[i].an_bng_mac_idx, false);

			DBG(printk("%s i=%d idx=%d\n", __func__, i,
				vtable_virtual_mac_list[i].an_bng_mac_idx));

			vtable_virtual_mac_list[i].an_bng_mac_idx = 0xffff;
			vtable_virtual_mac_list[i].dev_name[0] = '\0';
			return 0;
		}
	}

#ifdef CONFIG_CS752X_ACCEL_KERNEL
	cs_hw_accel_mgr_delete_flow_based_hash_entry();
#endif

	return -1;
}

EXPORT_SYMBOL(cs_core_vtable_virtual_mac_add);
EXPORT_SYMBOL(cs_core_vtable_virtual_mac_del_by_dev);

void cs_core_vtable_virtual_mac_init(void) {
	int i;
	for (i = 0; i < CORE_VTABLE_VIRTURAL_MAC_MAX; i++) {
		vtable_virtual_mac_list[i].an_bng_mac_idx = 0xffff;
		vtable_virtual_mac_list[i].dev_name[0] = '\0';
	}
}

/* initialize the default vtables.
 * Seven default Vtables: broadcast, multicast, L2, L3, RE x2, and ARP. */
int cs_core_vtable_init(void)
{
	cs_vtable_t *new_table;
	int i;

	for (i = 0; i < CORE_VTABLE_TYPE_MAX; i++) {
		vtable_list[i] = NULL;
		spin_lock_init(&vtable_lock[i]);
	}

	for (i = 0; i < CORE_APP_TYPE_MAX; i++) {
		app_hm_table[i][0] = APP_HM_TABLE_INVALID;
		app_hm_table[i][1] = 0;
	}

	for (i = CORE_VTABLE_TYPE_BCAST; i < CORE_VTABLE_TYPE_MAX; i++) {
		new_table = cs_core_vtable_alloc(i);
		if (new_table == NULL)
			printk("%s:%d:failed to create default vtable for"
					"table type %d\n", __func__,
					__LINE__, i);
	}

	cs_core_vtable_virtual_mac_init();
	return 0;
} /* cs_core_vtable_init */

/* exit the core vtable */
void cs_core_vtable_exit(void)
{
	// FIXME!! implement later
	return;
} /* cs_core_vtable_exit */

static int get_flow_from_rule_vtbl_type(unsigned int rule_vtbl_type)
{
	switch (rule_vtbl_type) {
		case CORE_VTABLE_TYPE_L2_RULE:
			return CORE_VTABLE_TYPE_L2_FLOW;
		case CORE_VTABLE_TYPE_L3_RULE_PREROUTING:
		case CORE_VTABLE_TYPE_L3_RULE_FORWARD:
		case CORE_VTABLE_TYPE_L3_RULE_POSTROUTING:
			return CORE_VTABLE_TYPE_L3_FLOW;
		default:
			return CORE_VTABLE_TYPE_NONE;
	};
} /* get_flow_from_rule_vtbl_type */

/* hash mask related APIs (all internal APIs)*/
int cs_core_vtable_set_hashmask_index_to_apptype(unsigned int app_type,
		u8 hash_mask_idx)
{
	if (app_hm_table[app_type][0] == APP_HM_TABLE_INVALID) {
//		printk("%s:%d:add hash mask type %d, index %d\n", __func__,
//				__LINE__, app_type, hash_mask_idx);
		app_hm_table[app_type][0] = hash_mask_idx;
		app_hm_table[app_type][1] = 1;
	} else {
		if (app_hm_table[app_type][0] != hash_mask_idx) {
//			printk("%s:%d:a previous hash mask %d has been set to "
//					"this app_type %d\n", __func__,
//					__LINE__, app_hm_table[app_type][0],
//					app_type);
			return -1;
		} else {
			DBG(printk("%s:%d:hash mask type %d, index %d has been "
					"added before!\n", __func__, __LINE__,
					app_type, hash_mask_idx));
			app_hm_table[app_type][1]++;
		}
	}
	return 0;
} /* cs_core_vtable_set_hashmask_index_to_apptype */

int cs_core_vtable_unset_hashmask_index_from_apptype(
		unsigned int app_type)
{
	app_hm_table[app_type][0] = APP_HM_TABLE_INVALID;
	app_hm_table[app_type][1] = 0;
	return 0;
} /* cs_core_vtable_unset_hashmask_index_from_apptype */

/* get the hashmask index with given fwd application type flow */
int cs_core_vtable_get_hashmask_index_from_apptype(
		unsigned int app_type, u8 *hm_idx)
{
	if (hm_idx == NULL)
		return -1;
	if (app_hm_table[app_type][0] == APP_HM_TABLE_INVALID) {
		printk("%s:%d:no hash mask has been set to this "
				"app_type %d\n", __func__, __LINE__,
				app_type);
		return -1;
	}
	if (app_hm_table[app_type][1] == 0) {
		printk("%s:%d:the ref count to this app_type %d is 0\n",
				__func__, __LINE__, app_type);
		return -1;
	}
	*hm_idx = app_hm_table[app_type][0];

	return 0;
} /* cs_core_vtable_get_hashmask_index_from_apptype */

inline void init_hashmask_entry(fe_hash_mask_entry_t *hm_entry)
{
	memset(hm_entry, 0xff, sizeof(fe_hash_mask_entry_t));
#define DEFAULT_HASH_KEYGEN_POLY		0
	hm_entry->keygen_poly_sel = DEFAULT_HASH_KEYGEN_POLY;
	hm_entry->ip_sa_mask = 0;
	hm_entry->ip_da_mask = 0;
}

void convert_hashmask_flag_to_data(u64 flag,
		fe_hash_mask_entry_t *hm_entry)
{
	init_hashmask_entry(hm_entry);

	/* L2 */
	if (flag & CS_HM_MAC_SA_MASK)
		hm_entry->mac_sa_mask = 0;
	if (flag & CS_HM_MAC_DA_MASK)
		hm_entry->mac_da_mask = 0;
	if (flag & CS_HM_ETHERTYPE_MASK)
		hm_entry->ethertype_mask = 0;
	if (flag & CS_HM_TPID_ENC_1_LSB_MASK)
		hm_entry->tpid_enc_1_lsb_mask = 0;
	if (flag & CS_HM_TPID_ENC_1_MSB_MASK)
		hm_entry->tpid_enc_1_msb_mask = 0;
	if (flag & CS_HM_VID_1_MASK)
		hm_entry->vid_1_mask = 0;
	if (flag & CS_HM_8021P_1_MASK)
		hm_entry->_8021p_1_mask = 0;
	if (flag & CS_HM_DEI_1_MASK)
		hm_entry->dei_1_mask = 0;
	if (flag & CS_HM_TPID_ENC_2_LSB_MASK)
		hm_entry->tpid_enc_2_lsb_mask = 0;
	if (flag & CS_HM_TPID_ENC_2_MSB_MASK)
		hm_entry->tpid_enc_2_msb_mask = 0;
	if (flag & CS_HM_VID_2_MASK)
		hm_entry->vid_2_mask = 0;
	if (flag & CS_HM_8021P_2_MASK)
		hm_entry->_8021p_2_mask = 0;
	if (flag & CS_HM_DEI_2_MASK)
		hm_entry->dei_2_mask = 0;
	if (flag & CS_HM_PPPOE_SESSION_ID_VLD_MASK)
		hm_entry->pppoe_session_id_vld_mask = 0;
	if (flag & CS_HM_PPPOE_SESSION_ID_MASK)
		hm_entry->pppoe_session_id_mask = 0;

	/* L3 */
	if (flag & CS_HM_IP_DA_MASK)
		hm_entry->ip_da_mask = 0x080;
	/* 0x080 = 128 bit mask.. works for both IPv4 and IPv6 */
	if (flag & CS_HM_IP_SA_MASK)
		hm_entry->ip_sa_mask = 0x080;
	/* 0x080 = 128 bit mask.. works for both IPv4 and IPv6 */
	if (flag & CS_HM_IP_PROT_MASK)
		hm_entry->ip_prot_mask = 0;
	if (flag & CS_HM_IP_FRAGMENT_MASK)
		hm_entry->ip_fragment_mask = 0;
	if (flag & CS_HM_IP_VER_MASK)
		hm_entry->ip_ver_mask = 0;
	if (flag & CS_HM_IP_VLD_MASK)
		hm_entry->ip_vld_mask = 0;
	if (flag & CS_HM_L3_CHKSUM_ERR_MASK)
		hm_entry->l3_chksum_err_mask = 0;
	if (flag & CS_HM_DSCP_MASK)
		hm_entry->dscp_mask = 0;
	if (flag & CS_HM_ECN_MASK)
		hm_entry->ecn_mask = 0;

	/* L4 */
	if (flag & CS_HM_L4_VLD_MASK)
		hm_entry->l4_vld_mask = 0;
	/* how we control whether src port and dst port are ranged is that:
	 * by default if only CS_HM_L4_DP_MASK or/and CS_HM_L4_SP_MASK is on,
	 * then port ranged is not used.  However, if CS_HM_L4_PORTS_RNGD is
	 * on, the both port will be using port range table */
	/* port range definition:
	 * 00 = Not ranged (exact matched).
	 * 01 = Destination port is ranged but source port is exact matched.
	 * 10 = Destination port is exact matched but source port is ranged.
	 * 11 = Both destination and source are ranged. */
	hm_entry->l4_ports_rngd = 0;
	if (flag & CS_HM_L4_DP_MASK) {
		hm_entry->l4_dp_mask = 0;
		if (flag & CS_HM_L4_PORTS_RNGD)
			hm_entry->l4_ports_rngd |= 0x1;
	}

	if (flag & CS_HM_L4_SP_MASK) {
		hm_entry->l4_sp_mask = 0;
		if (flag & CS_HM_L4_PORTS_RNGD)
			hm_entry->l4_ports_rngd |= 0x2;
	}

	if (flag & CS_HM_TCP_CTRL_MASK)
		hm_entry->tcp_ctrl_mask = TCP_CTRL_MASK;

	/* IPsec */
	if (flag & CS_HM_SPI_VLD_MASK)
		hm_entry->spi_vld_mask = 0;
	if (flag & CS_HM_SPI_MASK)
		hm_entry->spi_mask = 0;

	/* IPv6 */
	if (flag & CS_HM_IPV6_NDP_MASK)
		hm_entry->ipv6_ndp_mask = 0;
	if (flag & CS_HM_IPV6_HBH_MASK)
		hm_entry->ipv6_hbh_mask = 0;
	if (flag & CS_HM_IPV6_RH_MASK)
		hm_entry->ipv6_rh_mask = 0;
	if (flag & CS_HM_IPV6_DOH_MASK)
		hm_entry->ipv6_doh_mask = 0;
	if (flag & CS_HM_IPV6_FLOW_LBL_MASK)
		hm_entry->ipv6_flow_lbl_mask = 0;

	/* Miscellaneous */
	if (flag & CS_HM_ORIG_LSPID_MASK)
		hm_entry->orig_lspid_mask = 0;
	if (flag & CS_HM_LSPID_MASK)
		hm_entry->lspid_mask = 0;
#ifdef CONFIG_CS75XX_MTU_CHECK
	if (flag & CS_HM_PKTLEN_RNG_MATCH_VECTOR_B0_MASK)
		hm_entry->pktlen_rng_match_vector_mask &= ~0x1;
	if (flag & CS_HM_PKTLEN_RNG_MATCH_VECTOR_B1_MASK)
		hm_entry->pktlen_rng_match_vector_mask &= ~(0x1 << 1);
	if (flag & CS_HM_PKTLEN_RNG_MATCH_VECTOR_B2_MASK)
		hm_entry->pktlen_rng_match_vector_mask &= ~(0x1 << 2);
	if (flag & CS_HM_PKTLEN_RNG_MATCH_VECTOR_B3_MASK)
		hm_entry->pktlen_rng_match_vector_mask &= ~(0x1 << 3);
#endif
	if (flag & CS_HM_RECIRC_IDX_MASK)
		hm_entry->recirc_idx_mask = 0;
	if (flag & CS_HM_MCIDX_MASK)
		hm_entry->mcidx_mask = 0;
	if (flag & CS_HM_MCGID_MASK)
		hm_entry->mcgid_mask = 0;

	return;
} /* convert_hashmask_flag_to_data */

static int  vtable_setup_def_class(unsigned int vtbl_type,
		fe_class_entry_t *p_class)
{
	fe_eth_type_entry_t etype_entry;
	unsigned int etype_idx;
	int ret;

	memset((void *)p_class, 0xff, sizeof(fe_class_entry_t));
	p_class->entry_valid = 1;
	p_class->l3.ip_sa_mask = 0x000;
	p_class->l3.ip_da_mask = 0x000;
	p_class->port.mcgid = 0;
	p_class->port.mcgid_mask = 0;
	p_class->parity = 0;
	p_class->sdb_idx = 0;
	p_class->rule_priority = 0;
	switch (vtbl_type) {
	case CORE_VTABLE_TYPE_BCAST:
		p_class->rule_priority = BCAST_DEF_VTABLE_PRIORITY;
		p_class->l2.bcast_da = 1;
		p_class->l2.bcast_da_mask = 0;
		break;
	case CORE_VTABLE_TYPE_L2_MCAST:
		p_class->rule_priority = L2_MCAST_DEF_VTABLE_PRIORITY;
		p_class->l2.mcast_da = 1;
		p_class->l2.mcast_da_mask = 0;
		p_class->port.mcgid_mask = 0x1ff;
		break;
	case CORE_VTABLE_TYPE_L3_MCAST_V4:
		p_class->rule_priority = L3_MCAST_DEF_VTABLE_PRIORITY;
		p_class->l3.da[0] = 0xe0000000;
		p_class->l3.da[1] = 0;
		p_class->l3.da[2] = 0;
		p_class->l3.da[3] = 0;
		p_class->l3.ip_da_mask = 0x04;
		p_class->l3.ip_valid = 1;
		p_class->l3.ip_valid_mask = 0;
		p_class->l3.ip_ver = 0;
		p_class->l3.ip_ver_mask = 0;
		p_class->port.mcgid_mask = 0x1ff;
		break;
	case CORE_VTABLE_TYPE_L3_MCAST_V6:
		p_class->rule_priority = L3_MCAST_DEF_VTABLE_PRIORITY;
		p_class->l3.da[0] = 0;
		p_class->l3.da[1] = 0;
		p_class->l3.da[2] = 0;
		p_class->l3.da[3] = 0xff000000;
		p_class->l3.ip_da_mask = 0x08;
		p_class->l3.ip_valid = 1;
		p_class->l3.ip_valid_mask = 0;
		p_class->l3.ip_ver = 1;
		p_class->l3.ip_ver_mask = 0;
		p_class->port.mcgid_mask = 0x1ff;
		break;
	case CORE_VTABLE_TYPE_L2_FLOW:
		p_class->rule_priority = L2_FLOW_DEF_VTABLE_PRIORITY;
		p_class->l2.da_an_mac_hit = 0;
		p_class->l2.da_an_mac_hit_mask = 0;
		p_class->l2.da_an_mac_sel = 0xf;
		p_class->l2.da_an_mac_sel_mask = 1;
		break;
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
	case CORE_VTABLE_TYPE_L3_IPLIP:
		p_class->rule_priority = L3_FLOW_DEF_VTABLE_PRIORITY+1;
		p_class->l2.da_an_mac_hit = 1;
		p_class->l2.da_an_mac_hit_mask = 0;
		p_class->l2.da_an_mac_sel = 0xf;
		p_class->l2.da_an_mac_sel_mask = 1;
		break;
#endif
	case CORE_VTABLE_TYPE_L3_FLOW:
		p_class->rule_priority = L3_FLOW_DEF_VTABLE_PRIORITY;
		p_class->l2.da_an_mac_hit = 1;
		p_class->l2.da_an_mac_hit_mask = 0;
		p_class->l2.da_an_mac_sel = 0xf;
		p_class->l2.da_an_mac_sel_mask = 1;
		break;
	case CORE_VTABLE_TYPE_RE0:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY;
		p_class->port.lspid = ENCRYPTION_PORT;
		p_class->port.lspid_mask = 0;
		break;
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
	case CORE_VTABLE_TYPE_RE1_IPLIP:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY + 1;
		p_class->port.lspid = ENCAPSULATION_PORT;
		p_class->port.lspid_mask = 0;
		break;
#endif
	case CORE_VTABLE_TYPE_RE1:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY;
		p_class->port.lspid = ENCAPSULATION_PORT;
		p_class->port.lspid_mask = 0;
		break;
#ifdef CONFIG_CS75XX_WFO
	case CORE_VTABLE_TYPE_RE0_WFO_L3:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY + 1;
		p_class->port.lspid = ENCRYPTION_PORT;
		p_class->port.lspid_mask = 0;
		p_class->l2.da_an_mac_hit = 1;
		p_class->l2.da_an_mac_hit_mask = 0;
		p_class->l2.da_an_mac_sel = 0xf;
		p_class->l2.da_an_mac_sel_mask = 1;
		break;
	case CORE_VTABLE_TYPE_RE1_WFO_L3:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY + 1;
		p_class->port.lspid = ENCAPSULATION_PORT;
		p_class->port.lspid_mask = 0;
		p_class->l2.da_an_mac_hit = 1;
		p_class->l2.da_an_mac_hit_mask = 0;
		p_class->l2.da_an_mac_sel = 0xf;
		p_class->l2.da_an_mac_sel_mask = 1;
		break;
#endif
	case CORE_VTABLE_TYPE_CPU:
		p_class->rule_priority = RE_SPECIFIC_DEF_VTABLE_PRIORITY;
		p_class->port.lspid = CPU_PORT;
		p_class->port.lspid_mask = 0;
		break;
	case CORE_VTABLE_TYPE_ARP:
		p_class->rule_priority = ARP_DEF_VTABLE_PRIORITY;
		memset(&etype_entry, 0, sizeof(etype_entry));
		etype_entry.ether_type = ETH_P_ARP;
		etype_entry.valid = 1;
		ret = cs_fe_table_add_entry(FE_TABLE_ETYPE, &etype_entry,
				&etype_idx);
		if (ret != 0) {
			cs_fe_table_del_entry_by_idx(FE_TABLE_ETYPE, etype_idx,
					false);
			return ret;
		}
		p_class->l2.ethertype_enc = etype_idx + 1;
		p_class->l2.ethertype_enc_mask = 0;
		break;
	case CORE_VTABLE_TYPE_ICMPV6:
		p_class->rule_priority = ICMPV6_DEF_VTABLE_PRIORITY;
		p_class->l3.ip_prot = IPPROTO_ICMPV6;
		p_class->l3.ip_prot_mask = 0;
		p_class->l3.ip_valid = 1;
		p_class->l3.ip_valid_mask = 0;
		p_class->l3.ip_ver = 1;
		p_class->l3.ip_ver_mask = 0;
		break;
	default:
		break;
	}
	return 0;
} /* vtable_setup_def_class */

static int vtable_get_def_act(unsigned int vtbl_type)
{
	switch (vtbl_type) {
	case CORE_VTABLE_TYPE_BCAST:
	case CORE_VTABLE_TYPE_L2_MCAST:
	case CORE_VTABLE_TYPE_L3_MCAST_V4:
	case CORE_VTABLE_TYPE_L3_MCAST_V6:
	case CORE_VTABLE_TYPE_L2_FLOW:
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
	case CORE_VTABLE_TYPE_L3_IPLIP:
#endif
	case CORE_VTABLE_TYPE_L3_FLOW:		
		return CPU_PORT0_VOQ_BASE;
	case CORE_VTABLE_TYPE_CPU:
	case CORE_VTABLE_TYPE_RE0:
#ifdef CONFIG_CS75XX_WFO
	case CORE_VTABLE_TYPE_RE0_WFO_L3:
#endif
		return CPU_PORT6_VOQ_BASE;
#ifdef CONFIG_CS75XX_HW_ACCEL_IPLIP
	case CORE_VTABLE_TYPE_RE1_IPLIP:
#endif
	case CORE_VTABLE_TYPE_RE1:
#ifdef CONFIG_CS75XX_WFO
	case CORE_VTABLE_TYPE_RE1_WFO_L3:
#endif
		return CPU_PORT6_VOQ_BASE + 1;
	case CORE_VTABLE_TYPE_ARP:
	case CORE_VTABLE_TYPE_ICMPV6:
		return CPU_PORT7_VOQ_BASE;
	default:
		return -1;
	}
} /* vtable_get_def_act */

static int vtable_setup_def_hashmask(cs_vtable_t *p_vtbl)
{
	int ret, i;
	fe_hash_mask_entry_t hm_entry;
	cs_core_vtable_def_hashmask_info_t *vtable_info;
	u8 priority;
	int count = 0;

	vtable_info = &vtable_def_hm_info[p_vtbl->vtable_type];
	/*need to go through 6 fwd tuples*/
	for (i = 0; i < MAX_FWD_HASH_TUPLE ; i++) {
		if (apptype_hashmask_tbl[vtable_info->mask_apptype[i]] == 0)
			continue;

		convert_hashmask_flag_to_data(apptype_hashmask_tbl[
				vtable_info->mask_apptype[i]],
				&hm_entry);

		priority = i;	/* for future hash mask tuple addition
					   with higher priority */

		if ((i == 0) &&
				(vtable_info->mask_apptype[i] == CORE_FWD_APP_TYPE_SA_CHECK)
				)
			priority = 0x8;

		ret = cs_vtable_add_hashmask(p_vtbl, &hm_entry, priority,
				false);
		/* return value here is hashmask index.  if it's negative, then
		 * something is wrong. */
		if (ret < 0) {
			cs_vtable_del_hashmask_all(p_vtbl);
			return ret;
		}

		cs_core_vtable_set_hashmask_index_to_apptype(
				vtable_info->mask_apptype[i], ret);
		count++;
	}

	if (vtable_info->fwdtuple_count != count)
		printk("%s Err in create %d hash tuples at vtable_type=%d (need %d) \n",
			__func__, count, p_vtbl->vtable_type, vtable_info->fwdtuple_count);

	/*need to go through 2 QoS tuples*/
	count = 0;
	for (i = MAX_FWD_HASH_TUPLE; i < MAX_FWD_HASH_TUPLE + MAX_QOS_HASH_TUPLE;
		i++) {
		if (apptype_hashmask_tbl[vtable_info->mask_apptype[i]] == 0)
			continue;

		convert_hashmask_flag_to_data(apptype_hashmask_tbl[
				vtable_info->mask_apptype[i]],
				&hm_entry);

		priority = i - 6;	/* for future hash mask tuple addition
						   with lower priority */

		ret = cs_vtable_add_hashmask(p_vtbl, &hm_entry, priority,
				true);
		/* return value here is hashmask index.  if it's negative, then
		 * something is wrong. */
		if (ret < 0) {
			cs_vtable_del_hashmask_all(p_vtbl);
			return ret;
		}

		cs_core_vtable_set_hashmask_index_to_apptype(
				vtable_info->mask_apptype[i], ret);
		count++;
	}

	if (vtable_info->qostuple_count != count)
		printk("%s Err in create %d QoS hash tuples at vtable_type=%d (need %d) \n",
			__func__, count, p_vtbl->vtable_type, vtable_info->qostuple_count);

	return 0;
} /* vtable_setup_def_hashmask */

/* allocate a vtable with vtbl_type. Pointer to the table is returned when it
 * succeeds, or it will return NULL.  It performs allocation based on the
 * following list of case:
 * 	1) Vtbl_type is a flow vtable
 * 		a) No existing vtable in the chain => new vtable is the head
 * 		of chain.
 * 		b) Chain is not empty => new vtable will be inserted between
 * 		the last existed flow vtable and the first existed rule vtable.
 * 	2) Vtbl_type is rule vtable
 * 		a) Chain of its respective flow vtable is empty => error!!
 * 		b) Else => the logic will locate where to insert the newly
 * 		allocated vtable in its respective chain. */
cs_vtable_t *cs_core_vtable_alloc(unsigned int vtbl_type)
{
	cs_vtable_t *new_table, *prev_vtbl = NULL;
	fe_class_entry_t class_entry;
	unsigned int flow_vtbl_type = vtbl_type;
	int ret, def_act;

	if (vtbl_type >= CORE_VTABLE_RULE_TYPE_MAX)
		return NULL;

	/* locate where to insert the vtable if the allocation succeeds */
	if (vtbl_type < CORE_VTABLE_TYPE_MAX) {
		spin_lock(&vtable_lock[vtbl_type]);
		/* flow vtable in a non-empty chain, we need to locate
		 * the place to insert the vtable into the chain */
		if (vtable_list[vtbl_type] != NULL) {
			/* CORE_VTABLE_TYPE_NONE does not support linking? */
			if (vtbl_type == CORE_VTABLE_TYPE_NONE) {
				spin_unlock(&vtable_lock[vtbl_type]);
				return NULL;
			}
			prev_vtbl = vtable_list[vtbl_type];
			while ((prev_vtbl->next != NULL) &&
					(prev_vtbl->next->vtable_type ==
					 vtbl_type)) {
				prev_vtbl = prev_vtbl->next;
			};
		}
	} else if (vtbl_type < CORE_VTABLE_RULE_TYPE_MAX) {
		/* for rule vtable allocation, we need to find the previous and
		 * next vtable for this new vtable */
		flow_vtbl_type = get_flow_from_rule_vtbl_type(vtbl_type);
		/* it should have valid flow vtable to link with */
		if (flow_vtbl_type == CORE_VTABLE_TYPE_NONE)
			return NULL;

		prev_vtbl = vtable_list[flow_vtbl_type];
		/* flow vtable should've been created before creating
		 * its respective rule vtable */
		if (prev_vtbl == NULL)
			return NULL;

		spin_lock(&vtable_lock[flow_vtbl_type]);
		/* locate the prev vtable we are going to insert
		 * the new vtable into. */
		while ((prev_vtbl->next != NULL) &&
				(prev_vtbl->next->vtable_type <= vtbl_type)) {
			prev_vtbl = prev_vtbl->next;
		};
	}

	ret = vtable_setup_def_class(flow_vtbl_type, &class_entry);
	if (ret != 0) {
		spin_unlock(&vtable_lock[flow_vtbl_type]);
		return NULL;
	}
	def_act = vtable_get_def_act(flow_vtbl_type);
	if (def_act < 0) {
		spin_unlock(&vtable_lock[flow_vtbl_type]);
		return NULL;
	}

	new_table = cs_vtable_alloc(&class_entry, def_act, vtbl_type);
	if (new_table == NULL) {
		spin_unlock(&vtable_lock[flow_vtbl_type]);
		return NULL;
	}
	new_table->vtable_type = vtbl_type;

	if (prev_vtbl != NULL)
		ret = cs_vtable_insert_to_chain(new_table, prev_vtbl);
	else
		ret = vtable_setup_def_hashmask(new_table);
	if (ret != 0)
		goto EXIT_FREE_TABLE;

	if (vtable_list[flow_vtbl_type] == NULL)
		vtable_list[flow_vtbl_type] = new_table;

	spin_unlock(&vtable_lock[flow_vtbl_type]);

	return new_table;
EXIT_FREE_TABLE:
	spin_unlock(&vtable_lock[flow_vtbl_type]);
	cs_vtable_free(new_table);

	return NULL;
} /* cs_core_vtable_alloc */

/* release a vtable.  This function will release a given vtable. When a vtable
 * is to be released, all hash entries (and QoS entries) of the table will be
 * freed too.  If there is vtable connected to it, it will also take care the
 * re-chaining the vtables. If the table given is the first one of the chain,
 * then it will wipe the whole chain. */
int cs_core_vtable_release(cs_vtable_t *table)
{
	cs_vtable_t *head_table;

	if (table == NULL)
		return -1;

	head_table = cs_core_vtable_get_head(table->vtable_type);
	if (head_table == NULL)
		return -1;

	if (table->prev == NULL) {
		/* the first one of the chain. wipe all!! */
		while (table->next != NULL)
			cs_core_vtable_release(table->next);
	} else {
		spin_lock(&vtable_lock[head_table->vtable_type]);
		cs_vtable_remove_from_chain(table);
		spin_unlock(&vtable_lock[head_table->vtable_type]);
	}

	return cs_core_vtable_free(table);
} /* cs_core_vtable_release */

/* free vtable.  This API doesn't care about re-link of any previous and/or
 * next vtable in the same chain.  It just forcefully frees the vtable and all
 * the resources allocated. */
int cs_core_vtable_free(cs_vtable_t *table)
{
	return cs_vtable_free(table);
} /* cs_core_vtable_free */

/* release vtable by type
 * this API releases all the vtables associated with the given vtbl_type,
 * and it takes care of the chaining too.  However, if given vtbl_type
 * is flow vtable type, then it will wipe the whole chain! */
int cs_core_vtable_release_by_type(unsigned int vtbl_type)
{
	cs_vtable_t *table;
	int ret;

	table = cs_core_vtable_get(vtbl_type);
	while (table != NULL) {
		ret = cs_core_vtable_release(table);
		if (ret != 0)
			return ret;
		table = cs_core_vtable_get(vtbl_type);
	};
	return 0;
} /* cs_core_vtable_release_by_type */

/* get the first occurrence of the vtable with given vtbl_type */
cs_vtable_t *cs_core_vtable_get(unsigned int vtbl_type)
{
	cs_vtable_t *table;

	if (vtbl_type < CORE_VTABLE_TYPE_MAX) {
		return vtable_list[vtbl_type];
	} else if (vtbl_type < CORE_VTABLE_RULE_TYPE_MAX) {
		table = cs_core_vtable_get_head(vtbl_type);

		if (table == NULL)
			return table;
		while ((table != NULL) && (table->vtable_type < vtbl_type)) {
			table = table->next;
		};
		if ((table != NULL) &&
				(table->vtable_type == vtbl_type))
			return table;
	}
	return NULL;
} /* cs_core_vtable_get */

/* get the head of the chain where the vtable type belongs to */
cs_vtable_t *cs_core_vtable_get_head(unsigned int vtbl_type)
{
	int flow_vtbl_type;

	if (vtbl_type < CORE_VTABLE_TYPE_MAX) {
		return vtable_list[vtbl_type];
	} else if (vtbl_type < CORE_VTABLE_RULE_TYPE_MAX) {
		flow_vtbl_type = get_flow_from_rule_vtbl_type(vtbl_type);
		if (unlikely((flow_vtbl_type == CORE_VTABLE_TYPE_NONE)))
			return NULL;
		else
			return vtable_list[flow_vtbl_type];
	}
	return NULL;
} /* cs_core_vtable_get_head */


/* Adding hashmask to SDB of the vtable. If succeeds, the allocated
 * hash mask index will return through the pointer given by hm_idx */
int cs_core_vtable_add_hashmask(unsigned int vtbl_type,
		fe_hash_mask_entry_t *hash_mask, unsigned int priority,
		bool is_qos, unsigned int *hm_idx)
{
	cs_vtable_t *table;
	int ret;
	bool is_found = false;

	/* find the table!! */
	table = cs_core_vtable_get(vtbl_type);
	while ((table != NULL) && (is_found == false)) {
		if (cs_vtable_has_avail_hashmask_space(table, is_qos) == true) {
			is_found = true;
		} else {
			if ((table->next != NULL) &&
					(table->next->vtable_type == vtbl_type))
				table = table->next;
			else
				table = NULL;
		}
	}

	if (is_found == false)
		return -1;

	/* add the hashmask */
	ret = cs_vtable_add_hashmask(table, hash_mask, priority, is_qos);
	if (ret < 0)
		return -1;

	*hm_idx = ret;
	return 0;
} /* cs_core_vtable_add_hashmask */

/* Deleting hashmask from SDB of the vtable given with vtbl_type,
 * it will use the hash_mask entry info given to find the index */
int cs_core_vtable_del_hashmask(unsigned int vtbl_type,
		fe_hash_mask_entry_t *hash_mask, bool is_qos)
{
	cs_vtable_t *table;
	int ret;

	if (hash_mask == NULL)
		return -1;

	/* find the table!! */
	table = cs_core_vtable_get(vtbl_type);
	while (table != NULL) {
		ret = cs_vtable_del_hashmask(table, hash_mask, is_qos);
		if (ret != 0)
			return ret;
		if ((table->next != NULL) &&
				(table->next->vtable_type == vtbl_type))
			table = table->next;
		else
			table = NULL;
	}

	return 0;
} /* cs_core_vtable_del_hashmask */

/* deleting hashmask from SDB of the vtable given with vtbl_type,
 * it will find the matching hm_idx from SDB and remove it */
int cs_core_vtable_del_hashmask_by_idx(unsigned int vtbl_type,
		unsigned int hm_idx, bool is_qos)
{
	cs_vtable_t *table;
	int ret;

	/* find the table!! */
	table = cs_core_vtable_get(vtbl_type);
	while (table != NULL) {
		ret = cs_vtable_del_hashmask_by_idx(table, hm_idx, is_qos);
		if (ret != 0)
			return ret;
		if ((table->next != NULL) &&
				(table->next->vtable_type == vtbl_type))
			table = table->next;
		else
			table = NULL;
	}
	return 0;
} /* cs_core_vtable_del_hashmask_by_idx */

/* get the hashmask flag with given fwd application type flow */
int cs_core_vtable_get_hashmask_flag_from_apptype(unsigned int app_type,
		u64 *app_hm_flag)
{
	if (app_type >= CORE_APP_TYPE_MAX)
		return -1;
	if (app_hm_flag == NULL)
		return -1;
	*app_hm_flag = apptype_hashmask_tbl[app_type];
	return 0;
} /* cs_core_vtable_get_hashmask_flag_from_apptype */

/* set the hashmask flag to the given fwd application type flow */
int cs_core_vtable_set_hashmask_flag_for_apptype(unsigned int app_type,
		u64 app_hm_flag)
{
	// FIXME!! implement!
#if 0
	if (app_type >= CORE_APP_TYPE_MAX)
		return -1;
	apptype_hashmask_tbl[app_type] = app_hm_flag;
#endif
	return 0;
} /* cs_core_vtable_set_hashmask_flag_for_apptype */

int cs_core_vtable_set_entry_valid(unsigned int vtbl_type, u8 valid)
{
	fe_class_entry_t entry;
	cs_vtable_t *table;
	int ret;

	if (valid > 1)
		return -1;
	/* find the table!! */
	table = cs_core_vtable_get(vtbl_type);
	while (table != NULL) {
		ret = cs_fe_table_get_entry(FE_TABLE_CLASS, table->class_index, &entry);
		if (ret != 0)
			return ret;
		entry.entry_valid = valid;
		ret = cs_fe_table_set_entry(FE_TABLE_CLASS, table->class_index, &entry);
		if (ret != 0)
			return ret;
		if ((table->next != NULL) &&
				(table->next->vtable_type == vtbl_type))
			table = table->next;
		else
			table = NULL;
	}
	return 0;
}

void cs_core_vtable_dump(void)
{
	int i, count;
	cs_vtable_t *table;
	fe_class_entry_t entry;

	for (i = 0; i < CORE_VTABLE_TYPE_MAX; i++) {
		count = 1;
		printk("Vtable Type#%d -------------------------\n", i);
		table = vtable_list[i];
		while (table != NULL) {
			printk("\tTable#%d:\n", count);
			printk("\t\tclass_index = %d\n", table->class_index);
			printk("\t\tsdb_index = %d\n", table->sdb_index);
			if (!cs_fe_table_get_entry(FE_TABLE_CLASS, table->class_index, &entry)) {				
				printk("\t\tvalid = %d\n", entry.entry_valid);
				printk("\t\tpriority = %d\n", entry.rule_priority);
			}
			printk("\t\tmcgid = %d\n", table->mcgid);			
			printk("\t\tuuflow_idx = %d\n", table->uuflow_idx);
			printk("\t\tumflow_idx = %d\n", table->umflow_idx);
			printk("\t\tbcflow_idx = %d\n", table->bcflow_idx);
			printk("--------------------------\n");
			table = table->next;
			count++;
		}
		printk("\n");
	}
	return;
} /* cs_core_vtable_dump */


