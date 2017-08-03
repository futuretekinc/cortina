/*
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef _RTL8370M_VB_H_
#define _RTL8370M_VB_H_
#include <rtk_api.h>

#define RTL83XX_DRIVER_NAME	"rtl8370"
#define SWITCH_DEVICE_NAME	"switch"
#define CPU_PORT_ID			2
#define SWITCH_PHY_ADDR		0
#define SWITCH_CPU_PORT		RTK_EXT_1_MAC
#define RTL83XX_DBG			0

/* #define RTL83XX_L2_ISOLATION	1 */


#define PHY_ID_RTL8211		0x001cc910
#define PHY_ID_MASK			0xFFFFFFF0

typedef struct {
	unsigned short cmd;         // refer to RTK_COMMAND_DEF

	/* parameters for commands */
	union {
		/* rtk_switch_maxPktLen_set(rtk_switch_maxPktLen_t len) */
		/* rtk_switch_maxPktLen_get(rtk_switch_maxPktLen_t *pLen) */
		struct rtk_switch_max_pktlen_para {
			rtk_switch_maxPktLen_t len;
		} switch_max_pktlen;

		/* rtk_switch_greenEthernet_set(rtk_enable_t enable) */
		/* rtk_switch_greenEthernet_get(rtk_enable_t *pEnable) */
		struct rtk_switch_green_ethernet_para {
			rtk_enable_t enable;
		} switch_green_ethernet;

		/* rtk_rate_shareMeter_set(rtk_meter_id_t index, rtk_rate_t rate, rtk_enable_t ifg_include) */
		/* rtk_rate_shareMeter_get(rtk_meter_id_t index, rtk_rate_t *pRate ,rtk_enable_t *pIfg_include) */
		struct rtk_rate_sharemeter_para {
			rtk_meter_id_t index;
			rtk_rate_t rate;
			rtk_enable_t ifg_include;
		} rate_sharemeter;

		/* rtk_rate_igrBandwidthCtrlRate_set( rtk_port_t port, rtk_rate_t rate,  rtk_enable_t ifg_include, rtk_enable_t fc_enable) */
		/* rtk_rate_igrBandwidthCtrlRate_get(rtk_port_t port, rtk_rate_t *pRate, rtk_enable_t *pIfg_include, rtk_enable_t *pFc_enable) */
		struct rtk_rate_igr_bw_para {
			rtk_port_t port;
			rtk_rate_t rate;
			rtk_enable_t ifg_include;
			rtk_enable_t fc_enable;
		} rate_igr_bw;

		/* rtk_rate_egrBandwidthCtrlRate_set(rtk_port_t port, rtk_rate_t rate,  rtk_enable_t ifg_include) */
		/* rtk_rate_egrBandwidthCtrlRate_get(rtk_port_t port, rtk_rate_t *pRate, rtk_enable_t *pIfg_include) */
		struct rtk_rate_egr_bw_para {
			rtk_port_t port;
			rtk_rate_t rate;
			rtk_enable_t ifg_include;
		} rate_egr_bw;

		/* rtk_rate_egrQueueBwCtrlEnable_set(rtk_port_t port, rtk_qid_t queue, rtk_enable_t enable) */
		/* rtk_rate_egrQueueBwCtrlEnable_get(rtk_port_t port, rtk_qid_t queue, rtk_enable_t *pEnable) */
		struct rtk_rate_egr_qbw_en_para {
			rtk_port_t port;
			rtk_qid_t queue;
			rtk_enable_t enable;
		} rate_egr_qbw_en;

		/* rtk_rate_egrQueueBwCtrlRate_set(rtk_port_t port, rtk_qid_t queue, rtk_meter_id_t index) */
		/* rtk_rate_egrQueueBwCtrlRate_get(rtk_port_t port, rtk_qid_t queue, rtk_meter_id_t *pIndex) */
		struct rtk_rate_egr_qbw_para {
			rtk_port_t port;
			rtk_qid_t queue;
			rtk_meter_id_t index;
		} rate_egr_qbw;

		/* rtk_storm_controlRate_set(rtk_port_t port, rtk_rate_storm_group_t storm_type, rtk_rate_t rate, rtk_enable_t ifg_include, rtk_uint32 mode) */
		/* rtk_storm_controlRate_get(rtk_port_t port, rtk_rate_storm_group_t storm_type, rtk_rate_t *pRate, rtk_enable_t *pIfg_include, rtk_uint32 mode) */
		struct rtk_storm_ctrl_rate_para {
			rtk_port_t port;
			rtk_rate_storm_group_t storm_type;
			rtk_rate_t rate;
			rtk_enable_t ifg_include;
			rtk_uint32 mode;
		} storm_ctrl_rate;

		/* rtk_storm_bypass_set(rtk_storm_bypass_t type, rtk_enable_t enable) */
		/* rtk_storm_bypass_get(rtk_storm_bypass_t type, rtk_enable_t *pEnable) */
		struct rtk_storm_bypass_para {
			rtk_storm_bypass_t type;
			rtk_enable_t enable;
		} storm_bypass;

		/* rtk_qos_init(rtk_queue_num_t queueNum) */
		struct rtk_qos_para {
			rtk_queue_num_t queue_num;
		} qos;
		
		/* rtk_qos_priSel_set(rtk_priority_select_t *pPriDec) */
		/* rtk_qos_priSel_get(rtk_priority_select_t *pPriDec) */
		struct rtk_qos_pri_sel_para {
			rtk_priority_select_t priDec; /* struct rtk_priority_select_s */
		} qos_pri_sel;
		
		/* rtk_qos_1pPriRemap_set(rtk_pri_t dot1p_pri, rtk_pri_t int_pri) */
		/* rtk_qos_1pPriRemap_get(rtk_pri_t dot1p_pri, rtk_pri_t *pInt_pri) */
		struct rtk_qos_dot1p_pri_remap_para {
			rtk_pri_t dot1p_pri;
			rtk_pri_t int_pri;
		} qos_dot1p_pri_remap;
		
		/* rtk_qos_dscpPriRemap_set(rtk_dscp_t dscp, rtk_pri_t int_pri) */
		/* rtk_qos_dscpPriRemap_get(rtk_dscp_t dscp, rtk_pri_t *pInt_pri) */
		struct rtk_qos_dscp_pri_remap_para {
			rtk_dscp_t dscp;
			rtk_pri_t int_pri;
		} qos_dscp_pri_remap;
		
		/* rtk_qos_portPri_set(rtk_port_t port, rtk_pri_t int_pri) */
		/* rtk_qos_portPri_get(rtk_port_t port, rtk_pri_t *pInt_pri) */
		struct rtk_qos_port_pri_para {
			rtk_port_t port;
			rtk_pri_t int_pri;
		} qos_port_pri;
		
		/* rtk_qos_queueNum_set(rtk_port_t port, rtk_queue_num_t queue_num) */
		/* rtk_qos_queueNum_get(rtk_port_t port, rtk_queue_num_t *pQueue_num) */
		struct rtk_qos_que_num_para {
			rtk_port_t port;
			rtk_queue_num_t queue_num;
		} qos_que_num;
		
		/* rtk_qos_priMap_set(rtk_queue_num_t queue_num, rtk_qos_pri2queue_t *pPri2qid) */
		/* rtk_qos_priMap_get(rtk_queue_num_t queue_num, rtk_qos_pri2queue_t *pPri2qid) */
		struct rtk_qos_pri_map_para {
			rtk_queue_num_t queue_num;
			rtk_qos_pri2queue_t pri2qid; /* struct rtk_qos_pri2queue_s */
		} qos_pri_map;

		/* rtk_qos_schedulingQueue_set(rtk_port_t port, rtk_qos_queue_weights_t *pQweights) */
		/* rtk_qos_schedulingQueue_get(rtk_port_t port, rtk_qos_queue_weights_t *pQweights) */
		struct rtk_qos_sche_que_para {
			rtk_port_t port;
			rtk_qos_queue_weights_t qweights; /* struct rtk_qos_queue_weights_s */
		} qos_sche_que;
		
		/* rtk_qos_1pRemarkEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_qos_1pRemarkEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_qos_dot1p_remark_en_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} qos_dot1p_remark_en;
		
		/* rtk_qos_1pRemark_set(rtk_pri_t int_pri, rtk_pri_t dot1p_pri) */
		/* rtk_qos_1pRemark_get(rtk_pri_t int_pri, rtk_pri_t *pDot1p_pri) */
		struct rtk_qos_dot1p_remark_para {
			rtk_pri_t int_pri;
			rtk_pri_t dot1p_pri;
		} qos_dot1p_remark;
		
		/* rtk_qos_dscpRemarkEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_qos_dscpRemarkEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_qos_dscp_remark_en_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} qos_dscp_remark_en;
		
		/* rtk_qos_dscpRemark_set(rtk_pri_t int_pri, rtk_dscp_t dscp) */
		/* rtk_qos_dscpRemark_get(rtk_pri_t int_pri, rtk_dscp_t *pDscp) */
		struct rtk_qos_dscp_remark_para {
			rtk_pri_t int_pri;
			rtk_dscp_t dscp;
		} qos_dscp_remark;
		
		/* rtk_port_phyAutoNegoAbility_set(rtk_port_t port, rtk_port_phy_ability_t *pAbility) */
		/* rtk_port_phyAutoNegoAbility_get(rtk_port_t port, rtk_port_phy_ability_t *pAbility) */
		/* rtk_port_phyForceModeAbility_set(rtk_port_t port, rtk_port_phy_ability_t *pAbility) */
		/* rtk_port_phyForceModeAbility_get(rtk_port_t port, rtk_port_phy_ability_t *pAbility) */
		struct rtk_port_phy_ability_para {
			rtk_port_t port;
			rtk_port_phy_ability_t ability; /* struct rtk_port_phy_ability_s */
		} port_phy_an_ability;

		struct rtk_port_phy_ability_para port_phy_force_ability;
		
		/* rtk_port_phyStatus_get(rtk_port_t port, rtk_port_linkStatus_t *pLinkStatus, rtk_port_speed_t *pSpeed, rtk_port_duplex_t *pDuplex) */
		struct rtk_port_phy_status_para {
			rtk_port_t port;
			rtk_port_linkStatus_t linkStatus;
			rtk_port_speed_t speed;
			rtk_port_duplex_t duplex;
		} port_phy_status;

		/* rtk_port_macForceLinkExt_set(rtk_ext_port_t port, rtk_mode_ext_t mode, rtk_port_mac_ability_t *pPortability) */
		/* rtk_port_macForceLinkExt_get(rtk_ext_port_t port, rtk_mode_ext_t *pMode, rtk_port_mac_ability_t *pPortability) */
		struct rtk_port_mac_force_link_ext_para {
//			rtk_ext_port_t port; -- XTRA
			rtk_mode_ext_t mode;
			rtk_port_mac_ability_t ability; /* struct rtk_port_mac_ability_s */
		} port_mac_force_link_ext;

		/* rtk_port_macStatus_get(rtk_port_t port, rtk_port_mac_ability_t *pPortstatus) */
		struct rtk_port_mac_status_para {
			rtk_port_t port;
			rtk_port_mac_ability_t status; /* struct rtk_port_mac_ability_s */
		} port_mac_status;
		
		/* rtk_port_phyReg_set(rtk_port_t port, rtk_port_phy_reg_t reg, rtk_port_phy_data_t regData) */
		/* rtk_port_phyReg_get(rtk_port_t port, rtk_port_phy_reg_t reg, rtk_port_phy_data_t *pData) */
		struct rtk_port_phy_reg_para {
			rtk_port_t port;
			rtk_port_phy_reg_t reg;
			rtk_port_phy_data_t data;
		} port_phy_reg;

		/* rtk_port_adminEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_port_adminEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_port_admin_state_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} port_admin_state;

		/* rtk_port_isolation_set(rtk_port_t port, rtk_portmask_t portmask) */
		/* rtk_port_isolation_get(rtk_port_t port, rtk_portmask_t *pPortmask) */
		struct rtk_port_isolation_para {
			rtk_port_t port;
			rtk_portmask_t portmask; /* struct rtk_portmask_s */
		} port_isolation;
		
		/* rtk_port_rgmiiDelayExt0_set(rtk_data_t txDelay, rtk_data_t rxDelay) */
		/* rtk_port_rgmiiDelayExt0_get(rtk_data_t *pTxDelay, rtk_data_t *pRxDelay) */
		/* rtk_port_rgmiiDelayExt1_set(rtk_data_t txDelay, rtk_data_t rxDelay) */
		/* rtk_port_rgmiiDelayExt1_get(rtk_data_t *pTxDelay, rtk_data_t *pRxDelay) */
		/* rtk_port_rgmiiDelayExt_set(rtk_ext_port_t port, rtk_data_t txDelay, rtk_data_t rxDelay) */
		/* rtk_port_rgmiiDelayExt_get(rtk_ext_port_t port, rtk_data_t *pTxDelay, rtk_data_t *pRxDelay) */
		struct rtk_port_rgmii_delay_para {
//			rtk_ext_port_t port; -- XTRA
			rtk_data_t txDelay;
			rtk_data_t rxDelay;
		} port_rgmii_delay;
		
		/* rtk_port_phyEnableAll_set(rtk_enable_t enable) */
		/* rtk_port_phyEnableAll_get(rtk_enable_t *pEnable) */
		struct rtk_port_enable_all_para {
			rtk_enable_t enable;
		} port_enable_all;
		
		/* rtk_vlan_set(rtk_vlan_t vid, rtk_portmask_t mbrmsk, rtk_portmask_t untagmsk, rtk_fid_t fid) */
		/* rtk_vlan_get(rtk_vlan_t vid, rtk_portmask_t *pMbrmsk, rtk_portmask_t *pUntagmsk, rtk_fid_t *pFid) */
		struct rtk_vlan_para {
			rtk_vlan_t vid;
			rtk_portmask_t mbrmsk;   /* struct rtk_portmask_s */
			rtk_portmask_t untagmsk; /* struct rtk_portmask_s */
			rtk_fid_t fid;
 		} vlan;
		
		/* rtk_vlan_portPvid_set(rtk_port_t port, rtk_vlan_t pvid, rtk_pri_t priority) */
		/* rtk_vlan_portPvid_get(rtk_port_t port, rtk_vlan_t *pPvid, rtk_pri_t *pPriority) */
		struct rtk_vlan_pvid_para {
			rtk_port_t port;
			rtk_vlan_t pvid;
			rtk_pri_t priority;
		} vlan_pvid;
		
		/* rtk_vlan_portIgrFilterEnable_set(rtk_port_t port, rtk_enable_t igr_filter) */
		/* rtk_vlan_portIgrFilterEnable_get(rtk_port_t port, rtk_enable_t *pIgr_filter) */
		struct rtk_vlan_port_igrfilter_en_para {
			rtk_port_t port;
			rtk_enable_t igr_filter;
		} vlan_port_igrfilter_en;
		
		/* rtk_vlan_portAcceptFrameType_set(rtk_port_t port, rtk_vlan_acceptFrameType_t accept_frame_type) */
		/* rtk_vlan_portAcceptFrameType_get(rtk_port_t port, rtk_vlan_acceptFrameType_t *pAccept_frame_type) */
		struct rtk_vlan_port_aft_para {
			rtk_port_t port;
			rtk_vlan_acceptFrameType_t accept_frame_type;
		} vlan_port_aft;
		
		/* rtk_vlan_vlanBasedPriority_set(rtk_vlan_t vid, rtk_pri_t priority) */
		/* rtk_vlan_vlanBasedPriority_get(rtk_vlan_t vid, rtk_pri_t *pPriority) */
		struct rtk_vlan_based_pri_para {
			rtk_vlan_t vid;
			rtk_pri_t priority;
		} vlan_based_pri;
		
		/* rtk_vlan_tagMode_set(rtk_port_t port, rtk_vlan_tagMode_t tag_mode) */
		/* rtk_vlan_tagMode_get(rtk_port_t port, rtk_vlan_tagMode_t *pTag_mode) */
		struct rtk_vlan_tagmode_para {
			rtk_port_t port;
			rtk_vlan_tagMode_t tag_mode;
		} vlan_tagmode;

		/* rtk_vlan_stg_set(rtk_vlan_t vid, rtk_stg_t stg) */
		/* rtk_vlan_stg_get(rtk_vlan_t vid, rtk_stg_t *pStg) */
		struct rtk_vlan_stg_para {
			rtk_vlan_t vid;
			rtk_stg_t stg;
		} vlan_stg;
		
		/* rtk_vlan_protoAndPortBasedVlan_add(rtk_port_t port, rtk_vlan_protoAndPortInfo_t info) */
		/* rtk_vlan_protoAndPortBasedVlan_get(rtk_port_t port, rtk_vlan_proto_type_t proto_type, rtk_vlan_protoVlan_frameType_t frame_type, rtk_vlan_protoAndPortInfo_t *pInfo) */
		/* rtk_vlan_protoAndPortBasedVlan_del(rtk_port_t port, rtk_vlan_proto_type_t proto_type, rtk_vlan_protoVlan_frameType_t frame_type) */
		/* rtk_vlan_protoAndPortBasedVlan_delAll(rtk_port_t port) */
		struct rtk_vlan_ppbased_vlan_para {
			rtk_port_t port;
			rtk_vlan_proto_type_t proto_type;
			rtk_vlan_protoVlan_frameType_t frame_type;
			rtk_vlan_protoAndPortInfo_t info; /* struct rtk_vlan_protoAndPortInfo_s */
		} vlan_ppbased_vlan;
		
		/* rtl8370b_setAsicVlanFilter(rtk_uint32 enabled) */
		/* rtl8370b_getAsicVlanFilter(rtk_uint32* pEnabled) */
		struct rtk_vlan_mbr_filter_para {
			rtk_uint32 enable;
		} vlan_mbr_filter;

		/* rtk_stp_mstpState_set(rtk_stp_msti_id_t msti, rtk_port_t port, rtk_stp_state_t stp_state) */
		/* rtk_stp_mstpState_get(rtk_stp_msti_id_t msti, rtk_port_t port, rtk_stp_state_t *pStp_state) */
		struct rtk_stp_mstp_state_para {
			rtk_stp_msti_id_t msti;
			rtk_port_t port;
			rtk_stp_state_t stp_state;
		} stp_mstp_state;
		
		/* rtk_l2_addr_add(rtk_mac_t *pMac, rtk_l2_ucastAddr_t *pL2_data) */
		/* rtk_l2_addr_get(rtk_mac_t *pMac, rtk_l2_ucastAddr_t *pL2_data) */
		/* rtk_l2_addr_next_get(rtk_l2_read_method_t read_method, rtk_port_t port, rtk_uint32 *pAddress, rtk_l2_ucastAddr_t *pL2_data) */
		/* rtk_l2_addr_del(rtk_mac_t *pMac, rtk_l2_ucastAddr_t *pL2_data) */

		struct rtk_l2_addr_para {
//			rtk_l2_read_method_t read_method;	-- XTRA
			rtk_port_t port;
			rtk_uint32 address;
			rtk_mac_t mac; /* struct rtk_mac_s */
			rtk_l2_ucastAddr_t l2_data; /* struct rtk_l2_ucastAddr_s */
		} l2_addr;
		
		/* rtk_l2_mcastAddr_add(rtk_mac_t *pMac, rtk_data_t ivl, rtk_data_t cvid_fid, rtk_portmask_t portmask) */
		/* rtk_l2_mcastAddr_get(rtk_mac_t *pMac, rtk_data_t ivl, rtk_data_t cvid_fid, rtk_portmask_t *pPortmask) */
		/* rtk_l2_mcastAddr_next_get(rtk_uint32 *pAddress, rtk_mac_t *pMac, rtk_data_t *pIvl, rtk_data_t *pCvid_fid, rtk_portmask_t *pPortmask) */
		/* rtk_l2_mcastAddr_del(rtk_mac_t *pMac, rtk_data_t ivl, rtk_data_t cvid_fid) */
		struct rtk_l2_mcaddr_para {
			rtk_uint32 address;
			rtk_mac_t mac; /* struct rtk_mac_s */
			rtk_data_t ivl;
			rtk_data_t cvid_fid;
			rtk_portmask_t portmask; /* struct rtk_portmask_s */
		} l2_mcaddr;
		
		/* rtk_l2_ipMcastAddr_add(ipaddr_t sip, ipaddr_t dip, rtk_portmask_t portmask) */
		/* rtk_l2_ipMcastAddr_get(ipaddr_t sip, ipaddr_t dip, rtk_portmask_t *pPortmask) */
		/* rtk_l2_ipMcastAddr_next_get(rtk_uint32 *pAddress, ipaddr_t *pSip, ipaddr_t *pDip, rtk_portmask_t *pPortmask) */
		/* rtk_l2_ipMcastAddr_del(ipaddr_t sip, ipaddr_t dip) */
		struct rtk_l2_ipmcaddr_para {
			rtk_uint32 address;
			ipaddr_t sip;
			ipaddr_t dip;
			rtk_portmask_t portmask; /* struct rtk_portmask_s */
		} l2_ipmcaddr;
		
		/* rtk_l2_flushType_set(rtk_l2_flushType_t type, rtk_vlan_t vid, rtk_uint32 portOrTid) */
		struct rtk_l2_flush_type_para {
			rtk_l2_flushType_t type;
			rtk_vlan_t vid;
			rtk_uint32 portOrTid;
		} l2_flush_type;
		
		/* rtk_l2_flushLinkDownPortAddrEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_l2_flushLinkDownPortAddrEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_l2_flush_lkdn_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} l2_flush_lkdn;

		/* rtk_l2_agingEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_l2_agingEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_l2_aging_en_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} l2_aging_en;

		/* rtk_l2_limitLearningCnt_set(rtk_port_t port, rtk_mac_cnt_t mac_cnt) */
		/* rtk_l2_limitLearningCnt_get(rtk_port_t port, rtk_mac_cnt_t *pMac_cnt) */
		struct rtk_l2_lrn_lmt_para {
			rtk_port_t port;
			rtk_mac_cnt_t mac_cnt;
		} l2_lrn_lmt;
		
		/* rtk_l2_limitLearningCntAction_set(rtk_port_t port, rtk_l2_limitLearnCntAction_t action) */
		/* rtk_l2_limitLearningCntAction_get(rtk_port_t port, rtk_l2_limitLearnCntAction_t *pAction) */
		struct rtk_l2_lrn_action_para {
			rtk_port_t port;
			rtk_l2_limitLearnCntAction_t action;
		} l2_lrn_action;

		/* rtk_l2_learningCnt_get(rtk_port_t port, rtk_mac_cnt_t *pMac_cnt) */
		struct rtk_l2_lrn_cnt_para {
			rtk_port_t port;
			rtk_mac_cnt_t mac_cnt;
		} l2_lrn_cnt;

		/* rtk_l2_floodPortMask_set(rtk_l2_flood_type_t flood_type, rtk_portmask_t flood_portmask) */
		/* rtk_l2_floodPortMask_get(rtk_l2_flood_type_t flood_type, rtk_portmask_t *pFlood_portmask) */
		struct rtk_l2_flood_ports_para {
			rtk_l2_flood_type_t flood_type;
			rtk_portmask_t flood_portmask; /* struct rtk_portmask_s */
		} l2_flood_ports;

		/* rtk_l2_localPktPermit_set(rtk_port_t port, rtk_enable_t permit) */
		/* rtk_l2_localPktPermit_get(rtk_port_t port, rtk_enable_t *pPermit) */
		struct rtk_l2_localpkt_pmt_para {
			rtk_port_t port;
			rtk_enable_t permit;
		} l2_localpkt_pmt;
		
		/* rtk_l2_aging_set(rtk_l2_age_time_t aging_time) */
		/* rtk_l2_aging_get(rtk_l2_age_time_t *pAging_time) */
		struct rtk_l2_aging_para {
			rtk_l2_age_time_t aging_time;
		} l2_aging;
		
		/* rtk_l2_ipMcastAddrLookup_set(rtk_l2_lookup_type_t type) */
		/* rtk_l2_ipMcastAddrLookup_get(rtk_l2_lookup_type_t *pType) */
		struct rtk_l2_ipmcaddr_lkup_para {
			rtk_l2_lookup_type_t type;
		} l2_ipmcaddr_lkup;
		
		/* rtk_l2_entry_get(rtk_l2_addr_table_t *pL2_entry) */
		struct rtk_l2_entry_para {
			rtk_l2_addr_table_t l2_entry; /* struct rtk_l2_addr_table_s */
		} l2_entry;
		
		/* rtk_svlan_servicePort_add(rtk_port_t port) */
		/* rtk_svlan_servicePort_get(rtk_portmask_t *pSvlan_portmask) */
		/* rtk_svlan_servicePort_del(rtk_port_t port) */
		struct rtk_svlan_svc_port_para {
			rtk_port_t port;
			rtk_portmask_t svlan_portmask; /* struct rtk_portmask_s */
		} svlan_svc_port;
		
		/* rtk_svlan_tpidEntry_set(rtk_uint32 svlan_tag_id) */
		/* rtk_svlan_tpidEntry_get(rtk_uint32 *pSvlan_tag_id) */
		struct rtk_svlan_tpid_para {
			rtk_uint32 svlan_tag_id;
		} svlan_tpid;
		
		/* rtk_svlan_priorityRef_set(rtk_svlan_pri_ref_t ref) */
		/* rtk_svlan_priorityRef_get(rtk_svlan_pri_ref_t *pRef) */
		struct rtk_svlan_pri_ref_para {
			rtk_svlan_pri_ref_t ref;
		} svlan_pri_ref;
		
		/* rtk_svlan_memberPortEntry_set(rtk_uint32 svid_idx, rtk_svlan_memberCfg_t *psvlan_cfg) */
		/* rtk_svlan_memberPortEntry_get(rtk_uint32 svid_idx, rtk_svlan_memberCfg_t *pSvlan_cfg) */
		struct rtk_svlan_mbrport_entry_para {
			rtk_uint32 svid_idx;
			rtk_svlan_memberCfg_t svlan_cfg; /* struct rtk_svlan_memberCfg_s */
		} svlan_mbrport_entry;
		
		/* rtk_svlan_defaultSvlan_set(rtk_port_t port, rtk_vlan_t svid) */
		/* rtk_svlan_defaultSvlan_get(rtk_port_t port, rtk_vlan_t *pSvid) */
		struct rtk_svlan_def_svid_para {
			rtk_port_t port;
			rtk_vlan_t svid;
		} svlan_def_svid;
		
		/* rtk_svlan_c2s_add(rtk_vlan_t vid, rtk_port_t src_port, rtk_vlan_t svid) */
		/* rtk_svlan_c2s_del(rtk_vlan_t vid, rtk_port_t src_port) */
		/* rtk_svlan_c2s_get(rtk_vlan_t vid, rtk_port_t src_port, rtk_vlan_t *pSvid) */
		struct rtk_svlan_c2s_para {
			rtk_vlan_t vid;
			rtk_port_t src_port;
			rtk_vlan_t svid;
		} svlan_c2s;
		
		/* rtk_svlan_unmatch_action_set(rtk_svlan_unmatch_action_t action, rtk_vlan_t svid) */
		/* rtk_svlan_unmatch_action_get(rtk_svlan_unmatch_action_t *pAction, rtk_vlan_t *pSvid) */
		struct rtk_svlan_unmatch_act_para {
//			rtk_svlan_unmatch_action_t action; -- XTRA
			rtk_vlan_t svid;
		} svlan_unmatch_act;
		
		/* rtk_svlan_ipmc2s_add(ipaddr_t ipmc, rtk_vlan_t svid) */
		/* rtk_svlan_ipmc2s_del(ipaddr_t ipmc) */
		/* rtk_svlan_ipmc2s_get(ipaddr_t ipmc, rtk_vlan_t *pSvid) */
		struct rtk_svlan_ipmc2s_para {
			ipaddr_t ipmc;
			rtk_vlan_t svid;
		} svlan_ipmc2s;
		
		
		/* rtk_svlan_l2mc2s_add(rtk_vlan_t svid, rtk_mac_t mac) */
		/* rtk_svlan_l2mc2s_del(rtk_mac_t mac) */
		/* rtk_svlan_l2mc2s_get(rtk_mac_t mac, rtk_vlan_t *pSvid) */
		struct rtk_svlan_l2mc2s_para {
			rtk_vlan_t svid;
			rtk_mac_t mac; /* struct rtk_mac_s */
		} svlan_l2mc2s;
		
		
		/* rtk_svlan_sp2c_add(rtk_vlan_t svid, rtk_port_t dst_port, rtk_vlan_t cvid) */
		/* rtk_svlan_sp2c_get(rtk_vlan_t svid, rtk_port_t dst_port, rtk_vlan_t *pCvid) */
		/* rtk_svlan_sp2c_del(rtk_vlan_t svid, rtk_port_t dst_port) */
		struct rtk_svlan_sp2c_para {
			rtk_vlan_t svid;
			rtk_port_t dst_port;
			rtk_vlan_t cvid;
		} svlan_sp2c;

		/* rtk_cpu_enable_set(rtk_enable_t enable) */
		/* rtk_cpu_enable_get(rtk_enable_t *pEnable) */
		struct rtk_cpu_tag_para {
			rtk_enable_t enable;
		} cpu_tag;

		/* rtk_cpu_tagPort_set(rtk_port_t port, rtk_cpu_insert_t mode) */
		/* rtk_cpu_tagPort_get(rtk_port_t *pPort, rtk_cpu_insert_t *pMode) */
		struct rtk_cpu_tag_port_para {
			rtk_port_t port;
			rtk_cpu_insert_t mode;
		} cpu_tag_port;

		/* rtl8370b_setAsicCputagPosition(rtk_uint32 postion) */
		/* rtl8370b_getAsicCputagPosition(rtk_uint32* pPostion) */
		struct rtk_cpu_tag_position_para {
			rtk_uint32 position;
		} cpu_tag_position;

		
		/* rtk_mirror_portBased_set(rtk_port_t mirroring_port, rtk_portmask_t *pMirrored_rx_portmask, rtk_portmask_t *pMirrored_tx_portmask) */
		/* rtk_mirror_portBased_get(rtk_port_t* pMirroring_port, rtk_portmask_t *pMirrored_rx_portmask, rtk_portmask_t *pMirrored_tx_portmask) */
		struct rtk_mirror_portbased_para {
			rtk_port_t mirroring_port;
			rtk_portmask_t mirrored_rx_portmask; /* struct rtk_portmask_s */
			rtk_portmask_t mirrored_tx_portmask; /* struct rtk_portmask_s */
		} mirror_portbased;
		
		/* rtk_mirror_portIso_set(rtk_enable_t enable) */
		/* rtk_mirror_portIso_get(rtk_enable_t *pEnable) */
		struct rtk_mirror_portiso_para {
			rtk_enable_t enable;
		} mirror_portiso;
		
		/* rtk_stat_global_get(rtk_stat_global_type_t cntr_idx, rtk_stat_counter_t *pCntr) */
		/* rtk_stat_global_getAll(rtk_stat_global_cntr_t *pGlobal_cntrs) */
		struct rtk_stat_global_para {
			rtk_stat_global_type_t cntr_idx;
			rtk_stat_counter_t cntr;
			rtk_stat_global_cntr_t global_cntrs; /* struct rtk_stat_global_cntr_s */
		} stat_global;
		
		/* rtk_stat_port_reset(rtk_port_t port) */
		/* rtk_stat_port_get(rtk_port_t port, rtk_stat_port_type_t cntr_idx, rtk_stat_counter_t *pCntr) */
		/* rtk_stat_port_getAll(rtk_port_t port, rtk_stat_port_cntr_t *pPort_cntrs) */
		struct rtk_stat_port_para {
			rtk_port_t port;
			rtk_stat_port_type_t cntr_idx;
			rtk_stat_counter_t cntr;
			rtk_stat_port_cntr_t port_cntrs; /* struct rtk_stat_port_cntr_s */
		} stat_port;
		
		/* rtk_filter_igrAcl_field_add(rtk_filter_cfg_t *pFilter_cfg, rtk_filter_field_t *pFilter_field) */
		struct rtk_acl_field_para {
			rtk_filter_cfg_t filter_cfg; /* struct rtk_filter_cfg_t */
			rtk_filter_field_t filter_field; /* struct rtk_filter_field */
			
		} acl_field;
		
		/* rtk_filter_igrAcl_cfg_add(rtk_filter_id_t filter_id, rtk_filter_cfg_t *pFilter_cfg, rtk_filter_action_t *pAction, rtk_filter_number_t *ruleNum) */
		/* rtk_filter_igrAcl_cfg_del(rtk_filter_id_t filter_id) */
		/* rtk_filter_igrAcl_cfg_get(rtk_filter_id_t filter_id, rtk_filter_cfg_raw_t *pFilter_cfg, rtk_filter_action_t *pAction) */
		struct rtk_acl_cfg_para {
			rtk_filter_id_t filter_id;
			rtk_filter_cfg_t filter_cfg; /* struct rtk_filter_cfg_t */
			rtk_filter_cfg_raw_t filter_cfg_raw; /* struct rtk_filter_cfg_raw_t */
			rtk_filter_action_t action; /* struct rtk_filter_action_t */
			rtk_filter_number_t ruleNum;

			/* use array instead of linked list for filter_field in filter_cfg */
			rtk_uint32 field_num;
			rtk_filter_field_t filter_field[RTK_MAX_NUM_OF_FILTER_FIELD]; /* struct rtk_filter_field */
		} acl_cfg;
		
		/* rtk_filter_igrAcl_unmatchAction_set(rtk_port_t port, rtk_filter_unmatch_action_t action) */
		/* rtk_filter_igrAcl_unmatchAction_get(rtk_port_t port, rtk_filter_unmatch_action_t* action) */
		struct rtk_acl_umaction_para {
			rtk_port_t port;
			rtk_filter_unmatch_action_t action;
		} acl_umaction;
		
		/* rtk_filter_igrAcl_state_set(rtk_port_t port, rtk_filter_state_t state) */
		/* rtk_filter_igrAcl_state_get(rtk_port_t port, rtk_filter_state_t* state) */
		struct rtk_acl_state_para {
			rtk_port_t port;
			rtk_filter_state_t state;
		} acl_state;
		
		/* rtk_eee_portEnable_set(rtk_port_t port, rtk_enable_t enable) */
		/* rtk_eee_portEnable_get(rtk_port_t port, rtk_enable_t *pEnable) */
		struct rtk_eee_porten_para {
			rtk_port_t port;
			rtk_enable_t enable;
		} eee_porten;

		/* rtk_igmp_state_set(rtk_enable_t enabled) */
		/* rtk_igmp_state_get(rtk_enable_t * pEnabled) */
		struct rtk_igmp_state_para {
			rtk_enable_t enable;
		} igmp_state;

		/* rtk_igmp_static_router_port_set(rtk_portmask_t portmask) */
		/* rtk_igmp_static_router_port_get(rtk_portmask_t *pPortmask) */
		struct rtk_igmp_router_port_para {
			rtk_portmask_t portmask;
		} igmp_router_port;

		/* rtk_igmp_protocol_set(rtk_port_t port,rtk_igmp_protocol_t protocol,rtk_trap_igmp_action_t action) */
		/* rtk_igmp_protocol_get(rtk_port_t port,rtk_igmp_protocol_t protocol,rtk_trap_igmp_action_t * pAction) */
		struct rtk_igmp_protocol_para {
			rtk_port_t port;
//			rtk_igmp_protocol_t protocol; -- XTRA
			rtk_trap_igmp_action_t action;
		} igmp_protocol;
		
		/* PHY/SWITCH MDIO READ/WRITE parameters */
		struct mdio_para {
			unsigned int phy_addr; /* only valid for PHY MDIO CMD */
			unsigned int reg_addr;
			unsigned int data;
		} mdio;
	} para;

	/* return value from RTK APIs */
	int ret;
} RTK_CMD_T;


#endif /* _RTL8370M_VB_H_ */
