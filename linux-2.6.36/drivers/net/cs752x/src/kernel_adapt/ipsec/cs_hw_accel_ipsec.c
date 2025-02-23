/*
 * Copyright (c) Cortina-Systems Limited 2010.  All rights reserved.
 *                Wen Hsu <wen.hsu@cortina-systems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * cs_hw_accel_ipsec.c
 *
 * $Id$
 *
 * This file contains the implementation for CS IPsec Offload Kernel Module.
 */

#include <linux/if_ether.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/esp.h>
#include <net/ah.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/timer.h>
#include <linux/crypto.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include "cs_core_logic.h"
#include "cs_core_vtable.h"
#include "cs_hw_accel_ipsec.h"
#include "cs_core_hmu.h"
#include <cs752x_eth.h>
#include <linux/etherdevice.h>
#include "cs_hw_accel_manager.h"
#include "cs_accel_cb.h"


#ifdef CONFIG_CS752X_HW_ACCEL_ETHERIP
#include "cs_hw_accel_etherip.h"
#endif

#undef CS_IPSEC_DEBUG_MODE
#ifdef CS_IPSEC_DEBUG_MODE
#include <net/inet_ecn.h>
#endif

/* general data base */
static cs_ipsec_db_t ipsec_main_db;
static cs_boolean f_ipsec_enbl = FALSE;

spinlock_t ipsec_rx_lock;

#ifdef CS_IPC_ENABLED
static struct ipc_context *ipc_ctxt0, *ipc_ctxt1;
//static struct timer_list cs_ipc_timer;
static unsigned char f_re_status[2] = { CS_IPSEC_IPC_RE_DEAD,
	CS_IPSEC_IPC_RE_DEAD
};
#endif

#ifdef CONFIG_CS752X_PROC
#include "cs752x_proc.h"

extern u32 cs_adapt_debug;
#endif				/* CONFIG_CS752X_PROC */

#define DBG(x)	if (cs_adapt_debug & CS752X_ADAPT_IPSEC) (x)

cs_core_hmu_t ipsec_fwd_hmu_entry;
cs_core_hmu_value_t ipsec_fwd_hmu_value;

extern struct cs_kernel_hw_accel_jt hw_jt;
extern void ni_dm_byte(unsigned int location, int length);
extern int netif_rx_cs(struct sk_buff *skb);
extern u32 cs_hw_ipsec_offload_mode;

#define MIN(a,b) (((a) <= (b)) ? (a):(b))

#if 1				/* in first release, dir and table id are 1 to 1 mapping */
#define IPSEC_TABLE_ID_BY_DIR(dir) (dir)
#endif

#define CS_FE_LSPID_ARM	CPU_PORT	// Even though we only have 1 ARM core
					// in FPGA, how do we differ the source
					// port?

#ifdef CS_IPC_ENABLED
static int ipsec_ipc_send_stop(unsigned char op_id, unsigned short sa_id);
#endif
static int ipsec_skb_header_flush_cb_queue(unsigned char table_id,
				unsigned short sa_id);
static int ipsec_skb_header_flush_expire_cb_queue(unsigned char table_id,
				unsigned short sa_id);

#if 1
/* FIXME!! TEMPORARY DEFINE.. need to find a real place.. or real function
 * to retrieve those info */
#define CS_FE_INVALID_SW_ACTION 0x000fffff
#endif

#define CS_IPSEC_SPIDX_TIMEOUT 100; /*100 jiffies = 1 sec*/
extern int set_fwd_hash_result(cs_kernel_accel_cb_t * cb,
			       cs_fwd_hash_t * fwd_hash);
#if 0
/* FIXME!! inline will define unreachable and will cause build error due to stricker compiler in 3.4.11 Yocto build  */
extern inline unsigned int get_app_type_from_module_type(unsigned int mod_mask);
#else
extern unsigned int get_app_type_from_module_type(unsigned int mod_mask);
#endif /* FIXME */

extern void set_fwd_hash_result_index_to_cs_cb(cs_kernel_accel_cb_t * cs_cb,
					       cs_fwd_hash_t * fwd_hash);


/************************ internal APIs ****************************/
static inline int cs_ipsec_enable(void)
{
	return cs_accel_kernel_module_enable(CS752X_ADAPT_ENABLE_IPSEC);
}

static s8 cs_ipsec_get_cipher_alg(unsigned char *alg_name)
{
	unsigned char name[10][20] = { "ecb(cipher_null)", "cipher_null",
		"cbc(des)", "des", "cbc(des3_ede)", "des3_ede",
		"aes", "cbc(aes)", "rfc4106(gcm(aes))", "rfc4309(ccm(aes))"
	};
	u8 alg_num[10] = { CS_IPSEC_CIPHER_NULL, CS_IPSEC_CIPHER_NULL,
		CS_IPSEC_DES, CS_IPSEC_DES, CS_IPSEC_3DES, CS_IPSEC_3DES,
		CS_IPSEC_AES, CS_IPSEC_AES, CS_IPSEC_AES, CS_IPSEC_AES
	};
	u8 iii;

	for (iii = 0; iii < 10; iii++) {
		if (strncmp(alg_name, &name[iii][0], 20) == 0)
			return alg_num[iii];
	}
	return -1;
}

static u8 cs_ipsec_get_cipher_mode(u8 alg_mode)
{
	switch (alg_mode) {
	case SADB_EALG_NULL:
		return CS_IPSEC_CIPHER_ECB;
	case SADB_EALG_DESCBC:
	case SADB_EALG_3DESCBC:
	case SADB_X_EALG_AESCBC:
		return CS_IPSEC_CIPHER_CBC;
	case SADB_X_EALG_AESCTR:
		return CS_IPSEC_CIPHER_CTR;
	case SADB_X_EALG_AES_CCM_ICV8:
	case SADB_X_EALG_AES_CCM_ICV12:
	case SADB_X_EALG_AES_CCM_ICV16:
		return CS_IPSEC_CIPHER_CCM;
	case SADB_X_EALG_AES_GCM_ICV8:
	case SADB_X_EALG_AES_GCM_ICV12:
	case SADB_X_EALG_AES_GCM_ICV16:
		return CS_IPSEC_CIPHER_GCM;
	default:
		return -1;
	}
} /* cs_ipsec_get_cipher_mode */

static s8 cs_ipsec_get_auth_alg(unsigned char *alg_name)
{
	unsigned char name[5][16] = { "md5", "sha1", "sha224", "sha256",
		"digest_null"
	};
	unsigned char name_hmac[5][20] = { "hmac(md5)", "hmac(sha1)",
		"hmac(sha224)", "hmac(sha256)", "hmac(digest_null)"
	};
	u8 alg_num[5] = { CS_IPSEC_MD5, CS_IPSEC_SHA1, CS_IPSEC_SHA224,
		CS_IPSEC_SHA256, CS_IPSEC_AUTH_NULL
	};
	u8 iii;

	for (iii = 0; iii < 5; iii++) {
		if (strncmp(alg_name, &name[iii][0], 16) == 0)
			return alg_num[iii];
		if (strncmp(alg_name, &name_hmac[iii][0], 20) == 0)
			return alg_num[iii];
	}
	return -1;
} /* cs_ipsec_get_auth_alg */

static u32 calc_crc(u32 crc, u8 const *p, u32 len)
{
	int i;

	while (len--) {

		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY_LE : 0);
	}
	return crc;
}

static unsigned char ipsec_spidx_get(unsigned char op_id, unsigned short sa_id)
{
	unsigned short curr_mask;
	unsigned char offset = 0;
	spinlock_t * re_lock;

	if (op_id == CS_IPSEC_OP_DEC) {
		curr_mask = ipsec_main_db.re0_spidx_mask[sa_id];
		re_lock = &ipsec_main_db.re0_lock;
	} else if (op_id == CS_IPSEC_OP_ENC) {
		curr_mask = ipsec_main_db.re1_spidx_mask[sa_id];
		re_lock = &ipsec_main_db.re1_lock;
	} else
		return 0xff;	/* sp_idx value should be
				 * between 112 to 127 */
	spin_lock_bh(re_lock);
	do {
		if (0 == (curr_mask & (0x1 << offset))) {
			curr_mask |= (0x1 << offset);
			if (op_id == CS_IPSEC_OP_DEC)
				ipsec_main_db.re0_spidx_mask[sa_id] = curr_mask;
			else
				ipsec_main_db.re1_spidx_mask[sa_id] = curr_mask;
			spin_unlock_bh(re_lock);
			return (CS_IPSEC_SP_IDX_START + offset);
		}
		offset++;
	} while (offset < CS_IPSEC_SP_IDX_NUM);
	spin_unlock_bh(re_lock);

	ipsec_skb_header_flush_expire_cb_queue(op_id, sa_id);

	return 0xff;
}				/* ipsec_spidx_get */


static bool ipsec_spidx_if_used(unsigned char op_id,
	unsigned short sa_id, unsigned char sp_idx)
{
	unsigned short curr_mask;
	unsigned char offset = sp_idx - CS_IPSEC_SP_IDX_START;

	if (op_id == CS_IPSEC_OP_DEC)
		curr_mask = ipsec_main_db.re0_spidx_mask[sa_id];
	else if (op_id == CS_IPSEC_OP_ENC)
		curr_mask = ipsec_main_db.re1_spidx_mask[sa_id];
	else
		return 0xff;

	return (0 != (curr_mask & (0x1 << offset)));

}				/* ipsec_spidx_if_used */


static void ipsec_spidx_free(unsigned char op_id,
	unsigned short sa_id, unsigned char spidx)
{
	if ((spidx < CS_IPSEC_SP_IDX_START) ||
	    (spidx >= (CS_IPSEC_SP_IDX_START + CS_IPSEC_SP_IDX_NUM)))
		return;
	if (op_id == CS_IPSEC_OP_DEC) {
		spin_lock_bh(&ipsec_main_db.re0_lock);
		ipsec_main_db.re0_spidx_mask[sa_id] &=
		    ~(0x1 << (spidx - CS_IPSEC_SP_IDX_START));
		spin_unlock_bh(&ipsec_main_db.re0_lock);
	} else if (op_id == CS_IPSEC_OP_ENC) {
		spin_lock_bh(&ipsec_main_db.re1_lock);
		ipsec_main_db.re1_spidx_mask[sa_id] &=
		    ~(0x1 << (spidx - CS_IPSEC_SP_IDX_START));
		spin_unlock_bh(&ipsec_main_db.re1_lock);
	}
	return;
}				/* ipsec_spidx_free */

/* CS IPsec SADB handler */
cs_ipsec_sadb_t *ipsec_sadb_get(unsigned char table_id, unsigned short sa_id)
{

	if (table_id == 0)
		return &ipsec_main_db.re0_sadb_q[sa_id];
	if (table_id == 1)
		return &ipsec_main_db.re1_sadb_q[sa_id];
	return NULL;
}				/* ipsec_sadb_get */

/* Recovery mechanism */
static int ipsec_enqueue_skb_to_sadb(unsigned char table_id,
				     unsigned short sa_id, struct sk_buff *skb)
{
	cs_ipsec_skb_queue_t *p_skb_queue, *p_curr_skb_queue;
	unsigned char pkt_cnt = 0;
	cs_kernel_accel_cb_t *cs_cb = CS_KERNEL_SKB_CB(skb);
	spinlock_t * re_lock;

	if ((0 != table_id) && (1 != table_id))
		return -1;
	if (sa_id >= CS_IPSEC_TUN_NUM)
		return -1;
	if (NULL == skb)
		return -1;
	DBG(printk("%s:%d table_id=%d, sa_id=%d\n", __func__, __LINE__, table_id, sa_id));

	p_skb_queue = kmalloc(sizeof(cs_ipsec_skb_queue_t), GFP_ATOMIC);
	if (NULL == p_skb_queue)
		return -1;
	cs_cb->common.tag = 0x0000;
	p_skb_queue->skb = skb;
	p_skb_queue->next = NULL;

	p_curr_skb_queue = NULL;
	if (0 == table_id) {
		re_lock = &ipsec_main_db.re0_lock;
	} else {
		re_lock = &ipsec_main_db.re1_lock;
	}

	spin_lock_bh(re_lock);

	if (0 == table_id) {
		if (NULL == ipsec_main_db.re0_skb_q[sa_id])
			ipsec_main_db.re0_skb_q[sa_id] = p_skb_queue;
		else
			p_curr_skb_queue = ipsec_main_db.re0_skb_q[sa_id];
	} else {		/* if (1 == table_id) */
		if (NULL == ipsec_main_db.re1_skb_q[sa_id])
			ipsec_main_db.re1_skb_q[sa_id] = p_skb_queue;
		else
			p_curr_skb_queue = ipsec_main_db.re1_skb_q[sa_id];
	}
	/* the original list is not empty, so we have to enqueue this skb to
	 * the end of the list */

	if (NULL != p_curr_skb_queue) {
		while (NULL != p_curr_skb_queue->next) {
			p_curr_skb_queue = p_curr_skb_queue->next;
			pkt_cnt++;
		};
		// FIXME! debug message..
		DBG(printk("%s:%d:current queue count = %d\n", __func__, __LINE__,
		     pkt_cnt));
		if (pkt_cnt >= CS_IPSEC_CB_QUEUE_MAX) {
			/* we are queuing too many packets.. going to drop this one..
			 * in case of infinite loop */
			kfree(p_skb_queue);
			kfree_skb(skb);
		} else
			p_curr_skb_queue->next = p_skb_queue;
	} else {
		DBG(printk("%s:%d:current queue count = 1\n", __func__, __LINE__));
	}
	spin_unlock_bh(re_lock);

	return 0;
}				/* ipsec_enqueue_skb_to_sadb */

