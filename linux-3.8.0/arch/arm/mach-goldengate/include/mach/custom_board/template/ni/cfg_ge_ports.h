/* 2012 (c) Copyright Cortina Systems Inc.
 * Author: Alex Nemirovsky <alex.nemirovsky@cortina-systems.com>
 *
 * This file is licensed under the terms of the GNU General Public License version 2. 
 * This file is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * WARNING!!: DO NOT MODIFY THIS TEMPLATE FILE 
 * 
 * Future Cortina releases updates will overwrite this location. 
 *
 * Instead, copy out this template into your own custom_board/my_board_name tree 
 * and create a patch against the Cortina source code which included this template file
 * from this location. When your code is fully functional, your patch should also 
 * remove the #warning message from the code which directed you
 * to this template file for inspection and customization.
 */ 

/* Configure Cortina NI Gigabit Ethernet Ports */

/* these are likely good defaults for most set ups, lets try them out */
#if 1 /* template start */

[GE_PORT0_CFG] = {
                .auto_nego = AUTONEG_ENABLE,   	/* AUTONEG_ENABLE or AUTONEG_DISABLE */
#if (GMAC0_PHY_MODE == NI_MAC_PHY_RGMII_1000)
                .speed = SPEED_1000,
#else
                .speed = SPEED_100,
#endif
                .irq = IRQ_NI_RX_XRAM0, 	/* Interrupt assignment */
                .phy_mode = GMAC0_PHY_MODE,     /* GMAC0_PHY_MODE, NI_MAC_PHY_RMII, NI_MAC_PHY_RMII_1000, etc */
                .flowctrl = 0,			/* 0 (disable pause frame), FLOW_CTRL_TX | FLOW_CTRL_RX, etc */
                .full_duplex = DUPLEX_FULL,	/* full or half duplex */
                .port_id = GE_PORT0,
                .phy_addr = GE_PORT0_PHY_ADDR,
                .mac_addr = (&(eth_mac[0][0])),
#if defined(CONFIG_CS75XX_GMAC0_RMII) && \
        defined(CONFIG_CS75XX_INT_CLK_SRC_RMII_GMAC0)
                .rmii_clk_src = 1,
#else
                .rmii_clk_src = 0,
#endif
        },
[GE_PORT1_CFG] = {
                .auto_nego = AUTONEG_ENABLE,    /* AUTONEG_ENABLE or AUTONEG_DISABLE */
#if (GMAC1_PHY_MODE == NI_MAC_PHY_RGMII_1000)
                .speed = SPEED_1000,
#else
                .speed = SPEED_100,
#endif
                .irq = IRQ_NI_RX_XRAM1,         /* Interrupt assignment */
                .phy_mode = GMAC1_PHY_MODE,     /* GMAC1_PHY_MODE, NI_MAC_PHY_RMII, NI_MAC_PHY_RMII_1000, etc */
                .flowctrl = 0,                  /* 0 (disable pause frame), FLOW_CTRL_TX | FLOW_CTRL_RX, etc */
                .full_duplex = DUPLEX_FULL,     /* full or half duplex */
                .port_id = GE_PORT1,
                .phy_addr = GE_PORT1_PHY_ADDR,
                .mac_addr = (&(eth_mac[1][0])),
#if defined(CONFIG_CS75XX_GMAC1_RMII) && \
        defined(CONFIG_CS75XX_INT_CLK_SRC_RMII_GMAC1)
                .rmii_clk_src = 1,
#else
                .rmii_clk_src = 0,
#endif
        },
[GE_PORT2_CFG] = {
               .auto_nego = AUTONEG_ENABLE,    /* AUTONEG_ENABLE or AUTONEG_DISABLE */
#if (GMAC2_PHY_MODE == NI_MAC_PHY_RGMII_1000)
                .speed = SPEED_1000,
#else
                .speed = SPEED_100,
#endif
                .irq = IRQ_NI_RX_XRAM2,         /* Interrupt assignment */
                .phy_mode = GMAC2_PHY_MODE,     /* GMAC2_PHY_MODE, NI_MAC_PHY_RMII, NI_MAC_PHY_RMII_1000, etc */
                .flowctrl = 0,                  /* 0 (disable pause frame), FLOW_CTRL_TX | FLOW_CTRL_RX, etc */
                .full_duplex = DUPLEX_FULL,     /* full or half duplex */
                .port_id = GE_PORT2,
                .phy_addr = GE_PORT2_PHY_ADDR,
                .mac_addr = (&(eth_mac[2][0])),
#if defined(CONFIG_CS75XX_GMAC0_RMII) && \
        defined(CONFIG_CS75XX_INT_CLK_SRC_RMII_GMAC0)
                .rmii_clk_src = 1,
#else
                .rmii_clk_src = 0,
#endif
        }


#endif /* template end */
