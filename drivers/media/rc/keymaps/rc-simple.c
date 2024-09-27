// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * rc-simple.c - Generic RC keymap tree support. .
 *
 * Copyright (C) 2024 Zhang Ning
 *
 * Author: Zhang Ning <zhangn1985@outlook.com>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#define DRV_NAME "simple-keymap"
#define KEYMAP_NAME "rc-simple"
#include <media/rc-map.h>

static int simple_keymap_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	struct rc_map_table *simple_table;
	struct rc_map_list *simple_map;
	unsigned numkeys;

	int len1, len2; 
	int ret;
	unsigned i;
	u64 scancode;
	u32 keycode;

	
	simple_map = devm_kzalloc(dev, sizeof(struct rc_map_list),
				GFP_KERNEL);
	simple_map->map.name = KEYMAP_NAME;

	ret = of_property_read_u32(np, "proto", &simple_map->map.rc_proto);
	if (!ret) {
		dev_err(dev, "proto DT property missing");
		return -ENODEV;
	}

	len1 = of_property_count_elems_of_size(np, "rc-scancode", sizeof(u64));
	if (!len1) {
		dev_err(dev, "rc-scancode DT property failed");
		return -ENODEV;
	}

	len2 = of_property_count_elems_of_size(np, "rc-keycode", sizeof(u32));
	if (!len2) {
		dev_err(dev, "rc-keycode DT property failed");
		return -ENODEV;
	}
	
	if (len1 != len2) {
		dev_err(dev, "rc-keycode and scancode length mismatch!");
		return -ENODEV;
	}
	numkeys = len1;
	simple_table = devm_kzalloc(dev, sizeof(struct rc_map_table) * numkeys,
				GFP_KERNEL);
				
	for (i = 0; i < numkeys; i++) {
		ret = of_property_read_u64_index(np, "rc-scancode", i, &scancode);
		if (!ret)
			return -ENODEV;
		simple_table->scancode = scancode;
		ret = of_property_read_u32_index(np, "rc-scancode", i, &keycode);
		if (!ret)
			return -ENODEV;
		simple_table->keycode = keycode;
		simple_table++;
	}
	simple_map->map.scan = simple_table;
	simple_map->map.size = numkeys;

	platform_set_drvdata(pdev, simple_map);
	return rc_map_register(simple_map);
}

static void simple_keymap_remove(struct platform_device *pdev)
{
	struct rc_map_list *simple_map = platform_get_drvdata(pdev);
	rc_map_unregister(simple_map);
}


static const struct of_device_id simple_keymap_ids[] = {
        { .compatible = DRV_NAME, },
        { }
};
MODULE_DEVICE_TABLE(of, simple_keymap_ids);

static struct platform_driver simple_keymap_driver = {
        .driver = {
                .name = DRV_NAME,
                .of_match_table = of_match_ptr(simple_keymap_ids),
        },
        .probe = simple_keymap_probe,
	.remove = simple_keymap_remove,
};

module_platform_driver(simple_keymap_driver);

MODULE_DESCRIPTION("Simple rc keymap driver");
MODULE_AUTHOR("Zhang Ning <zhangn1985@outlook.com>");
MODULE_LICENSE("GPL");