static struct sk_buff *ipsec_dequeue_skb_from_sadb(unsigned char table_id,
						   unsigned short sa_id)
{
	cs_ipsec_skb_queue_t *p_skb_queue;
	struct sk_buff *rslt_skb = NULL;
	spinlock_t * re_lock;

	if ((0 != table_id) && (1 != table_id))
		return NULL;
	if (sa_id >= CS_IPSEC_TUN_NUM)
		return NULL;

	DBG(printk("%s:%d table_id=%d, sa_id=%d\n", __func__, __LINE__, table_id, sa_id));

	if (0 == table_id) {
		re_lock = &ipsec_main_db.re0_lock;
	} else {
		re_lock = &ipsec_main_db.re1_lock;
	}

	spin_lock_bh(re_lock);

	if ((0 == table_id) && (NULL != ipsec_main_db.re0_skb_q[sa_id])) {
		/* if there are skb queued in the list of table#0+sa_id, dequeue it */
		p_skb_queue = ipsec_main_db.re0_skb_q[sa_id];
		ipsec_main_db.re0_skb_q[sa_id] = p_skb_queue->next;
		rslt_skb = p_skb_queue->skb;
		kfree(p_skb_queue);
	} else if (NULL != ipsec_main_db.re1_skb_q[sa_id]) {
		/* if there are skb queued in the list of table#1+sa_id, dequeue it */
		p_skb_queue = ipsec_main_db.re1_skb_q[sa_id];
		ipsec_main_db.re1_skb_q[sa_id] = p_skb_queue->next;
		rslt_skb = p_skb_queue->skb;
		kfree(p_skb_queue);
	}
	spin_unlock_bh(re_lock);

	return rslt_skb;
}				/* ipsec_dequeue_skb_from_sadb */


static int ipsec_sadb_find_by_xfrm_state(struct xfrm_state *p_x_state,
					unsigned char *p_table,
					unsigned short *p_idx)
{
	unsigned short iii;

	if ((NULL == p_x_state) || (NULL == p_table) || (NULL == p_idx))
		return -1;

	for (iii = 0; iii < CS_IPSEC_TUN_NUM; iii++) {
		if ((p_x_state == ipsec_main_db.re0_sadb_q[iii].x_state) &&
			(ipsec_main_db.re0_sadb_q[iii].used == 1)){
			*p_table = 0;
			*p_idx = iii;
			return 0;
		}

		if ((p_x_state == ipsec_main_db.re1_sadb_q[iii].x_state)&&
			(ipsec_main_db.re1_sadb_q[iii].used == 1)) {
			*p_table = 1;
			*p_idx = iii;
			return 0;
		}
	}
	return CS_ERR_ENTRY_NOT_FOUND;
}				/* ipsec_sadb_find_by_xfrm_state */

static int ipsec_sadb_get_avail_sa_id(unsigned char table_id,
				 unsigned short *p_sa_id)
{
	unsigned short start_idx = 0, curr_idx;
	cs_ipsec_sadb_t *p_sadb_table = NULL, *p_curr_sadb = NULL;
	spinlock_t * re_lock;

	if (0 == table_id) {
		start_idx = ipsec_main_db.p_re0_idx;
		p_sadb_table = &ipsec_main_db.re0_sadb_q[0];
		re_lock = &ipsec_main_db.re0_lock;
	} else if (1 == table_id) {
		start_idx = ipsec_main_db.p_re1_idx;
		p_sadb_table = &ipsec_main_db.re1_sadb_q[0];
		re_lock = &ipsec_main_db.re1_lock;
	}
	spin_lock_bh(re_lock);

	curr_idx = start_idx;
	p_curr_sadb = p_sadb_table + curr_idx;
	do {
		if (CS_IPSEC_TUN_NUM == curr_idx) {
			curr_idx = 0;
			p_curr_sadb = p_sadb_table;
		}

		if (0 == p_curr_sadb->used) {
			/* found an available sadb to use */
			*p_sa_id = curr_idx;
			DBG(printk("%s got sa %d\n", __func__, curr_idx));
			p_curr_sadb->used = 1;
			if (0 == table_id)
				ipsec_main_db.p_re0_idx = curr_idx + 1;
			else
				ipsec_main_db.p_re1_idx = curr_idx + 1;

			spin_unlock_bh(re_lock);
			return 0;
		}

		curr_idx++;
		p_curr_sadb++;
	} while (curr_idx != start_idx);

	spin_unlock_bh(re_lock);
	DBG(printk("WARNING: %s failure to get sa id !!!\n", __func__));

	return -1;
}				/* ipsec_sadb_get_avail_sa_id */

static char* ipsec_sadb_state_to_str(int state)
{
	switch (state)
	{
	case SADB_INIT:
		return "SADB_INIT";
	case SADB_PE_STOPPED:
		return "SADB_PE_STOPPED";
	case SADB_ACCELERATED:
		return "SADB_ACCELERATED";
	case SADB_WAIT_PE_STOP:
		return "SADB_WAIT_PE_STOP";
	default:
		return "IMPOSSIBLE";
	}
	return "IMPOSSIBLE";
}

static int ipsec_sadb_free(unsigned char table_id, unsigned short sa_id)
{
	cs_ipsec_sadb_t *p_sadb;

	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (NULL == p_sadb)
		return -1;

	p_sadb->used = 0;
	p_sadb->spi = 0;
	p_sadb->state = SADB_INIT;
	dma_map_single(NULL, (void *)p_sadb,
				sizeof(cs_ipsec_sadb_t), DMA_TO_DEVICE);

	DBG(printk("%s for table_id %d sa_id %d used=%d state=%s \n", __func__,
	    table_id, sa_id, p_sadb->used,ipsec_sadb_state_to_str(p_sadb->state)));

	return 0;
}				/* ipsec_sadb_free */


static int ipsec_sadb_end(unsigned char table_id, unsigned short sa_id)
{
	cs_ipsec_sadb_t *p_sadb;
	struct sk_buff *skb;
	int x_valid;
	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (NULL == p_sadb)
		return -1;

	dma_map_single(NULL, (void *)p_sadb,
			sizeof(cs_ipsec_sadb_t), DMA_FROM_DEVICE);

	DBG(printk("%s:%d table_id=%d, sa_id=%d, state=%s seq_num=%d \n", __func__, __LINE__,
		table_id, sa_id, ipsec_sadb_state_to_str(p_sadb->state), p_sadb->seq_num));

	/* Update the real xfrm information based on the current number
	 * stored in DDR */
	if (p_sadb->used == 1) {
		x_valid = kern_ptr_validate(p_sadb->x_state, sizeof(struct xfrm_state));
		if (x_valid == 1) {
			if (p_sadb->x_state->km.state != XFRM_STATE_VALID) {
				x_valid = 0;
			}
		}
		if (SADB_WAIT_PE_STOP == p_sadb->state) {
			if (x_valid == 1) {
				p_sadb->x_state->replay.oseq = p_sadb->seq_num - 1;
				p_sadb->x_state->curlft.bytes = p_sadb->bytes_count;
				p_sadb->x_state->curlft.packets = p_sadb->packets_count;
			}
		}
		p_sadb->state = SADB_PE_STOPPED;
		dma_map_single(NULL, (void *)p_sadb,
			sizeof(cs_ipsec_sadb_t), DMA_TO_DEVICE);
	} else {
		x_valid = 0;
	}

	/*FIXME: x_state could be already expired when we get ipc_stop callback*/
	while ((skb = ipsec_dequeue_skb_from_sadb(table_id, sa_id)) != NULL) {
		/* resume the packet to its proper handler in Kernel */
		if (x_valid == 1) {
			if (CS_IPSEC_INBOUND == p_sadb->sa_dir) {

				DBG(printk("%s:%d xfrm_input_resume table_id=%d, sa_id=%d skb=%p\n",
					__func__, __LINE__, table_id, sa_id, skb));
				xfrm_input_resume(skb, p_sadb->x_state->id.proto);
			} else {

				DBG(printk("%s:%d xfrm_output_resume table_id=%d, sa_id=%d skb=%p\n",
					__func__, __LINE__, table_id, sa_id, skb));
				xfrm_output_resume(skb, 1);
			}
		} else {
			kfree_skb(skb);
		}
	}

	ipsec_sadb_free(table_id, sa_id);
	return 0;
}

static int ipsec_check_xfrm_accelerate(struct xfrm_state *p_x_state)
{
	if ((XFRM_MODE_TUNNEL != p_x_state->props.mode) &&
	    (XFRM_MODE_TRANSPORT != p_x_state->props.mode))
		return -1;

	/* don't support other encapsulation for now.. such as ESPINUDP.
	 * ESPINUDP_NON_IKE, and for AH mode, there is no encap */
	if (p_x_state->encap)
		return -1;

	if (p_x_state->km.state != XFRM_STATE_VALID) {
		DBG(printk("%s:%d set sw_only because x->km.state != XFRM_STATE_VALID\n",
			__func__, __LINE__));
		return -1;
	}

	if (IPPROTO_ESP == p_x_state->id.proto) {
		if (p_x_state->aead) {
			if ((0 >
			     cs_ipsec_get_cipher_alg(p_x_state->aead->alg_name))
			    || (0 >
				cs_ipsec_get_cipher_mode(p_x_state->props.
							 ealgo)))
				return -1;
		} else {
			if ((0 >
			     cs_ipsec_get_cipher_alg(p_x_state->ealg->alg_name))
			    || (0 >
				cs_ipsec_get_cipher_mode(p_x_state->props.
							 ealgo)))
				return -1;
			if (p_x_state->aalg) {
				if (0 >
				    cs_ipsec_get_auth_alg(p_x_state->aalg->
							  alg_name))
					return -1;
				if (NULL ==
				    xfrm_aalg_get_byname(p_x_state->aalg->
							 alg_name, 0))
					return -1;
			}
		}
	} else if (IPPROTO_AH == p_x_state->id.proto) {
		if (0 > cs_ipsec_get_auth_alg(p_x_state->aalg->alg_name))
			return -1;
	} else {
		return -1;
	}

	return 0;
}				/* ipsec_check_xfrm_accelerate */

