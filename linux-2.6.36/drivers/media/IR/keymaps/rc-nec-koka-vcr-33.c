/* rc5-hauppauge-new.h - Keytable for rc5_dvb_t_new Remote Controller
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

static struct ir_scancode nec_koka_vcr_33[] = {
	/* Power */
	{ 0x613E609F, KEY_POWER },
	/* Keys 0 to 9 */
	{ 0x613E08f7, KEY_0 },
	{ 0x613E8877, KEY_1 },
	{ 0x613E48B7, KEY_2 },
	{ 0x613EC837, KEY_3 },
	{ 0x613E28D7, KEY_4 },
	{ 0x613EA857, KEY_5 },
	{ 0x613E6897, KEY_6 },
	{ 0x613EE817, KEY_7 },
	{ 0x613E18E7, KEY_8 },
	{ 0x613E9867, KEY_9 },
	/* Function */
	{ 0x613E906F, KEY_TV2 },	/* CATV */
	{ 0x613E00FF, KEY_TV },		/* TVAV */
	{ 0x613ED827, KEY_DISPLAYTOGGLE },	/* DISP */
	//{ 0x0, KEY_MUTE},
	//{ 0x613EE01F, KEY_CHANNELUP},
	//{ 0x613E807F, KEY_CHANNELDOWN},
	//{ 0x0, KEY_VOLUMEUP},
	//{ 0x0, KEY_VOLUMEDOWN},
	{ 0x613ED02F, KEY_ENTER },
	{ 0x613EF00F, KEY_PROGRAM },	/* PA */
	{ 0x613EE01F, KEY_UP },		/* SEL_UP */
	{ 0x613EE01F, KEY_DOWN },	/* SEL_DOWN */
	{ 0x613E30CF, KEY_UNDO },	/* RET */
	//{ 0x0, KEY_SOUND},
	{ 0x613E20DF, KEY_STOP },
	{ 0x613EC03F, KEY_PLAY },
	{ 0x613E40BF, KEY_BACK },	/* REV */
	{ 0x613E10EF, KEY_FORWARD },
	{ 0x613E50AF, KEY_RECORD },
	{ 0x613EA05F, KEY_PAUSE },
	{ 0x613EF807, KEY_MENU },
	{ 0x613EB04F, KEY_LANGUAGE },	/* MTS */
};

static struct rc_keymap nec_koka_vcr_33_map = {
	.map = {
		.scan    = nec_koka_vcr_33,
		.size    = ARRAY_SIZE(nec_koka_vcr_33),
		.ir_type = IR_TYPE_NEC,
		.name    = RC_MAP_NEC_KOKA_VCR_33,
	}
};

static int __init init_rc_map_nec_koka_vcr_33(void)
{
	return ir_register_map(&nec_koka_vcr_33_map);
}

static void __exit exit_rc_map_nec_koka_vcr_33(void)
{
	ir_unregister_map(&nec_koka_vcr_33_map);
}

module_init(init_rc_map_nec_koka_vcr_33)
module_exit(exit_rc_map_nec_koka_vcr_33)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mauro Carvalho Chehab <mchehab@redhat.com>");
