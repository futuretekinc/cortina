/* rc5-hauppauge-new.h - Keytable for rc5_dvb_t Remote Controller
 *
 * keymap imported from ir-keymaps.c
 *
 * Copyright (c) 2010 by Mauro Carvalho Chehab <mchehab@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <media/rc-map.h>

/*
 * Hauppauge:the newer, gray remotes (seems there are multiple
 * slightly different versions), shipped with cx88+ivtv cards.
 *
 * This table contains the complete RC5 code, instead of just the data part
 */

static struct ir_scancode rc5_dvb_t[] = {
	/* Power */
	{ 0x38000000, KEY_POWER },
	/* Keys 0 to 9 */
	{ 0x39100000, KEY_0 },
	{ 0x38400000, KEY_1 },
	{ 0x38500000, KEY_2 },
	{ 0x38600000, KEY_3 },
	{ 0x38800000, KEY_4 },
	{ 0x38900000, KEY_5 },
	{ 0x38A00000, KEY_6 },
	{ 0x38C00000, KEY_7 },
	{ 0x38D00000, KEY_8 },
	{ 0x38E00000, KEY_9 },
	/* Function */
	{ 0x39200000, KEY_SCREEN },
	{ 0x38300000, KEY_MUTE },
	{ 0x38200000, KEY_ZOOM },
	{ 0x38100000, KEY_REFRESH },
	{ 0x38F00000, KEY_CHANNELUP },
	{ 0x39300000, KEY_CHANNELDOWN },
	{ 0x38700000, KEY_VOLUMEUP },
	{ 0x38B00000, KEY_VOLUMEDOWN },
	{ 0x39000000, KEY_UP },
	{ 0x39400000, KEY_DOWN },
	{ 0x39500000, KEY_ENTER },
	{ 0x39600000, KEY_RECORD },
	{ 0x39700000, KEY_STOP },
};

static struct rc_keymap rc5_dvb_t_map = {
	.map = {
		.scan    = rc5_dvb_t,
		.size    = ARRAY_SIZE(rc5_dvb_t),
		.ir_type = IR_TYPE_RC5,
		.name    = RC_MAP_RC5_DVB_T,
	}
};

static int __init init_rc_map_rc5_dvb_t(void)
{
	return ir_register_map(&rc5_dvb_t_map);
}

static void __exit exit_rc_map_rc5_dvb_t(void)
{
	ir_unregister_map(&rc5_dvb_t_map);
}

module_init(init_rc_map_rc5_dvb_t)
module_exit(exit_rc_map_rc5_dvb_t)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