int ipsec_sadb_update(unsigned char table_id, unsigned short sa_id,
		      unsigned char dir, struct xfrm_state *p_x_state,
		      unsigned char ip_ver)
{
	cs_ipsec_sadb_t *p_sadb;
	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (NULL == p_sadb)
		return -1;
	/* general info */
	p_sadb->spi = p_x_state->id.spi;
	/* IPPROTO_ESP = 50 => p_sadb->proto = 0
	 * IPPOROT_AH = 51  => p_sadb->proto = 1 */
	p_sadb->proto = p_x_state->id.proto - IPPROTO_ESP;
	p_sadb->seq_num = p_x_state->replay.oseq + 1;
	p_sadb->replay_window = p_x_state->props.replay_window;
	/* advance the sequence number by 1, because this current packet has yet
	 * updated the real sequence number */
	p_sadb->ip_ver = ip_ver;
	p_sadb->sa_dir = dir;
	p_sadb->x_state = p_x_state;
	p_sadb->lifetime_bytes = MIN(p_x_state->lft.hard_byte_limit,
				     p_x_state->lft.soft_byte_limit);
	p_sadb->bytes_count = p_x_state->curlft.bytes;
	p_sadb->lifetime_packets = MIN(p_x_state->lft.hard_packet_limit,
				       p_x_state->lft.soft_packet_limit);
	p_sadb->packets_count = p_x_state->curlft.packets;	/* add_time and use_time? */

	// FIXME! Debug message
	DBG(printk("%s:%d:spi = 0x%x, proto = %d, seq_num = %d, ", __func__,
		   __LINE__, p_sadb->spi, p_sadb->proto, p_sadb->seq_num));
	DBG(printk("ip_ver = %d, sa_dir = %d\n", p_sadb->ip_ver, p_sadb->sa_dir));
	DBG(printk("\t\tlifetime bytes = %d, packet = %d\n", p_sadb->lifetime_bytes,
	     p_sadb->lifetime_packets));
	if (XFRM_MODE_TUNNEL == p_x_state->props.mode)
		p_sadb->mode = 0;	/* 0 for tunnel */
	else if (XFRM_MODE_TRANSPORT == p_x_state->props.mode)
		p_sadb->mode = 1;
	else {
		DBG(printk("%s:%d: mode = %d\n", __func__, __LINE__,
			p_x_state->props.mode));
		return -1;
	}

	if (p_x_state->id.proto == IPPROTO_ESP) {
		struct esp_data *esp = (struct esp_data *)p_x_state->data;
		struct crypto_aead *aead = esp->aead;

		p_sadb->iv_len = crypto_aead_ivsize(aead) >> 2;

		if (p_x_state->aead) {
			p_sadb->ealg =
			    cs_ipsec_get_cipher_alg(p_x_state->aead->alg_name);
			p_sadb->ealg_mode =
			    cs_ipsec_get_cipher_mode(p_x_state->props.ealgo);
			if ((0 > p_sadb->ealg) || (0 > p_sadb->ealg_mode)) {
				DBG(printk("%s:%d: ealg = %d, ealg mode = %d\n",
						__func__, __LINE__,
						p_sadb->ealg,
						p_sadb->ealg_mode));
				return -1;
			}

			p_sadb->enc_keylen =
			    (p_x_state->aead->alg_key_len + 7) >> 5;
			memcpy(p_sadb->ekey, p_x_state->aead->alg_key,
			       (p_sadb->enc_keylen << 2));

			/* in AEAD mode, there is no authentication algorithm */
			p_sadb->aalg = CS_IPSEC_AUTH_NULL;
			memset(p_sadb->akey, 0x00, MAX_AUTH_KEY_LEN);
			p_sadb->auth_keylen = 0;

			/* but still need to define ICV length */
#if 0
			p_sadb->icv_trunclen =
			    p_x_state->aead->alg_icv_len >> 5;
#endif
			p_sadb->icv_trunclen = crypto_aead_authsize(aead);
			DBG(printk("%s:%d:p_x_state->aead->alg_icv_len %d"
			     " vs aead_authsize %d\n",
			     __func__, __LINE__,
			     p_x_state->aead->alg_icv_len >> 5,
			     crypto_aead_authsize(aead)));
		} else {
			struct xfrm_algo_desc *ealg_desc;

			p_sadb->ealg =
			    cs_ipsec_get_cipher_alg(p_x_state->ealg->alg_name);
			p_sadb->ealg_mode =
			    cs_ipsec_get_cipher_mode(p_x_state->props.ealgo);
			if ((0 > p_sadb->ealg) || (0 > p_sadb->ealg_mode)) {
				DBG(printk("%s:%d: ealg = %d, ealg mode = %d\n",
						__func__, __LINE__,
						p_sadb->ealg,
						p_sadb->ealg_mode));
				return -1;
			}

			p_sadb->enc_keylen =
			    (p_x_state->ealg->alg_key_len + 7) >> 5;
			memcpy(p_sadb->ekey, p_x_state->ealg->alg_key,
			       p_sadb->enc_keylen << 2);

			/* get IV length */
			ealg_desc =
			    xfrm_ealg_get_byname(p_x_state->ealg->alg_name, 0);
			if (p_x_state->aalg) {
				struct xfrm_algo_desc *aalg_desc;

				p_sadb->aalg =
				    cs_ipsec_get_auth_alg(p_x_state->aalg->
							  alg_name);
				if (0 > p_sadb->aalg) {
					DBG(printk("%s:%d: aalg = %d\n",
						__func__, __LINE__,
						p_sadb->aalg));
					return -1;
				}

				p_sadb->auth_keylen =
				    (p_x_state->aalg->alg_key_len + 7) >> 5;
				memcpy(p_sadb->akey, p_x_state->aalg->alg_key,
				       p_sadb->auth_keylen << 2);

				/* get ICV (integrity check value) truncated length */
				aalg_desc =
				    xfrm_aalg_get_byname(p_x_state->aalg->
							 alg_name, 0);
				if (NULL == aalg_desc) {
					printk("%s:%d: aalg_desc = %p\n",
						__func__, __LINE__, aalg_desc);
					return -1;
				}

				p_sadb->icv_trunclen =
				    aalg_desc->uinfo.auth.icv_truncbits >> 5;
			} else {
				p_sadb->aalg = CS_IPSEC_AUTH_NULL;
				memset(p_sadb->akey, 0x00, MAX_AUTH_KEY_LEN);
				p_sadb->auth_keylen = 0;
				p_sadb->icv_trunclen = 0;
			}
		}
		// FIXME! debug message
		DBG(printk("%s:%d:ealg %x, ealg_mode %x, enc_keylen %x, iv_len %x, ",
		     __func__, __LINE__, p_sadb->ealg, p_sadb->ealg_mode,
		     p_sadb->enc_keylen, p_sadb->iv_len));
		DBG(printk("aalg %x, auth_keylen %x, icv_trunclen %x\n", p_sadb->aalg,
		     p_sadb->auth_keylen, p_sadb->icv_trunclen));
	} else if (IPPROTO_AH == p_x_state->id.proto) {
		struct ah_data *ahp = (struct ah_data *)p_x_state->data;

		/* set all the unused field to 0 */
		p_sadb->enc_keylen = 0;
		p_sadb->ealg = 0;
		p_sadb->ealg_mode = 0;
		memset(p_sadb->ekey, 0x00, MAX_ENC_KEY_LEN);

		/* set the real authentication-required value */
		p_sadb->aalg = cs_ipsec_get_auth_alg(p_x_state->aalg->alg_name);
		if (0 > p_sadb->aalg) {
			DBG(printk("%s:%d: aalg = %d\n", __func__, __LINE__,
				p_sadb->aalg));
			return -1;
		}

		p_sadb->auth_keylen = (p_x_state->aalg->alg_key_len + 7) >> 5;
		memcpy(p_sadb->akey, p_x_state->aalg->alg_key,
		       p_sadb->auth_keylen << 2);
		p_sadb->icv_trunclen = ahp->icv_trunc_len >> 2;
	} else {
		DBG(printk("%s:%d: protocol = %d\n", __func__, __LINE__,
			p_x_state->id.proto));
		return -1;
	}

	if (CS_IPSEC_IPV4 == ip_ver) {
		p_sadb->tunnel_saddr.addr[0] = p_x_state->props.saddr.a4;
		p_sadb->tunnel_daddr.addr[0] = p_x_state->id.daddr.a4;
		p_sadb->eth_addr[12] = 0x8;
		p_sadb->eth_addr[13] = 0x00;
	} else if (CS_IPSEC_IPV6 == ip_ver) {
		ipv6_addr_copy((struct in6_addr *)&p_sadb->tunnel_saddr.addr,
			       (struct in6_addr *)&p_x_state->props.saddr.a6);
		ipv6_addr_copy((struct in6_addr *)&p_sadb->tunnel_daddr.addr,
			       (struct in6_addr *)&p_x_state->id.daddr.a6);
		p_sadb->eth_addr[12] = 0x86;
		p_sadb->eth_addr[13] = 0xDD;
	} else {
		DBG(printk("%s:%d: IP ver = %d\n", __func__, __LINE__, ip_ver));
		return -1;
	}

	DBG(printk("%s:%d:ip address sa 0x%x da 0x%x\n",
	     __func__, __LINE__, p_sadb->tunnel_saddr.addr[0],
				 p_sadb->tunnel_daddr.addr[0]));
	if (CS_IPSEC_IPV6 == ip_ver) {
		DBG(printk("%s:%d:ip address sa[1] 0x%x da[1] 0x%x\n",
		     __func__, __LINE__, p_sadb->tunnel_saddr.addr[1],
					 p_sadb->tunnel_daddr.addr[1]));
		DBG(printk("%s:%d:ip address sa[2] 0x%x da[2] 0x%x\n",
		     __func__, __LINE__, p_sadb->tunnel_saddr.addr[2],
					 p_sadb->tunnel_daddr.addr[2]));

		DBG(printk("%s:%d:ip address sa[3] 0x%x da[3] 0x%x\n",
		     __func__, __LINE__, p_sadb->tunnel_saddr.addr[3],
					 p_sadb->tunnel_daddr.addr[3]));
	}
	/* need to create a ip header and calculate a checksum for outbound case */
	if (CS_IPSEC_OUTBOUND == dir) {
		if (CS_IPSEC_IPV4 == ip_ver) {
			struct iphdr iph;

			iph.ihl = 5;
			iph.version = IPVERSION;
			iph.tos = 0;
			iph.tot_len = 0;
			iph.id = 0;
			iph.frag_off = 0;
			iph.ttl = 64;
			iph.protocol = p_x_state->id.proto;
			iph.saddr = p_sadb->tunnel_saddr.addr[0];
			iph.daddr = p_sadb->tunnel_daddr.addr[0];
			ip_send_check(&iph);
			p_sadb->checksum = iph.check;
		}
	}

	/* Flush the p_sadb so that RE gets correct data */
	dma_map_single(NULL, (void *)p_sadb,
		sizeof(cs_ipsec_sadb_t), DMA_TO_DEVICE);

	return 0;
}				/* ipsec_sadb_update */

static int ipsec_sadb_check_and_create(unsigned char dir,
				       struct xfrm_state *p_x_state,
				       unsigned char ip_ver,
				       unsigned char *p_table_id,
				       unsigned short *p_sa_id)
{
	int status;
	unsigned short sa_id;
	unsigned char table_id;
	if ((NULL == p_x_state) || (NULL == p_table_id) || (NULL == p_sa_id))
		return -1;

	table_id = IPSEC_TABLE_ID_BY_DIR(dir);

	if (0 != ipsec_check_xfrm_accelerate(p_x_state))
		return -1;

	status = ipsec_sadb_get_avail_sa_id(table_id, &sa_id);
	if (status != 0)
		return status;

	/* consume the sadb first, in case if there are multiple access, by doing so
	 * other processes won't allocate this resource */
	//ipsec_sadb_consume(table_id, sa_id);

	status = ipsec_sadb_update(table_id, sa_id, dir, p_x_state, ip_ver);
	if (status != 0)
		ipsec_sadb_free(table_id, sa_id);

	*p_table_id = table_id;
	*p_sa_id = sa_id;

	return status;
}				/* ipsec_sadb_check_and_create */

static int ipsec_sadb_stop(unsigned char table_id, unsigned short sa_id,
			   int delete_hash)
{
	cs_ipsec_sadb_t *p_sadb;
	cs_core_hmu_value_t hmu_value;

	DBG(printk(" %s stopping acceleration for table_id %d sa_id %d\n",
	     __func__, table_id, sa_id));


	/* 1) set it in Kernel sadb data structure that the acceleration applied
	 * on this SADB is going to stop. */
	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (NULL == p_sadb)
		return -1;

	if (p_sadb->state == SADB_ACCELERATED)
		p_sadb->state = SADB_WAIT_PE_STOP;

	dma_map_single(NULL, (void *)p_sadb,
		sizeof(cs_ipsec_sadb_t), DMA_TO_DEVICE);

	/* 2) disable all the hash entries that are related to the
	 * gu_id */
	if (delete_hash) {
		DBG(printk(" %s deleting hashes\n", __func__));
		memset(&hmu_value, 0, sizeof(hmu_value));

		hmu_value.type = CS_CORE_HMU_WATCH_SWID64;
		hmu_value.mask = 0x08;
		hmu_value.value.swid64 = CS_IPSEC_GID(table_id, sa_id);
		cs_core_hmu_delete_hash(CS_CORE_HMU_WATCH_SWID64, &hmu_value);
	}
	/* ignoring all the possible errors from deleting hash. even though
	 * they are somehow critical */

	/* 3) also need to check it's cb queue to delete the entry */
	ipsec_skb_header_flush_cb_queue(table_id, sa_id);

	if (p_sadb->used == 0) {
		ipsec_sadb_end(table_id, sa_id);
		return -1;
	}

#ifdef CS_IPC_ENABLED
	/* 4) send ipc message to RE to notify this tunnel is no longer
	 * accelerated and handled by RE */
	ipsec_ipc_send_stop(table_id, sa_id);
	/* ignoring all the possible erros here too. */
#endif

	DBG(printk(" %s stopped acceleration waiting for IPC confirmation to free sadb\n", __func__));
	/* 5) all the rest of packets will be enqueued, until the module receive
	 * a "stop complete" from IPsec Offload engine. */

	return 0;
}				/* ipsec_sadb_stop */

static void ipsec_sadb_stop_all(unsigned char table_id)
{
	unsigned short sa_id;
	cs_ipsec_sadb_t *p_sadb;

	for (sa_id = 0; sa_id < CS_IPSEC_TUN_NUM; sa_id++) {
		p_sadb = ipsec_sadb_get(table_id, sa_id);
		if (p_sadb->used == 1)
			ipsec_sadb_stop(table_id, sa_id, 1);
	}
	return;
}				/* ipsec_sadb_stop_all */

