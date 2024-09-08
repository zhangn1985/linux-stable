/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ETNAVIV_PCI_DRV_H__
#define __ETNAVIV_PCI_DRV_H__

#ifdef CONFIG_DRM_ETNAVIV_PCI_DRIVER

#define ETNA_MAX_IP_BLOCK          4

enum etnaviv_pci_chip_id {
	GCCORE_PCI_CHIP_ID_UNKNOWN = 0,
	JM9100 = 1,
	JD9230P = 2,
	GP102 = 3,
	GC1000_IN_LS7A1000 = 4,
	GCCORE_PCI_CHIP_ID_LAST,
};

struct vivante_gc_ip_block {
	u32 id;
	u32 offset;
	u32 size;
	char compatible[20];
};

struct etnaviv_pcie_ip_funcs {
	int (*init)(struct pci_dev *pdev);
	void (*fini)(struct pci_dev *pdev);
};

struct etnaviv_pci_gpu_data {
	enum etnaviv_pci_chip_id chip_id;
	u32 num_core;
	u32 num_vram;
	u32 vram_bars[2];
	u32 mmio_bar;
	struct vivante_gc_ip_block ip_block[ETNA_MAX_IP_BLOCK];
	bool has_dedicated_vram;
	bool has_iatu;
	bool has_display;
	const struct etnaviv_pcie_ip_funcs *pcie_ip_funcs;
	char market_name[24];
};

const struct etnaviv_pci_gpu_data *
etnaviv_pci_get_match_data(struct device *dev);

int etnaviv_register_pci_driver(void);
void etnaviv_unregister_pci_driver(void);

int jemo_pcie_init(struct pci_dev *pdev);

#else

static inline int etnaviv_register_pci_driver(void) { return 0; }
static inline void etnaviv_unregister_pci_driver(void) { }

static inline struct etnaviv_pci_gpu_data *
etnaviv_pci_get_match_data(struct device *dev) { return NULL; }

#endif

#endif
