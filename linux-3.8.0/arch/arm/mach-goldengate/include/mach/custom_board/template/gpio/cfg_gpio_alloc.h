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

/* Map GPIO pins to BSP definitions using GPIO for specific functions */

/* these are likely good defaults for most set ups based on Cortina reference boards, lets try them out */

#if 1 /* template start */

#define GPIO_FLASH_WP_CNTRL     GPIO2_BIT19
#define GPIO_VOIP_LED           GPIO2_BIT20
#define GPIO_SLIC0_RESET        GPIO2_BIT22
#define GPIO_SLIC_INT           GPIO2_BIT26
#define GPIO_PCIE_RESET         GPIO2_BIT28     /* #define GPIO2_BIT28  32*2+28 */
#define GPIO_USB_VBUS_POWER_0   GPIO2_BIT24
#define GPIO_USB_VBUS_POWER_1   GPIO2_BIT25

#endif /* template end */