/* skb handling */
/* the following API is used for calculate the original skb size of
 * a skb that's already in ESP/AH process */
static unsigned short ipsec_calc_orig_skb_size(struct sk_buff *skb)
{
	unsigned short curr_size;

	curr_size = skb->len;

	/* Current skb->len does not include ethernet header and IP header */
	/*
	 * Complete IPsec ESP tunnel mode packet would include following fileds
	 * Ether header: 14 bytes
	 * Outer IP header: 20 bytes
	 * ESP header: 8 bytes
	 * IV: max 16 bytes
	 * padding: max 16 bytes
	 * ESP trailer: 2 bytes
	 * ICV: 12 bytes
	 */

	/* add in the IP header length */
	curr_size += ip_hdrlen(skb);

	/* add in ethernet header length */
	curr_size += ETH_HLEN;

	return curr_size;
}				/* ipsec_calc_orig_skb_size */

int ipsec_sadb_check_skb_queue_empty(unsigned char table_id, unsigned short sa_id)
{
	if ((0 != table_id) && (1 != table_id))
		return -1;
	if (sa_id >= CS_IPSEC_TUN_NUM)
		return -1;

	if ((0 == table_id) && (NULL != ipsec_main_db.re0_skb_q[sa_id]))
		return 0;
	return -1;
}				/* ipsec_peek_skb_queue_of_sadb */


static int ipsec_create_internal_hash(unsigned char table_id,
				      unsigned short sa_id, struct sk_buff *skb,
				      unsigned char sp_idx)
{
	cs_fwd_hash_t fwd_hash[1];
	unsigned int fwd_cnt = 1, i;
	fe_sw_hash_t *key_dir1 = &fwd_hash[0].key;
	cs_ipsec_sadb_t *p_sadb;
	int status;
	u64 fwd_hm_flag;
	unsigned char func_id = 0;
	cs_kernel_accel_cb_t *org_cs_cb = CS_KERNEL_SKB_CB(skb);
	cs_kernel_accel_cb_t new_cs_cb;
	cs_kernel_accel_cb_t *cs_cb;
	u16 fwd_hash_idx[2]={0,0};

	if (org_cs_cb == NULL)
		return -1;
	if (table_id == CS_IPSEC_OP_DEC)
		printk("%s why send packt to decryption engine?\n", __func__);

	cs_cb = &new_cs_cb;
	memcpy(cs_cb, org_cs_cb, sizeof(new_cs_cb));

	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (p_sadb == NULL)
		return -1;

	memset(&fwd_hash, 0, sizeof(cs_fwd_hash_t));

	cs_cb->common.module_mask = CS_MOD_MASK_IPSEC;

	/* The hash from RE back to CPU */
	fwd_hash[0].fwd_app_type =
	    get_app_type_from_module_type((CS_MOD_MASK_FROM_RE0 |
					   CS_MOD_MASK_FROM_RE1));

	status = cs_core_vtable_get_hashmask_flag_from_apptype(
	    					fwd_hash[0].fwd_app_type,
							      &fwd_hm_flag);
	if (status != 0)
		return status;

	status = cs_core_vtable_get_hashmask_index_from_apptype(
						fwd_hash[0].fwd_app_type,
						&fwd_hash[0].key.mask_ptr_0_7);

	if (status != 0)
		return status;

	for (i = 0; i < 6; i++) {
		/* check how to populate mac sa for inbound case */
		if (p_sadb->sa_dir == CS_IPSEC_OUTBOUND)
			key_dir1->mac_sa[i] = p_sadb->eth_addr[11 - i];
	}

	/* In the decrypt path the MAC SA would be same as sent so need to
	 * convert it back */
	if (p_sadb->sa_dir == CS_IPSEC_INBOUND)
		key_dir1->mac_sa[0] = sp_idx;
	/* L3 */
	if (p_sadb->ip_ver == CS_IPSEC_IPV4) {
		key_dir1->ip_version = 0;
		key_dir1->sa[0] = ntohl(p_sadb->tunnel_saddr.addr[0]);
		key_dir1->da[0] = ntohl(p_sadb->tunnel_daddr.addr[0]);
		key_dir1->ip_prot = p_sadb->proto + IPPROTO_ESP;
		key_dir1->ip_valid = 1;
		key_dir1->spi_vld = 1;
		key_dir1->spi_idx = ntohl(p_sadb->spi);
	} else {
		key_dir1->ip_version = 1;
		key_dir1->sa[0] = ntohl(p_sadb->tunnel_saddr.addr[3]);
		key_dir1->da[0] = ntohl(p_sadb->tunnel_daddr.addr[3]);
		key_dir1->sa[1] = ntohl(p_sadb->tunnel_saddr.addr[2]);
		key_dir1->da[1] = ntohl(p_sadb->tunnel_daddr.addr[2]);
		key_dir1->sa[2] = ntohl(p_sadb->tunnel_saddr.addr[1]);
		key_dir1->da[2] = ntohl(p_sadb->tunnel_daddr.addr[1]);
		key_dir1->sa[3] = ntohl(p_sadb->tunnel_saddr.addr[0]);
		key_dir1->da[3] = ntohl(p_sadb->tunnel_daddr.addr[0]);
		key_dir1->ip_prot = p_sadb->proto + IPPROTO_ESP;
		key_dir1->ip_valid = 1;
		key_dir1->spi_vld = 1;
		key_dir1->spi_idx = ntohl(p_sadb->spi);
	}	/* IPv6 */

	key_dir1->lspid = CS_IPSEC_OUT_LSPID_FROM_RE;
	/* printk("%s:%d: CS_IPSEC_OUT_LSPID_FROM_RE = %d\n",
			__func__, __LINE__, key_dir1->lspid); */

	key_dir1->recirc_idx = sp_idx;

	cs_cb->common.sw_action_id = CS_IPSEC_SW_ACTION_ID(func_id, sp_idx,
							   sa_id);

	set_fwd_hash_result(cs_cb, &fwd_hash[0]);

	fwd_hash[0].param.voq_id = CS_IPSEC_CPU_VOQ_ID_FOR_RE(table_id);
	/* printk("%s:%d: table_id = %d, CS_IPSEC_CPU_VOQ_ID_FOR_RE = %d\n",
		__func__, __LINE__, table_id, fwd_hash[0].param.voq_id); */

	/*
	 * for internal hash, we should remove all the watch and only watch by
	 * IPSec SPGID
	 */
 	for (i = 0; i < CS_SWID64_MOD_MAX; i++)
		cs_cb->common.swid[i] = CS_INVALID_SWID64;
	cs_cb->common.swid_cnt = 0;
	cs_core_logic_add_swid64(cs_cb, CS_IPSEC_SPGID(func_id, sa_id, sp_idx));

	for (i = 0; i < fwd_cnt; i++) {
		fwd_hash[i].lifetime = CS_IPSEC_SPIDX_TIMEOUT;
		status += cs_core_add_fwd_hash(&fwd_hash[i], &fwd_hash_idx[i]);
		if (status != 0)
			goto FAIL_TO_ADD_HASH;

		set_fwd_hash_result_index_to_cs_cb(cs_cb, &fwd_hash[i]);
		cs_core_hmu_link_src_and_hash(cs_cb, fwd_hash_idx[i]);
	}

	/* complete adding hash!! now calling back to module to notify
	 * hash creation status */
	cs_core_hmu_callback_for_hash_creation_pass(cs_cb);

	return status;
FAIL_TO_ADD_HASH:
	// FIXME!! implement here. need to delete all the created hash?
	cs_core_hmu_callback_for_hash_creation_fail(cs_cb);
	return -1;

}				/* ipsec_create_internal_hash */

static void ipsec_set_skb_cb_info(unsigned char table_id, unsigned short sa_id,
				  struct sk_buff *skb)
{

	cs_kernel_accel_cb_t *cs_cb = CS_KERNEL_SKB_CB(skb);

	/* NOTE! assuming ip_hdr loc is given before entering this function */

	if (cs_cb == NULL)
		return;

	cs_core_logic_add_swid64(cs_cb, CS_IPSEC_GID(table_id, sa_id));
	cs_cb->common.module_mask |= CS_MOD_MASK_IPSEC;
	DBG(printk(
			"%s:%d adding swid for table id %d and sa id %d\n",
			__func__,__LINE__,table_id,sa_id));
	return;
} /* ipsec_set_skb_cb_info */

static void ipsec_skb_header_free(struct sk_buff *skb) {
	nf_reset(skb);
	cs_accel_cb_del(skb);
	dst_release(skb_dst(skb));
#ifdef CONFIG_XFRM
	secpath_put(skb->sp);
#endif
}


static void ipsec_skb_header_copy(struct sk_buff *dst_skb,
				       struct sk_buff *old_skb)
{
	/* Could refer to __copy_skb_header() in skbuff.c*/

	__nf_copy(dst_skb, old_skb);
#ifdef CONFIG_XFRM
	dst_skb->sp = secpath_get(old_skb->sp);
#endif
	dst_skb->sk = old_skb->sk;
	memcpy(&dst_skb->tstamp, &old_skb->tstamp, sizeof(ktime_t));
	dst_skb->dev = old_skb->dev;

	if (skb_dst(old_skb) != NULL) {
		skb_dst_set(dst_skb, dst_clone(skb_dst(old_skb)));
	}

	cs_accel_cb_clone(dst_skb, old_skb);
	dst_skb->priority = old_skb->priority;
	dst_skb->sk = old_skb->sk;
	dst_skb->local_df = old_skb->local_df;
	dst_skb->cloned = old_skb->cloned;
	//dst_skb->ip_summed = old_skb->ip_summed;
	dst_skb->ip_summed = CHECKSUM_UNNECESSARY;
	dst_skb->nohdr = old_skb->nohdr;
	dst_skb->pkt_type = old_skb->pkt_type;
	dst_skb->fclone = old_skb->fclone;
	dst_skb->ipvs_property = old_skb->ipvs_property;
	dst_skb->peeked = old_skb->peeked;
	dst_skb->nf_trace = old_skb->nf_trace;
	dst_skb->protocol = old_skb->protocol;
	dst_skb->destructor = old_skb->destructor;


	dst_skb->skb_iif = old_skb->skb_iif;
#ifdef CONFIG_NET_SCHED
	dst_skb->tc_index = old_skb->tc_index;
#ifdef CONFIG_NET_CLS_ACT
	dst_skb->tc_verd = old_skb->tc_verd;
#endif
#endif
	dst_skb->queue_mapping = old_skb->queue_mapping;
#ifdef CONFIG_IPV6_NDISC_NODETYPE
	dst_skb->ndisc_nodetype = old_skb->ndisc_nodetype;
#endif
#ifdef CONFIG_NETWORK_SECMARK
	dst_skb->secmark = old_skb->secmark;
#endif
	dst_skb->mark = old_skb->mark;
	dst_skb->vlan_tci = old_skb->vlan_tci;
	dst_skb->users = old_skb->users;

}				/* ipsec_copy_useful_skb_info */

static int ipsec_skb_header_store_skb_cb_info(unsigned char table_id, unsigned short sa_id,
				   struct sk_buff *skb, unsigned char sp_idx)
{
	cs_ipsec_cb_queue_t *p_cb_queue, *p_curr_cb_queue = NULL;

	p_cb_queue = kzalloc(sizeof(cs_ipsec_cb_queue_t), GFP_ATOMIC);
	if (NULL == p_cb_queue)
		return -1;

	p_cb_queue->skb.cs_cb_loc = 0;
	ipsec_skb_header_copy(&p_cb_queue->skb, skb);

	p_cb_queue->idx = sp_idx;
	p_cb_queue->next = NULL;
	p_cb_queue->timeout = jiffies + CS_IPSEC_SPIDX_TIMEOUT;

	if (table_id == 0) {
		if (ipsec_main_db.re0_cb_q[sa_id] == NULL)
			ipsec_main_db.re0_cb_q[sa_id] = p_cb_queue;
		else
			p_curr_cb_queue = ipsec_main_db.re0_cb_q[sa_id];
	} else if (table_id == 1) {
		if (ipsec_main_db.re1_cb_q[sa_id] == NULL)
			ipsec_main_db.re1_cb_q[sa_id] = p_cb_queue;
		else
			p_curr_cb_queue = ipsec_main_db.re1_cb_q[sa_id];
	} else {
		cs_accel_cb_del(&p_cb_queue->skb);
		ipsec_skb_header_free(&p_cb_queue->skb);
		kfree(p_cb_queue);
		return -1;
	}
	if (p_curr_cb_queue != NULL) {
		while (p_curr_cb_queue->next != NULL) {
			p_curr_cb_queue = p_curr_cb_queue->next;
		};
		p_curr_cb_queue->next = p_cb_queue;
	}

	return 0;
}				/* ipsec_store_skb_cb_info */

