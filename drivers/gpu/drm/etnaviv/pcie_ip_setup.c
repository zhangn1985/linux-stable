// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>

#include "etnaviv_drv.h"
#include "etnaviv_pci_drv.h"

#define PCIE_IATU_BASE_ADDR               0x10000
#define PCIE_IATU_BAR_ADDR_INC            0x200

#define PCIE_REGION_INBOUND               1
#define PCIE_REGION_OUTBOUND              0
#define PCIE_REGION_DIRECT_BIT            31
#define PCIE_REGION_DIRECT_BITMASK        0x80000000
#define PCIE_REGION_INDEX_BITMASK         0x7FFFFFFF

#define PCIE_REGION_TYPE_MEM              0x00
#define PCIE_REGION_TYPE_IO               0x10

#define PCIE_REGION_MATCH_BAR             1
#define PCIE_REGION_MATCH_ADDR            0

#define PCIE_REGION_ENABLE_BITMASK        BIT(31)
#define PCIE_REGION_ENABLE_BIT            BIT(31)
#define PCIE_REGION_MODE_BITMASK          BIT(30)
#define PCIE_REGION_MODE_BIT              BIT(30)

#define PCIE_REGION_BAR_NUM_BITMASK       GENMASK(10, 8)
#define PCIE_REGION_BAR_NUM_SHIFT         8

#define PCIE_REGION_INBOUND_TYPE          0x100
#define PCIE_REGION_INBOUND_CTRL          0x104
#define PCIE_REGION_INBOUND_ADDR_LO       0x114
#define PCIE_REGION_INBOUND_ADDR_HI       0x118

static void iatu_write(void __iomem *iatu, u32 bar, u32 offset, u32 value)
{
	u32 bar_base = PCIE_IATU_BASE_ADDR + bar * PCIE_IATU_BAR_ADDR_INC;

	writel(value, iatu + bar_base + offset);
}

static u32 iatu_read(void __iomem *iatu, u32 bar, u32 offset)
{
	u32 bar_base = PCIE_IATU_BASE_ADDR + bar * PCIE_IATU_BAR_ADDR_INC;

	return readl(iatu + bar_base + offset);
}

static int iatu_map_bar(void __iomem *iatu, u32 bar, u64 axi_addr)
{
	u32 addr_hi = axi_addr >> 32;
	u32 addr_lo = axi_addr & 0xffffffff;
	u32 val;

	iatu_write(iatu, bar + 9, PCIE_REGION_INBOUND_ADDR_LO, addr_lo);
	iatu_write(iatu, bar + 9, PCIE_REGION_INBOUND_ADDR_HI, addr_hi);
	iatu_write(iatu, bar + 9, PCIE_REGION_INBOUND_TYPE,
				  PCIE_REGION_TYPE_MEM);

	val = PCIE_REGION_ENABLE_BIT |
	      PCIE_REGION_MODE_BIT |
	      bar << PCIE_REGION_BAR_NUM_SHIFT;
	iatu_write(iatu, bar + 9, PCIE_REGION_INBOUND_CTRL, val);

	/* sanity check */
	val = iatu_read(iatu, bar + 9, PCIE_REGION_INBOUND_ADDR_LO);
	if (val != addr_lo) {
		pr_err("%s : %u\n", __func__, __LINE__);
		return -EINVAL;
	}

	val = iatu_read(iatu, bar + 9, PCIE_REGION_INBOUND_ADDR_HI);
	if (val != addr_hi) {
		pr_err("%s : %u\n", __func__, __LINE__);
		return -EINVAL;
	}

	return 0;
}

int jemo_pcie_init(struct pci_dev *pdev)
{
	void __iomem *iatu;
	int ret;

	/* Bar 4 is PCIe iATU */
	iatu = pci_iomap(pdev, 4, 0);
	if (!iatu)
		return -ENOMEM;

	ret = iatu_map_bar(iatu, 0, 0x10000000);
	if (ret)
		return ret;

	ret = iatu_map_bar(iatu, 1, 0x00000000);
	if (ret)
		return ret;

	ret = iatu_map_bar(iatu, 2, 0x10000000);
	if (ret)
		return ret;

	pci_iounmap(pdev, iatu);

	dev_info(&pdev->dev, "PCIe iATU init done\n");

	return 0;
}
