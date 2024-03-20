// SPDX-License-Identifier: GPL-2.0+
//
// Copyright (C) 2019 Christian Hewitt <christianshewitt@gmail.com>
// Copyright (C) 2024 Zhang Ning <zhangn1985@outlook.com>

/*
 * Keytable for the Jianpian TV Box remote control
 */

#include <media/rc-map.h>
#include <linux/module.h>

#define RC_MAP_JIANPIAN "rc-jianpian"

static struct rc_map_table jianpian[] = {
	{ 130524, KEY_POWER },
	{ 130501, KEY_MUTE }, // mouse

	{ 130488, KEY_VOLUMEDOWN },
	{ 130465, KEY_VOLUMEUP },

	{ 130506, KEY_UP },
	{ 130514, KEY_DOWN },
	{ 130457, KEY_LEFT },
	{ 130497, KEY_RIGHT },
	{ 130510, KEY_OK },

	{ 130512, KEY_BACK },
	// { 130512, KEY_MENU }, //mic, no scancode
	{ 130453, KEY_HOME },

	{ 130546, KEY_MENU }, //Youtube
};

static struct rc_map_list jianpian_map = {
	.map = {
		.scan     = jianpian,
		.size     = ARRAY_SIZE(jianpian),
		.rc_proto = RC_PROTO_NEC,
		.name     = RC_MAP_JIANPIAN,
	}
};

static int __init init_rc_map_jianpian(void)
{
	return rc_map_register(&jianpian_map);
}

static void __exit exit_rc_map_jianpian(void)
{
	rc_map_unregister(&jianpian_map);
}

module_init(init_rc_map_jianpian)
module_exit(exit_rc_map_jianpian)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhang Ning <zhangn1985@outlook.com>");
MODULE_DESCRIPTION("Jianpian TV Box remote controller keytable");