static int ipsec_skb_header_flush_cb_queue(unsigned char table_id,
				unsigned short sa_id)
{
	cs_ipsec_cb_queue_t *p_cb_queue;

	if ((0 == table_id) && (NULL != ipsec_main_db.re0_cb_q[sa_id])) {
		do {
			p_cb_queue = ipsec_main_db.re0_cb_q[sa_id];
			ipsec_main_db.re0_cb_q[sa_id] = p_cb_queue->next;

			ipsec_spidx_free(table_id, sa_id, p_cb_queue->idx);
			ipsec_skb_header_free(&p_cb_queue->skb);
			kfree(p_cb_queue);
		} while (NULL != ipsec_main_db.re0_cb_q[sa_id]);
		ipsec_main_db.re1_spidx_mask[sa_id] = 0;
	} else if ((1 == table_id) && (NULL != ipsec_main_db.re1_cb_q[sa_id])) {
		do {
			p_cb_queue = ipsec_main_db.re1_cb_q[sa_id];
			ipsec_main_db.re1_cb_q[sa_id] = p_cb_queue->next;

			ipsec_spidx_free(table_id, sa_id, p_cb_queue->idx);
			ipsec_skb_header_free(&p_cb_queue->skb);
			kfree(p_cb_queue);
		} while (NULL != ipsec_main_db.re1_cb_q[sa_id]);
		ipsec_main_db.re1_spidx_mask[sa_id] = 0;
	}
	return 0;
}				/* ipsec_flush_cb_queue */

static int ipsec_skb_header_flush_expire_cb_queue(unsigned char table_id,
				unsigned short sa_id)
{
	cs_ipsec_cb_queue_t *p_cb_queue, *p_prev_cb_queue = NULL;
	cs_ipsec_cb_queue_t *p_cb_queue_temp;
	unsigned char spidx;

	if (table_id == 0)
		p_cb_queue = ipsec_main_db.re0_cb_q[sa_id];
	else if (table_id == 1)
		p_cb_queue = ipsec_main_db.re1_cb_q[sa_id];
	else
		return -1;

	if (p_cb_queue == NULL) {
		DBG(printk("%s:%d:can't find pre-existing CB queue\n", __func__,
			   __LINE__));
		return -1;
	}

	do {
		if (p_cb_queue->timeout < jiffies) {
			/* remove this item off the list */
			if (p_prev_cb_queue != NULL) {
				p_prev_cb_queue->next = p_cb_queue->next;
			} else {
				if (table_id == 0)
					ipsec_main_db.re0_cb_q[sa_id] =
					    p_cb_queue->next;
				else	/* if (1 == table_id) */
					ipsec_main_db.re1_cb_q[sa_id] =
					    p_cb_queue->next;
			}
			p_cb_queue_temp = p_cb_queue;
			p_cb_queue = p_cb_queue->next;

			/* now delete p_cb_queue */
			spidx = p_cb_queue_temp->idx;
			ipsec_skb_header_free(&p_cb_queue_temp->skb);
			kfree(p_cb_queue_temp);
			ipsec_spidx_free(table_id, sa_id, spidx);

		}else {
			p_prev_cb_queue = p_cb_queue;
			p_cb_queue = p_cb_queue->next;
		}

	} while (p_cb_queue != NULL);
	return 0;
}				/* ipsec_flush_cb_queue */


static int ipsec_skb_header_recover_skb_cb_info(unsigned char table_id,
				     unsigned short sa_id, unsigned char sp_idx,
				     struct sk_buff *skb)
{
	cs_ipsec_cb_queue_t *p_cb_queue, *p_prev_cb_queue = NULL;

	if (table_id == 0)
		p_cb_queue = ipsec_main_db.re0_cb_q[sa_id];
	else if (table_id == 1)
		p_cb_queue = ipsec_main_db.re1_cb_q[sa_id];
	else
		return -1;

	if (p_cb_queue == NULL) {
		DBG(printk("%s:%d:can't find pre-existing CB queue\n", __func__,
			   __LINE__));
		return -1;
	}

	do {
		if (p_cb_queue->idx == sp_idx) {
			/* remove this item off the list */
			if (p_prev_cb_queue != NULL) {
				p_prev_cb_queue->next = p_cb_queue->next;
			} else {
				if (table_id == 0)
					ipsec_main_db.re0_cb_q[sa_id] =
					    p_cb_queue->next;
				else	/* if (1 == table_id) */
					ipsec_main_db.re1_cb_q[sa_id] =
					    p_cb_queue->next;
			}

			/* now copy the skb info and then delete p_cb_queue */
			if (skb != NULL)
				ipsec_skb_header_copy(skb, &p_cb_queue->skb);
			ipsec_skb_header_free(&p_cb_queue->skb);
			kfree(p_cb_queue);
			ipsec_spidx_free(table_id, sa_id, sp_idx);
			return 0;
		}
		p_prev_cb_queue = p_cb_queue;
		p_cb_queue = p_cb_queue->next;
	} while (p_cb_queue != NULL);

	/* found nothing */
	return -1;
}				/* ipsec_replace_skb_cb_info */


static int ipsec_send_to_re_skb_xmit(unsigned char table_id,
					unsigned short sa_id,
					struct sk_buff *skb,
					unsigned char sp_idx)
{
	struct ethhdr *p_eth;
	struct iphdr *p_ip = (struct iphdr *)skb->data;
	/* unsigned char i; */
	HEADER_A_T ni_header_a;

	/* current skb is an IP packet. without proper L2 info. to be able to
	 * send it to IPsec Offload Engine, we need to construct a L2 ETH header. */
	skb_push(skb, ETH_HLEN);
	p_eth = (struct ethhdr *)skb->data;
	if (p_ip->version == 4)
		p_eth->h_proto = __constant_htons(ETH_P_IP);
	else if (p_ip->version == 6)
		p_eth->h_proto = __constant_htons(ETH_P_IPV6);

	memset(p_eth->h_source, 0x0, ETH_ALEN);
	/* because of the network byte swap.. write the sp_idx to SA MAC[5] */
	p_eth->h_source[5] = sp_idx;
	p_eth->h_source[4] = table_id;
	p_eth->h_source[3] = sa_id & 0xFF;
	p_eth->h_source[2] = (sa_id & 0x100) >> 1;
	p_eth->h_source[1] = 0;

#if 0
	if (sp_idx < CS_IPSEC_TUN_NUM) {
		unsigned int crc32;
		unsigned char *ptr = (unsigned char *)((u32) skb->data - 8);
		crc32 = ~(calc_crc(~0, (u8 const *)&ptr[1], 5));
		p_eth->h_source[0] = crc32 & 0xFF;
	}
	if (skb->dev != NULL) {
		for (i = 0; i < ETH_ALEN; i++)
			p_eth->h_dest[i] = skb->dev->dev_addr[i];
	}

	if (skb->dev != NULL) {
		for (i = 0; i < ETH_ALEN; i++)
			p_eth->h_dest[i] = skb->dev->dev_addr[i];
	}

	/* Create and populate header A */

	memset(&ni_header_a, 0, sizeof(HEADER_A_T));
	ni_header_a.fwd_type = CS_FWD_NORMAL;
	ni_header_a.pspid = 3;	/* CPU */
	ni_header_a.dvoq = CS_IPSEC_RE_VOQ_ID(table_id);
	ni_header_a.flags = 0x800;
#endif

	/* send the packet. by using ni_special_start_xmit */
#ifndef CS_IPSEC_OUTBOUND_BYPASS_LINUX
	ni_special_start_xmit(skb, &ni_header_a);
	DBG(printk("%s:%d:sent the packet out via ni_special_start_xmit\n",
		   __func__, __LINE__));

#else
	ni_start_xmit_to_re(skb, ipsec_main_db.re1_dest_voq[sa_id]);
	DBG(printk("%s:%d:sent the packet out via ni_start_xmit_to_re\n",
		   __func__, __LINE__));

#endif


	return 0;
}				/* ipsec_send_to_re_skb_convert */

static void ipsec_skb_header_revert_ipip_on_skb(struct sk_buff *skb)
{
	struct iphdr *p_ip = ip_hdr(skb);
	struct ipv6hdr *p_ip6 = ipv6_hdr(skb);
	/* at this point ip_hdr will point to out_ip, and we only deal
	 * with IPv4 */

	/* if outer IP header is not IPIP, then we don't have to worry about
	 * reverting it back to an IP packet */
	if (p_ip->version == 4) {
		if (p_ip->protocol != IPPROTO_IPIP)
			return;
	} else {
		if(p_ip6->nexthdr != NEXTHDR_IPV6)
			return;
	}

	/* however skb->data still points to the inner IP, we will just have to
	 * reset the network header to the inner IP */
	skb_reset_network_header(skb);

	return;
}				/* ipsec_revert_ipip_on_skb */

static void ipsec_send_to_re_setup_cs_cb(unsigned char table_id,
				      unsigned short sa_id, struct sk_buff *skb)
{
	cs_kernel_accel_cb_t *cs_cb = CS_KERNEL_SKB_CB(skb);
	cs_ipsec_sadb_t *p_sadb;
	p_sadb = ipsec_sadb_get(table_id, sa_id);

	if	(cs_cb->common.sw_only == CS_SWONLY_HW_ACCEL) {
		ipsec_set_skb_cb_info(table_id, sa_id, skb);
		cs_cb->output.l4_h.ah_esp.spi = p_sadb->spi;
		cs_cb->output.l3_nh.iph.protocol = p_sadb->proto + IPPROTO_ESP;
	}
}

void ipsec_sadb_update_dest_dev(unsigned char table_id,
	unsigned short sa_id, struct net_device * dev)
{
	ipsec_main_db.re1_dest_voq[sa_id] = dev;
}

static void ipsec_send_to_re_linux_skb(unsigned char table_id,
				      unsigned short sa_id, struct sk_buff *skb)
{
	int status;
	unsigned char sp_idx;

	/* if this skb is hijacked from kernel, the new ip header has been
	 * inserted, so we will have to remove it! */

	if (table_id == CS_IPSEC_OP_ENC)
		ipsec_skb_header_revert_ipip_on_skb(skb);

	if (skb->head == NULL) {
		printk("%s:%d skb->head == NULL ??\n", __func__, __LINE__);
	}

#ifndef CS_IPSEC_OUTBOUND_BYPASS_LINUX
	sp_idx = ipsec_spidx_get(table_id, sa_id);
	if (sp_idx == 0xff)
		goto get_sp_idx_fail;

	/* 1) create 2 hash entries for this specific flow */
	status = ipsec_create_internal_hash(table_id, sa_id, skb, sp_idx);
	if (status != 0)
		goto create_internal_hash_fail;

	/* 2) set up and store the cb/skb info */
	ipsec_set_skb_cb_info(table_id, sa_id, skb);
	status = ipsec_skb_header_store_skb_cb_info(table_id, sa_id, skb, sp_idx);
	if (status != 0)
		goto store_skb_cb_info_fail;
#else
	ipsec_send_to_re_setup_cs_cb(table_id, sa_id, skb);
	sp_idx = 0;
#endif
	/* 3) convert the packet to an ethernet packet and send it to RE */
	status = ipsec_send_to_re_skb_xmit(table_id, sa_id, skb, sp_idx);
	if (status != 0)
		goto store_skb_cb_info_fail;

	return;

store_skb_cb_info_fail:
	/* remove the hash that's created */
create_internal_hash_fail:
	ipsec_spidx_free(table_id, sa_id, sp_idx);
get_sp_idx_fail:
	kfree_skb(skb);

	DBG(printk("%s:%d don't have enough spidx\n", __func__, __LINE__));

	return;
}				/* ipsec_send_to_re_skb_prepare */

void ipsec_dequeue_skb_and_send_to_re(unsigned char table_id,
				      unsigned short sa_id)
{
	cs_boolean f_stop = FALSE;
	unsigned short new_size;
	struct sk_buff *skb;

	while ((skb = ipsec_dequeue_skb_from_sadb(table_id, sa_id)) != NULL) {
		/* Estimate the packet size from current size, make sure it does not go
		 * over the accepted packet size in IPsec Offload engine. */
		new_size = ipsec_calc_orig_skb_size(skb);
		if ((FALSE == f_stop) && (new_size <= CS_IPSEC_RE_SKB_SIZE_MAX)) {
			ipsec_send_to_re_linux_skb(table_id, sa_id, skb);
		} else {
			/* stop the acceleration of this sadb */
			ipsec_sadb_stop(table_id, sa_id, 1);
			f_stop = TRUE;
		}
	};
}				/* ipsec_dequeue_skb_and_send_to_re */

static int ipsec_sadb_find_by_mac_selector(struct sk_buff *skb,
					   unsigned char *p_table_id,
					   unsigned short *p_sa_id)
{
	unsigned int crc32;
	unsigned char *ptr = (unsigned char *)((u32) skb->data - 8);

	crc32 = ~(calc_crc(~0, (u8 const *)&ptr[1], 5));	/* Calculate CRC over the MAC SA */
	if ((crc32 & 0xFF) != ptr[0])
		return -1;	/* Not a valid scenario */
	*p_table_id = ptr[4] & 0x1;
	*p_sa_id = ptr[3];
	*p_sa_id |= ((unsigned short)(ptr[4] & 0x80)) << 1;

	return 0;
}				/* ipsec_sadb_find_by_mac_selector */

