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

/* Custom cs75xx phone resources for use with VoIP drivers */

/* If you have any additional cs75xx phone resources assign them here */
/* If you dont know what this is, leave it commented out */
/* This code is only enabled if you have defined CONFIG_PHONE_CS75XX_WRAPPER for 
 * a specific purpose. 
 */

#if 0 /* template start */
	{
	 .name = "my_resource_name",
         .start = MY_START_VALUE,
         .end = MY_END_VALUE,
         .flags = MY_IRQ,
         },
        {
         .name = "dev_num",
         .start = 1,
         .end = 1,
         .flags = IORESOURCE_IRQ,
         },
        {
         .name = "chan_num",
         .start = 2,
         .end = 2,
         .flags = IORESOURCE_IRQ,
         },

#endif /* template end */
