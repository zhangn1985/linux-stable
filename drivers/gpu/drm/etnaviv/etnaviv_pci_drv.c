// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>

#include "etnaviv_drv.h"
#include "etnaviv_pci_drv.h"

static const struct etnaviv_pcie_ip_funcs jemo_9xxxx_gpu_pcie_ip_funcs = {
	.init = jemo_pcie_init,
	.fini = NULL,
};

static const struct etnaviv_pci_gpu_data
gccore_platform_data[GCCORE_PCI_CHIP_ID_LAST] = {
	{
		.chip_id = GCCORE_PCI_CHIP_ID_UNKNOWN,
	},
	{
		.chip_id = JM9100,
		.num_core = 1,
		.num_vram = 2,
		.vram_bars = {0, 2},
		.mmio_bar = 1,
		.ip_block = {{0, 0x00900000, 0x00004000, "etnaviv-gpu,3d"},},
		.has_dedicated_vram = true,
		.has_iatu = true,
		.has_display = true,
		.pcie_ip_funcs = &jemo_9xxxx_gpu_pcie_ip_funcs,
		.market_name = "JingJia Micro JM9100",
	},
	{
		.chip_id = JD9230P,
		.num_core = 2,
		.num_vram = 2,
		.vram_bars = {0, 2},
		.mmio_bar = 1,
		.ip_block = {{0, 0x00900000, 0x00004000, "etnaviv-gpu,3d"},
			     {1, 0x00910000, 0x00004000, "etnaviv-gpu,3d"},},
		.has_dedicated_vram = true,
		.has_iatu = true,
		.has_display = true,
		.pcie_ip_funcs = &jemo_9xxxx_gpu_pcie_ip_funcs,
		.market_name = "JingJia Micro JD9230P",
	},
	{
		.chip_id = GP102,
		.num_core = 2,
		.num_vram = 1,
		.vram_bars = {0,},
		.mmio_bar = 2,
		.ip_block = {{0, 0x00040000, 0x00004000, "etnaviv-gpu,3d"},
			     {0, 0x000C0000, 0x00004000, "etnaviv-gpu,2d"},},
		.has_dedicated_vram = true,
		.has_iatu = false,
		.has_display = true,
		.market_name = "LingJiu GP102",
	},
	{
		.chip_id = GC1000_IN_LS7A1000,
		.num_core = 1,
		.num_vram = 1,
		.vram_bars = {2,},
		.mmio_bar = 0,
		.ip_block = {{0, 0, 0x00004000, "etnaviv-gpu,3d"},},
		.has_dedicated_vram = true,
		.has_iatu = false,
		.has_display = false,
		.market_name = "GC1000 in LS7A1000",
	},
};

static const char *match_names[ETNA_MAX_IP_BLOCK] = {0};

static const struct etnaviv_pci_gpu_data *
etnaviv_pci_get_platform_data(const struct pci_device_id *entity)
{
	enum etnaviv_pci_chip_id chip_id = entity->driver_data;

	if (chip_id == GCCORE_PCI_CHIP_ID_UNKNOWN ||
	    chip_id >= GCCORE_PCI_CHIP_ID_LAST)
		return NULL;

	return &gccore_platform_data[chip_id];
}

static void platform_device_remove_callback(void *data)
{
	struct platform_device *pdev = (struct platform_device *)data;

	etnaviv_destroy_platform_device(&pdev);
}

static int etnaviv_pci_probe(struct pci_dev *pdev,
			     const struct pci_device_id *ent)
{
	const struct etnaviv_pci_gpu_data *pdata;
	const struct etnaviv_pcie_ip_funcs *pcie_ip_funcs;
	struct device *dev = &pdev->dev;
	unsigned int i;
	unsigned int num_core;
	int ret;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	pdata = etnaviv_pci_get_platform_data(ent);
	if (!pdata)
		return -ENODEV;

	pcie_ip_funcs = pdata->pcie_ip_funcs;
	if (pcie_ip_funcs) {
		ret = pcie_ip_funcs->init(pdev);
		if (ret)
			return ret;
	}

	num_core = pdata->num_core;

	dev_info(dev, "%s has %u GPU cores\n", pdata->market_name, num_core);

	for (i = 0; i < num_core; i++) {
		const struct vivante_gc_ip_block *pblock = &pdata->ip_block[i];
		struct platform_device *virtual_child;
		resource_size_t start;
		struct resource res;

		start = pci_resource_start(pdev, pdata->mmio_bar);
		memset(&res, 0, sizeof(res));
		res.flags = IORESOURCE_MEM;
		res.name = "registers";
		res.start = start + pblock->offset;
		res.end = start + pblock->offset + pblock->size - 1;

		ret = etnaviv_create_platform_device(dev,
						     pblock->compatible,
						     pblock->id,
						     &res,
						     (void *)pdata,
						     sizeof(*pdata),
						     &virtual_child);
		if (ret)
			return ret;

		ret = devm_add_action_or_reset(dev,
					       platform_device_remove_callback,
					       virtual_child);
		if (ret)
			return ret;

		match_names[i] = dev_name(&virtual_child->dev);
	}

	for (i = 0; i < num_core; i++)
		dev_info(dev, "ip block %u: %s\n", i, match_names[i]);

	ret = etnaviv_create_platform_device(dev, "etnaviv",
					     PLATFORM_DEVID_NONE,
					     NULL, match_names,
					     sizeof(match_names),
					     &etnaviv_drm);
	if (ret)
		return ret;

	return 0;
}

static void etnaviv_pci_remove(struct pci_dev *pdev)
{

}

static const struct pci_device_id etnaviv_pci_id_list[] = {
	{0x0731, 0x9100, PCI_ANY_ID, PCI_ANY_ID, 0, 0, JM9100},
	{0x0731, 0x9230, PCI_ANY_ID, PCI_ANY_ID, 0, 0, JD9230P},
	{0x0709, 0x0001, PCI_ANY_ID, PCI_ANY_ID, 0, 0, GP102},
	{0x0014, 0x7A15, PCI_ANY_ID, PCI_ANY_ID, 0, 0, GC1000_IN_LS7A1000},
	{ }
};

static struct pci_driver etnaviv_pci_driver = {
	.name = "etnaviv",
	.id_table = etnaviv_pci_id_list,
	.probe = etnaviv_pci_probe,
	.remove = etnaviv_pci_remove,
};

int etnaviv_register_pci_driver(void)
{
	return pci_register_driver(&etnaviv_pci_driver);
}

void etnaviv_unregister_pci_driver(void)
{
	pci_unregister_driver(&etnaviv_pci_driver);
}

MODULE_DEVICE_TABLE(pci, etnaviv_pci_id_list);