#ifdef CS_IPC_ENABLED
/* IPC related */
static int ipsec_ipc_send_message(unsigned char op_id, unsigned short sa_id,
				  unsigned char msg_type, unsigned int data0,
				  unsigned int data1)
{
	int rc;
	cs_ipsec_ipc_msg_t msg;

	msg.op_id = op_id;
	msg.sa_id = sa_id;
	msg.data[0] = data0;
	msg.data[1] = data1;

	switch (cs_hw_ipsec_offload_mode) {
	case IPSEC_OFFLOAD_MODE_BOTH:
		if (CS_IPSEC_OP_DEC == op_id) {
			rc = g2_ipc_send(ipc_ctxt0, CS_IPSEC_IPC_RE0_CPU_ID,
					 CS_IPSEC_IPC_RE0_CLNT_ID, G2_IPC_HPRIO,
					 msg_type, &msg,
					 sizeof(cs_ipsec_ipc_msg_t));
			if (G2_IPC_OK != rc)
				return -1;
			else
				return 0;
		}
		if (CS_IPSEC_OP_ENC == op_id) {
			rc = g2_ipc_send(ipc_ctxt1, CS_IPSEC_IPC_RE1_CPU_ID,
					 CS_IPSEC_IPC_RE1_CLNT_ID, G2_IPC_HPRIO,
					 msg_type, &msg,
					 sizeof(cs_ipsec_ipc_msg_t));
			if (G2_IPC_OK != rc)
				return -1;
			else
				return 0;
		}
		break;
	case IPSEC_OFFLOAD_MODE_PE0:
		rc = g2_ipc_send(ipc_ctxt0, CS_IPSEC_IPC_RE0_CPU_ID,
				 CS_IPSEC_IPC_RE0_CLNT_ID, G2_IPC_HPRIO,
				 msg_type, &msg, sizeof(cs_ipsec_ipc_msg_t));
		if (G2_IPC_OK != rc)
			return -1;
		else
			return 0;
		break;
	case IPSEC_OFFLOAD_MODE_PE1:
		rc = g2_ipc_send(ipc_ctxt1, CS_IPSEC_IPC_RE1_CPU_ID,
				 CS_IPSEC_IPC_RE1_CLNT_ID, G2_IPC_HPRIO,
				 msg_type, &msg, sizeof(cs_ipsec_ipc_msg_t));
		if (G2_IPC_OK != rc)
			return -1;
		else
			return 0;
		break;
	default:
		DBG(printk("%s:%d: unknown IPSec offload mode %d\n", __func__,
			__LINE__, cs_hw_ipsec_offload_mode));
	}

	return -1;
}				/* ipsec_ipc_send_message */

static int ipsec_ipc_send_init(unsigned char op_id, unsigned short sa_id,
			       unsigned int start_loc0, unsigned int start_loc1)
{
	DBG(printk("%s:%d: op_id %d, sa_id %d, start_loc0 0x%x,"
		" start_loc1 0x%x\n",
		__func__, __LINE__, op_id, sa_id, start_loc0, start_loc1));
	return ipsec_ipc_send_message(op_id, sa_id, CS_IPSEC_IPC_RE_INIT,
				      start_loc0, start_loc1);
}				/* ipsec_ipc_start */

static int ipsec_ipc_send_stop(unsigned char op_id, unsigned short sa_id)
{
	DBG(printk("%s:%d: op_id %d, sa_id %d\n",
		__func__, __LINE__, op_id, sa_id));
	return ipsec_ipc_send_message(op_id, sa_id, CS_IPSEC_IPC_RE_STOP, 0, 0);
}				/* ipsec_ipc_stop */

static int ipsec_ipc_send_update(unsigned char op_id, unsigned short sa_id)
{
	DBG(printk("%s:%d: op_id %d, sa_id %d\n",\
		__func__, __LINE__, op_id, sa_id));
	return ipsec_ipc_send_message(op_id, sa_id, CS_IPSEC_IPC_RE_UPDATE, 0,
		0);
}				/* ipsec_ipc_update */

static int ipsec_ipc_send_dumpstate(unsigned char op_id, unsigned short sa_id)
{
	DBG(printk("%s:%d: op_id %d, sa_id %d\n",
		__func__, __LINE__, op_id, sa_id));
	return ipsec_ipc_send_message(op_id, sa_id, CS_IPSEC_IPC_RE_DUMPSTATE,
		0, 0);
}				/* ipsec_ipc_dumpstate */

static int ipsec_ipc_init_complete_callback(struct ipc_addr peer,
					    unsigned short msg_no,
					    const void *msg_data,
					    unsigned short msg_size,
					    struct ipc_context *context)
{
	unsigned char table_id;
	cs_ipsec_ipc_msg_t *p_msg = (cs_ipsec_ipc_msg_t *) msg_data;

	DBG(printk("%s:%d: client ID %d, CPU ID %d, msg_size %d\n",
		__func__, __LINE__, peer.client_id, peer.cpu_id, msg_size));
	if (msg_size != sizeof(cs_ipsec_ipc_msg_t))
		return -1;
	table_id = p_msg->op_id;

	DBG(printk("%s:%d:got here! table_id = %d\n",
		   __func__, __LINE__, table_id));

	if ((table_id == CS_IPSEC_OP_DEC) || (table_id == CS_IPSEC_OP_ENC))
		f_re_status[table_id] = CS_IPSEC_IPC_RE_ACTIVE;
	return 0;
}				/* ipsec_ipc_start_complete_callback */

static void ipsec_ipc_stop_complete_callback(struct ipc_addr peer,
					     unsigned short msg_no,
					     const void *msg_data,
					     unsigned short msg_size,
					     struct ipc_context *context)
{
	unsigned char table_id;
	unsigned short sa_id;
	cs_ipsec_ipc_msg_t *p_msg = (cs_ipsec_ipc_msg_t *) msg_data;

	if (msg_size != sizeof(cs_ipsec_ipc_msg_t))
		return;
	table_id = p_msg->op_id;
	sa_id = p_msg->sa_id;

	DBG(printk("%s:%d:got here! table_id = %d, sa_id = %d\n",
		   __func__, __LINE__, table_id, sa_id));

	if (table_id > CS_IPSEC_OP_ENC)
		return;

	ipsec_sadb_end(table_id, sa_id);
}				/* ipsec_ipc_stop_complete_callback */

static int ipsec_ipc_update_complete_callback(struct ipc_addr peer,
					      unsigned short msg_no,
					      const void *msg_data,
					      unsigned short msg_size,
					      struct ipc_context *context)
{
	/* FIXME! implementation
	 * What do we do here? do we want to stop the running hash and
	 * change the status of sa? then at update complete, change the
	 * status back to active? */

	DBG(printk("%s:%d: msg_no %d\n", __func__, __LINE__, msg_no));

	return 0;
}				/* ipsec_ipc_update_complete_callback */

static int ipsec_ipc_rcv_stop(struct ipc_addr peer,
			      unsigned short msg_no, const void *msg_data,
			      unsigned short msg_size,
			      struct ipc_context *context)
{
	unsigned char table_id;
	unsigned short sa_id;
	cs_ipsec_ipc_msg_t *p_msg = (cs_ipsec_ipc_msg_t *) msg_data;

	if (msg_size != sizeof(cs_ipsec_ipc_msg_t))
		return -1;
	table_id = p_msg->op_id;
	sa_id = p_msg->sa_id;

	DBG(printk("%s:%d: msg_no %d, op_id %d, sa_id %d\n",
		__func__, __LINE__, msg_no, table_id, sa_id));

	/* mark the tunnel is no longer accelerated */
	return ipsec_sadb_stop(table_id, sa_id, 1);
}				/* ipsec_ipc_rcv_stop */

struct g2_ipc_msg invoke_procs[] = {
	{CS_IPSEC_IPC_RE_INIT_COMPLETE,
	 (unsigned long)ipsec_ipc_init_complete_callback},
	{CS_IPSEC_IPC_RE_STOP_COMPLETE,
	 (unsigned long)ipsec_ipc_stop_complete_callback},
	{CS_IPSEC_IPC_RE_UPDATE_COMPLETE,
	 (unsigned long)ipsec_ipc_update_complete_callback},
	{CS_IPSEC_IPC_RE_STOP_BY_RE, (unsigned long)ipsec_ipc_rcv_stop},
};

/* for every 5 second period, call this timer func to check the status of RE */
void ipsec_ipc_timer_func(unsigned long data)
{
	struct timer_list *p_cs_ipc_timer;
	unsigned int loc0, loc1;

	p_cs_ipc_timer = (struct timer_list *)data;
	loc0 = virt_to_phys((void *)ipsec_sadb_get(CS_IPSEC_OP_DEC, 0));
	loc1 = virt_to_phys((void *)ipsec_sadb_get(CS_IPSEC_OP_ENC, 0));

	/* need to send IPC when the acceleration is first started */
	/* tell IPsec Offload Engine to starts this acceleration */
	switch (cs_hw_ipsec_offload_mode) {
	case IPSEC_OFFLOAD_MODE_BOTH:
		/* DEC */
		if (f_re_status[CS_IPSEC_OP_DEC] == CS_IPSEC_IPC_RE_DEAD) {
			ipsec_ipc_send_init(CS_IPSEC_OP_DEC, 0, loc0, 0);
		}
		/* ENC */
		if (f_re_status[CS_IPSEC_OP_ENC] == CS_IPSEC_IPC_RE_DEAD) {
			ipsec_ipc_send_init(CS_IPSEC_OP_ENC, 0, loc1, 0);
		}
		break;
	case IPSEC_OFFLOAD_MODE_PE0:
		if (f_re_status[CS_IPSEC_OP_DEC] == CS_IPSEC_IPC_RE_DEAD) {
			ipsec_ipc_send_init(CS_IPSEC_OP_DEC, 0, loc0, loc1);
		}
		break;
	case IPSEC_OFFLOAD_MODE_PE1:
		if (f_re_status[CS_IPSEC_OP_ENC] == CS_IPSEC_IPC_RE_DEAD) {
			ipsec_ipc_send_init(CS_IPSEC_OP_ENC, 0, loc0, loc1);
		}
		break;
	default:
		DBG(printk("%s:%d: unknown IPSec offload mode %d\n", __func__,
			__LINE__, cs_hw_ipsec_offload_mode));
	}

/* PRITAM send it once the table address */
//      p_cs_ipc_timer->expires = round_jiffies(jiffies +
//                      (CS_IPSEC_IPC_TIMER_PERIOD * HZ));
//      add_timer(p_cs_ipc_timer);
	return;
}				/* ipsec_ipc_timer_func */

static int ipsec_ipc_register(void)
{
	short status;

	status = g2_ipc_register(CS_IPSEC_IPC_RE0_CLNT_ID, invoke_procs,
				 4, 0, NULL, &ipc_ctxt0);
	if (status != G2_IPC_OK) {
		printk("%s::Failed to register IPC for IPsec Offload RE0\n",
		       __func__);
		return -1;
	} else
		printk("%s::successfully register IPC for RE0\n", __func__);

	status = g2_ipc_register(CS_IPSEC_IPC_RE1_CLNT_ID, invoke_procs,
				 4, 0, NULL, &ipc_ctxt1);
	if (status != G2_IPC_OK) {
		printk("%s::Failed to register IPC for IPsec Offload RE1\n",
		       __func__);
		return -1;
	} else
		printk("%s::successfully register IPC for RE1\n", __func__);

	return 0;
}				/* ipsec_ipc_register */

static void ipsec_ipc_deregister(void)
{
	g2_ipc_deregister(ipc_ctxt0);
	g2_ipc_deregister(ipc_ctxt1);
	printk("%s::Done deregister IPC for IPsec Offload\n", __func__);
}				/* ipsec_ipc_deregister */
#endif

static void cs_ipsec_db_init(void)
{
	ipsec_main_db.p_re0_idx = 0;
	ipsec_main_db.p_re1_idx = 0;
	spin_lock_init(&ipsec_main_db.re0_lock);
	spin_lock_init(&ipsec_main_db.re1_lock);
	spin_lock_init(&ipsec_rx_lock);

	memset(ipsec_main_db.re0_spidx_mask, 0x0,
	       sizeof(u16) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re1_spidx_mask, 0x0,
	       sizeof(u16) * CS_IPSEC_TUN_NUM);

	memset(ipsec_main_db.re0_cb_q, 0x0,
	       sizeof(cs_ipsec_cb_queue_t *) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re1_cb_q, 0x0,
	       sizeof(cs_ipsec_cb_queue_t *) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re0_skb_q, 0x0,
	       sizeof(cs_ipsec_skb_queue_t *) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re1_skb_q, 0x0,
	       sizeof(cs_ipsec_skb_queue_t *) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re0_sadb_q, 0x0,
	       sizeof(cs_ipsec_sadb_t) * CS_IPSEC_TUN_NUM);
	memset(ipsec_main_db.re1_sadb_q, 0x0,
	       sizeof(cs_ipsec_sadb_t) * CS_IPSEC_TUN_NUM);

}				/* cs_ipsec_db_init */

int ipsec_check_skb_accelerate(struct sk_buff *skb,
	unsigned char ip_ver, unsigned char dir) {
	struct iphdr *iph;
	unsigned short new_size = 0;

	if ((CS_IPSEC_IPV4 == ip_ver) &&
	    (CS_IPSEC_INBOUND == dir)) {
		iph = ip_hdr(skb);
		if (iph->frag_off & htons(IP_MF | IP_OFFSET)) {
			return 0;
		}
	}

	new_size = ipsec_calc_orig_skb_size(skb);
	if (new_size > CS_IPSEC_RE_SKB_SIZE_MAX) {
		return 0;
	}
	return 1;
}

/************************ external APIs ****************************/

/************************ main handlers *********************/
/* this function returns 0 when it is ok for Kernel to continue
 * its original task.  CS_DONE means Kernel does not have to
 * handle anymore. */
int cs_ipsec_handler(struct sk_buff *skb, struct xfrm_state *x,
			  unsigned char ip_ver, unsigned char dir)
{
	int status;
	cs_kernel_accel_cb_t *cs_cb = CS_KERNEL_SKB_CB(skb);
	unsigned char table_id = 0;
	unsigned short sa_id = 0;
	cs_ipsec_sadb_t *p_sadb;
	int ret = 0;

	if (f_ipsec_enbl == FALSE)
		return -1;

	/* TODO: need to take care the case that the packet is local generated,
	 * and the packet is going to be processed by the tunnel */
	if ((cs_cb == NULL) || (cs_cb->common.tag != CS_CB_TAG))
		return -1;

	/* check skb->cb..
	 * if IPsec module has already make hw_accel decision on this packet,
	 * then there is something wrong.. packet shouldn't come into this
	 * function again. overwrite the decision with sw_only.
	 * If it is already sw_only... just skip.
	 * if it is hw_accel by someone else... then there is something wrong
	 * too.. Only take care when it is "don't care" from all other CS752
	 * modules */
	/* sw_only can be set by other modules too so ignore it if it comes to
	 * this module */

	if (cs_ipsec_enable() == 0) {
		cs_cb->common.sw_only = CS_SWONLY_STATE;
		return -1;
	}

	status = ipsec_sadb_find_by_xfrm_state(x, &table_id, &sa_id);
	if (status == -1) {
		ret = -1;
		goto out_free;
	}

	/* Make sure the table id matched with the direction. */
	if ((status == 0) && (IPSEC_TABLE_ID_BY_DIR(dir) != table_id)) {
		/* shouldn't happen */
		ret = -1;
		goto out_free;
	}

	p_sadb = ipsec_sadb_get(table_id, sa_id);

	/* check if this tunnel has been accelerated */
	if ((status == 0) && (NULL != p_sadb) && (1 == p_sadb->used)) {

	DBG(printk("%s:%d p_sadb=%p status: used=%d state=%s\n",
		__func__, __LINE__, p_sadb, p_sadb->used,
		ipsec_sadb_state_to_str(p_sadb->state)));

		if (p_sadb->state == SADB_WAIT_PE_STOP) {
			/* this tunnel was accelerated but has been stopped
			 * because some errors occured */
			/* enqueue the packet into tunnel queue */
			ipsec_enqueue_skb_to_sadb(table_id, sa_id, skb);

		} else if (p_sadb->state == SADB_ACCELERATED) {
			/* this tunnel is accelerated, and now there is a newly
			 * introduced traffic. */
			/* estimate the packet size that will be sent to IPsec
			 * Offload engine, and make sure it does not violate
			 * the packet size limitation. */
			/* Continue processing this packet only if this is a new flow */

			if (ipsec_check_skb_accelerate(skb, ip_ver, dir) == 1) {
				if (CS_IPSEC_OUTBOUND == dir) {
					ipsec_send_to_re_linux_skb(table_id, sa_id, skb);
					return CS_DONE;	/* Kernel please skips */
				} else {
					ipsec_set_skb_cb_info(table_id, sa_id, skb);
					return 0;
				}
			} else {
				/* stop the acceleration of this sadb */
				if (CS_IPSEC_OUTBOUND == dir) {
					ipsec_enqueue_skb_to_sadb(table_id, sa_id, skb);
					ipsec_sadb_stop(table_id, sa_id, 1);
					return CS_DONE;	/* Kernel please skips */
				} else {
					cs_cb->common.sw_only = CS_SWONLY_STATE;
					return 0;
				}
			}
		} else if (p_sadb->state == SADB_INIT) {
			/* this tunnel is in the process of being accelerated,
			 * but the task of negotiation to IPsec Offload engine
			 * is not yet complete. 								*/
			/* but the packet which create tunnel hash could be sw_only,
			 * so we make this packet go normal path again.
			 */
		 	if (cs_cb->common.sw_only == CS_SWONLY_STATE)
				return -1;

			ipsec_set_skb_cb_info(table_id, sa_id, skb);

			return 0;
		} else {
			/* this tunnel was no longer accelerated, but we need to
			 * let the kernel process the rest.
			 * and RE already inform stop_callback
			 */
			cs_cb->common.sw_only = CS_SWONLY_STATE;
			DBG(printk("%s:%d set sw_only because sadb is stop\n", __func__, __LINE__));

			return 0;	/* Kernel please continues the rest */
		}
		return CS_DONE;	/* Kernel please skips */
	} else {
		/*
		 * If packet is sw_only, we don't create tunnel hash based on
		 * this packet
		 */
		if (cs_cb->common.sw_only == CS_SWONLY_STATE)
			return -1;

		if (ipsec_check_skb_accelerate(skb, ip_ver, dir) == 0) {
			cs_cb->common.sw_only = CS_SWONLY_STATE;
			return -1;
		}

		/* check if the tunnel can be accelerated by hardware */
		if (0 == ipsec_sadb_check_and_create(dir, x, ip_ver,
						     &table_id, &sa_id)) {
#ifdef CONFIG_CS752X_HW_ACCEL_ETHERIP
	if(cs_cb->common.module_mask & CS_MOD_MASK_ETHERIP) {
		if(ip_ver == CS_IPSEC_IPV6) {
			struct ipv6hdr *iph6 = (struct ipv6hdr *)skb->data;
			p_sadb = ipsec_sadb_get(table_id, sa_id);
			p_sadb->etherip = 1;
			memcpy(&p_sadb->eip_eth_addr[0], (cs_uint8*)skb->data + 42,14);
			/* 40 ip header + 2 byte eip hdr */
			p_sadb->eip_ttl = 255;
			ipv6_addr_copy((struct in6_addr *)&p_sadb->eip_tunnel_daddr.addr,
			       &iph6->daddr);
			ipv6_addr_copy((struct in6_addr *)&p_sadb->eip_tunnel_saddr.addr,
			       &iph6->saddr);
			dma_map_single(NULL, (void *)p_sadb,
					sizeof(cs_ipsec_sadb_t), DMA_FROM_DEVICE);
		}
	}
#endif

			ipsec_set_skb_cb_info(table_id, sa_id, skb);
		} else {
			/* not accelerable. */
			cs_cb->common.sw_only = CS_SWONLY_STATE;
			DBG(printk("%s:%d set sw_only because ipsec_sadb_check_and_create() "\
				"fail such as no available sadb \n",
				__func__, __LINE__));
		}
	}

out_free:
	return ret;
}				/* k_jt_cs_ipsec_handler */

EXPORT_SYMBOL(cs_ipsec_handler);

static void ipsec_insert_sec_path(struct sk_buff *skb, unsigned char table_id,
				  unsigned short sa_id)
{
	cs_ipsec_sadb_t *p_sadb;

	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (p_sadb == NULL)
		return;

	if (!skb->sp || atomic_read(&skb->sp->refcnt) != 1) {
		struct sec_path *sp;

		sp = secpath_dup(skb->sp);
		if (!sp) {
			//XFRM_INC_STATS(net, LINUX_MIB_XFRMINERROR);
			return;
		}
		if (skb->sp)
			secpath_put(skb->sp);
		skb->sp = sp;
	}

	if (skb->sp->len == XFRM_MAX_DEPTH)
		return;

	xfrm_state_hold(p_sadb->x_state);
	skb->sp->xvec[skb->sp->len++] = p_sadb->x_state;

	return;
}				/* ipsec_insert_sec_path */

int cs_hw_accel_ipsec_handle_rx(struct sk_buff *skb, unsigned char src_port,
				unsigned short in_voq_id,
				unsigned int sw_action)
{
	unsigned char op_id = 0, sp_idx, func_id;
	int status;
	unsigned short sa_id;
	cs_kernel_accel_cb_t *cs_cb = CS_KERNEL_SKB_CB(skb);
	cs_ipsec_sadb_t *p_sadb;
	cs_core_hmu_value_t hmu_value;
	int i;
#ifdef CS_IPSEC_DEBUG
	char * temp;
#endif
	unsigned long flags;
	spin_lock_irqsave(&ipsec_rx_lock, flags);

	if (cs_cb == NULL) {
		spin_unlock_irqrestore(&ipsec_rx_lock, flags);
		return -1;
	}
	/* 1) check if the src port is one of the recirculation engine ports,
	 * and make op_id equals to recirculation cpu id */
	/* FIXME!! what needs to be implemented here is that.. based on different
	 * receiving VOQ, we can assigned the right op_id */

	DBG(printk("%s:%d:Got here!! src_port = %d, ", __func__, __LINE__,
		   src_port));
	DBG(printk("in_voq_id = %d, sw_action = %x ", in_voq_id, sw_action));
	DBG(printk("skb->len %d\n",skb->len));

	/* We need to insert some original info here.. */

	/* check the sw_action value from header E */
	if ((CS_FE_INVALID_SW_ACTION == sw_action) || (0 == sw_action)) {
		/* if there is no valid sw_action, that means this packet is a hash miss
		 * from RE. */
		/* 1) fill up the control block info */
		/* notes here.. packet will be from recirculation engine. we will
		 * have to mark this packet with guid.. how do we find its original
		 * selector / SADB? */
		status = ipsec_sadb_find_by_mac_selector(skb, &op_id, &sa_id);
		if (status != 0) {
#ifdef CS_IPSEC_DEBUG
			printk("%s:%d:going to free the packet due to not find sadb\n", __func__,
			     __LINE__);
			temp = skb->data - ETH_HLEN;
			printk("\t mac:");
			for (i = 0; i < ETH_HLEN; i++)
				printk("%02X ", temp[i]);
			printk("\n");
#else
			DBG(printk("%s:%d:going to free the packet due to not find sadb\n", __func__,
			     __LINE__));
#endif
			/* well. i'd suggest just drop this packet for now.
			 * Any better solution? */
			kfree_skb(skb);
			spin_unlock_irqrestore(&ipsec_rx_lock, flags);
			return 0;
		}

		if (op_id != CS_IPSEC_OP_DEC) {
			printk("%s:%d something wrong that RE send Outbound packet but no sw_action \n"
				, __func__, __LINE__);
			kfree_skb(skb);
			spin_unlock_irqrestore(&ipsec_rx_lock, flags);
			return 0;
		}
		/* Here RE id equals table_id and no need for conversion */
		p_sadb = ipsec_sadb_get(op_id, sa_id);
		if ((p_sadb->used == 0) || (p_sadb->x_state->km.state != XFRM_STATE_VALID)) {
#ifdef CS_IPSEC_DEBUG
			printk("%s:%d:free the packet due to used=%d state=%x\n", __func__,
			     __LINE__, p_sadb->used, p_sadb->x_state->km.state);

			temp = skb->data - ETH_HLEN;
			printk("\t mac:");
			for (i = 0; i < ETH_HLEN; i++)
				printk("%02X ", temp[i]);
			printk("\n");
#else
			DBG(printk("%s:%d:free the packet due to used=%d state=%x\n", __func__,
			     __LINE__, p_sadb->used, p_sadb->x_state->km.state));

#endif
			kfree_skb(skb);
			spin_unlock_irqrestore(&ipsec_rx_lock, flags);
			return 0;
		}
		skb_reset_network_header(skb);
		ipsec_set_skb_cb_info(op_id, sa_id, skb);

		/* 2) transmit the packet via the normal Kernel, then
		 * it will be handled and proper hash entry will be create. */

		/* need to fill sec_path in skb, such that the Kernel could realize
		 * this packet when we do netif_rx */
		ipsec_insert_sec_path(skb, op_id, sa_id);
		netif_rx_cs(skb);
	} else {
		/* if there is valid sw_action, that means this packet is one of the
		 * special packet we offload encryption/decryption to RE, we need to
		 * resume the Kernel task before sending it out */
		/* note! in this case, voq may be different if we assign a special
		 * ingress voq for it. */

		sa_id = sw_action & 0x01ff;
		sp_idx = (unsigned char)((sw_action >> 9) & 0x7f);
		func_id = (unsigned char)((sw_action >> 16) & 0x3);

		op_id = CS_IPSEC_GET_RE_ID_FROM_FUNC_ID(func_id);
		DBG(printk("%s:%d:sa_id = %d, sp_idx = %d, func_id = %d, op_id = %d\n",
		     __func__, __LINE__, sa_id, sp_idx, func_id, op_id));

		/* Here RE id equals table_id and no need for conversion */
		p_sadb = ipsec_sadb_get(op_id, sa_id);
		status = ipsec_skb_header_recover_skb_cb_info(op_id, sa_id, sp_idx, skb);
		if (status != 0) {
			kfree_skb(skb);
			spin_unlock_irqrestore(&ipsec_rx_lock, flags);
			return 0;
		}

		skb_reset_network_header(skb);

		/* if it's inbound */
		if (op_id == CS_IPSEC_OP_DEC) {
			printk("%s:%d: any chance to get Inbound packet from RE with sw_action ??\n",
			     __func__, __LINE__);
			DBG(printk("%s:%d:a decryption path packet!!\n", __func__,
			     __LINE__));
			netif_rx(skb);
		} else {	/* if it's outbound */
			/* need to delete two internal hashes*/
			memset(&hmu_value, 0, sizeof(hmu_value));

			hmu_value.type = CS_CORE_HMU_WATCH_SWID64;
			hmu_value.mask = 0x08;
			hmu_value.value.swid64 = CS_IPSEC_SPGID(func_id, sa_id, sp_idx);
			cs_core_hmu_delete_hash(CS_CORE_HMU_WATCH_SWID64, &hmu_value);

			DBG(printk("%s:%d:an encryption path packet!!\n", __func__,
			     __LINE__));
			xfrm_output_resume(skb, 0);
		}
	}

	spin_unlock_irqrestore(&ipsec_rx_lock, flags);
	return 0;
}				/* cs_hw_accel_ipsec_handle_rx */


/* hooks inserted at xfrm_state_update() in net/xfrm/xfrm_state.c */
void cs_ipsec_xfrm_state_update(struct xfrm_state *x)
{
	unsigned char table_id;
	unsigned short sa_id;
	int status;
	cs_ipsec_sadb_t *p_sadb;

	if (f_ipsec_enbl == FALSE)
		return;

	status = ipsec_sadb_find_by_xfrm_state(x, &table_id, &sa_id);
	if (status != 0)
		return;
	DBG(printk("%s:%d status=%d table_id=%d  sa_id=%d\n",
		__func__, __LINE__, x->km.state, table_id, sa_id));

	p_sadb = ipsec_sadb_get(table_id, sa_id);
	if (NULL == p_sadb)
		return;

	/* assuming p_sadb ip_ver and dir are not going to change */
	status = ipsec_sadb_update(table_id, sa_id, p_sadb->sa_dir, x,
				   p_sadb->ip_ver);
	if (status != 0)
		return;

#ifdef CS_IPC_ENABLED
	ipsec_ipc_send_update(table_id, sa_id);
#endif

	/* FIXME! Do we need to stop all the hash? well at least for now
	 * this is not implemented? */
	return;
}				/* k_jt_cs_ipsec_x_state_update */

int cs_ipsec_callback_core_hmu_notify(u32 watch_bitmask,
					      cs_core_hmu_value_t * value,
					      u32 status)
{
	DBG(printk("cs_ipsec_callback_core_hmu_notify watch_bitmask 0x%x status 0x%x ",
	     watch_bitmask, status));
	DBG(printk(" type=%x, swid64=0x%llx \n", value->type, value->value.swid64));
	if (status == CS_CORE_HMU_RET_STATUS_CREATE_SUCCEED) {
		DBG(printk("%s:%d hash for ipsec successfully created\n", __func__,
		     __LINE__));
	}

	return 0;
}

void cs_hw_accel_ipsec_hook_remove(void) {
	f_ipsec_enbl = FALSE;
}

void cs_hw_accel_ipsec_hook_insert(void) {
	f_ipsec_enbl = TRUE;

}

static void cs_hw_accel_ipsec_callback_hma(unsigned long notify_event,
					   unsigned long value)
{
	unsigned short table_id;

	DBG(printk("cs_hw_accel_ipsec_callback_hma notify_event 0x%lx value 0x%lx\n",
	     notify_event, value));
	switch (notify_event) {
		case CS_HAM_ACTION_MODULE_DISABLE:
		case CS_HAM_ACTION_CLEAN_HASH_ENTRY:
			//cs_core_hmu_clean_watch(&ipsec_fwd_hmu_entry);

			for (table_id = 0; table_id < 2; table_id++) {
				ipsec_sadb_stop_all(table_id);
			}
			break;
		case CS_HAM_ACTION_MODULE_REMOVE:
			cs_hw_accel_ipsec_hook_remove();
			break;
		case CS_HAM_ACTION_MODULE_INSERT:
			cs_hw_accel_ipsec_hook_insert();
			break;
	}

}				/* cs_hw_accel_ipsec_callback_hma */

void cs_ipsec_xfrm_state_delete(struct xfrm_state *x)
{
	unsigned char table_id;
	unsigned short sa_id;
	int status;

	status = ipsec_sadb_find_by_xfrm_state(x, &table_id, &sa_id);
	if (status != 0)
		return;
	/* if a xfrm state is deleted, just stop the sadb */
	ipsec_sadb_stop(table_id, sa_id, 1);

	return;
}

/*
 * For SA rekey, it would send XFRM_MSG_EXPIRE message at first
 * then send out XFRM_MSG_DELSA
 */
static int cs_km_send_notify(struct xfrm_state *x, struct km_event *c)
{
	if (f_ipsec_enbl == FALSE)
		return 0;

	switch (c->event) {
		case XFRM_MSG_EXPIRE:
			DBG(printk("%s: xfrm_state=0x%p XFRM_MSG_EXPIRE, "
				"state: %d\n", __func__, x, x->km.state));
			if (x->km.state == XFRM_STATE_DEAD)
				cs_ipsec_xfrm_state_delete(x);
			break;
		case XFRM_MSG_DELSA:
			DBG(printk("%s: xfrm_state=0x%p XFRM_MSG_DELSA\n",
				__func__, x));
			cs_ipsec_xfrm_state_delete(x);
			break;
		case XFRM_MSG_NEWSA:
			DBG(printk("%s: xfrm_state=0x%p XFRM_MSG_NEWSA\n",
				__func__, x));
			break;
		case XFRM_MSG_UPDSA:
			DBG(printk("%s: xfrm_state=0x%p XFRM_MSG_UPDSA\n",
				__func__, x));
			cs_ipsec_xfrm_state_update(x);
			break;
		default:
			DBG(printk("%s:%d xfrm_state=0x%p status=%d \n",
				__func__, __LINE__, x, x->km.state));
			break;
	}
	return 0;
}

static struct xfrm_policy *cs_km_compile_policy(struct sock *sk, int opt,
						u8 *data, int len, int *dir)
{
	return NULL;

}

static int cs_km_send_acquire(struct xfrm_state *x, struct xfrm_tmpl *t, struct xfrm_policy *xp, int dir)
{
	return 0;
}


static struct xfrm_mgr hw_accel_km_mgr =
{
	.id		= "cs_ipsec_hw_accel_km",
	.notify		= cs_km_send_notify,
	.acquire	= cs_km_send_acquire,
	.compile_policy	= cs_km_compile_policy,
	.new_mapping	= NULL,
	.notify_policy	= NULL,
	.migrate	= NULL,
};


void cs_hw_accel_ipsec_init(void)
{
#ifdef CS_IPC_ENABLED
	int status_c;
#endif
	xfrm_register_km(&hw_accel_km_mgr);

	/* initialize the data base */
	cs_ipsec_db_init();

	/* FIXME! allocate classifier and SDB for traffic coming from RE */

	/* hw accel_manger */
	cs_hw_accel_mgr_register_proc_callback(CS752X_ADAPT_ENABLE_IPSEC,
					       cs_hw_accel_ipsec_callback_hma);

	/* register the hook function to core */
	cs_hw_accel_ipsec_hook_insert();


#ifdef CS_IPC_ENABLED
	/* FIXME! make sure IPC is insmod'ed */
	status_c = ipsec_ipc_register();
#endif

#ifdef CONFIG_CS752X_HW_ACCEL_ETHERIP
	/* If etherIP acceleration is enabled then call its init routine */
	cs_hw_accel_etherip_init();
#endif
	memset(&ipsec_fwd_hmu_entry, 0, sizeof(ipsec_fwd_hmu_entry));
	memset(&ipsec_fwd_hmu_value, 0, sizeof(ipsec_fwd_hmu_value));

	ipsec_fwd_hmu_value.type = CS_CORE_HMU_WATCH_SWID64;
	ipsec_fwd_hmu_value.mask = 0x08;
	ipsec_fwd_hmu_value.value.swid64 =
	    CS_SWID64_MASK(CS_SWID64_MOD_ID_IPSEC);
	ipsec_fwd_hmu_entry.watch_bitmask = CS_CORE_HMU_WATCH_SWID64;
	ipsec_fwd_hmu_entry.value_mask = &ipsec_fwd_hmu_value;
	ipsec_fwd_hmu_entry.callback = cs_ipsec_callback_core_hmu_notify;

	cs_core_hmu_register_watch(&ipsec_fwd_hmu_entry);

	return;
}				/* cs_hw_accel_ipsec_init */

void cs_hw_accel_ipsec_exit(void)
{

	cs_hw_accel_ipsec_hook_remove();

	cs_core_hmu_unregister_watch(&ipsec_fwd_hmu_entry);

	/* hw accel_manger */
	cs_hw_accel_mgr_register_proc_callback(CS752X_ADAPT_ENABLE_IPSEC, NULL);

	f_ipsec_enbl = FALSE;

	/* stop all the accelerated tunnel */

	/* deregister the hook function from core */

#ifdef CS_IPC_ENABLED
	ipsec_ipc_deregister();
#endif

}				/* cs_hw_accel_ipsec_exit */

#ifdef CS_IPC_ENABLED
int cs_hw_accel_ipsec_ipc_status(unsigned char table_id)
{
	return f_re_status[table_id];
}
#endif

void cs_hw_accel_ipsec_print_sadb_status(void)
{
	unsigned short sa_id;
	unsigned short table_id;
	cs_ipsec_sadb_t * p_sadb;
	u16 mask;
	printk("CS_IPSEC HW ACCELERATION SADB Management table: \n");
	for (table_id = 0; table_id < 2; table_id++) {
		printk("table %s \n", (table_id == 0) ? "Inbound" : "Outbound");
		for (sa_id = 0; sa_id < CS_IPSEC_TUN_NUM; sa_id++) {
			printk("\t sa_id %d: ", sa_id);
			p_sadb = ipsec_sadb_get(table_id, sa_id);

			if (p_sadb != NULL) {
				printk(" used=%d spi=%x state=%s ipv%s",
					p_sadb->used, p_sadb->spi, ipsec_sadb_state_to_str(p_sadb->state),
					(p_sadb->ip_ver == 0)?"4":"6");
				mask = (table_id == 0) ? ipsec_main_db.re0_spidx_mask[sa_id]
					: ipsec_main_db.re1_spidx_mask[sa_id];
				printk(" spidx_mask = 0x%x", mask);
			} else {
				printk(" NULL");
			}
			printk("\n");
		}
	}
}


void cs_hw_accel_ipsec_clean_sadb_status(void)
{
	unsigned short sa_id;
	unsigned short table_id;
	cs_ipsec_sadb_t * p_sadb;
	printk("Clean CS_IPSEC HW ACCELERATION SADB Management table\n");

	for (sa_id = 0; sa_id < CS_IPSEC_TUN_NUM; sa_id++) {
		for (table_id = 0; table_id < 2; table_id++) {
			p_sadb = ipsec_sadb_get(table_id, sa_id);
			if (p_sadb != NULL) {
				//p_sadb->used = 0;
				//p_sadb->spi = 0;
				//p_sadb->state = SADB_INIT;
				ipsec_sadb_stop(table_id, sa_id, 1);
			}
		}
	}

}

void cs_hw_accel_ipsec_print_pe_sadb_status(void)
{
	ipsec_ipc_send_dumpstate(0, 0);
	ipsec_ipc_send_dumpstate(1, 0);
}

void cs_hw_accel_ipsec_clean_pe_sadb_status(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		printk("send stop to said %d\n", i);
		ipsec_ipc_send_stop(0, i);
		ipsec_ipc_send_stop(1, i);
	}
}

