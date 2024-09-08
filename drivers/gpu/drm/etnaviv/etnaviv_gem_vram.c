// SPDX-License-Identifier: GPL-2.0

#include <linux/pci.h>

#include "etnaviv_drv.h"
#include "etnaviv_gem.h"
#include "etnaviv_gem_vram.h"
#include "etnaviv_pci_drv.h"

static struct lock_class_key etnaviv_vram_lock_class;

static u64 etnaviv_obj_cpu_phys_addr(struct etnaviv_gem_object *etnaviv_obj)
{
	struct drm_gem_object *obj = &etnaviv_obj->base;
	struct etnaviv_drm_private *priv = to_etnaviv_priv(obj->dev);

	if (!etnaviv_obj->vram_np) {
		drm_err(obj->dev, "No backing vram node, please pin it\n");
		return 0;
	}

	return priv->vram.cpu_base + etnaviv_obj->vram_np->start;
}

u64 etnaviv_obj_gpu_phys_addr(struct etnaviv_gem_object *etnaviv_obj)
{
	struct drm_gem_object *obj = &etnaviv_obj->base;
	struct etnaviv_drm_private *priv = to_etnaviv_priv(obj->dev);

	if (!etnaviv_obj->vram_np) {
		drm_err(obj->dev, "No backing vram node, no gpu offset\n");
		return 0;
	}

	return priv->vram.gpu_base + etnaviv_obj->vram_np->start;
}

/* Called with etnaviv_obj->lock held, allocate pages from dedicated VRAM */
static int etnaviv_gem_vram_get_pages(struct etnaviv_gem_object *etnaviv_obj)
{
	struct drm_gem_object *obj = &etnaviv_obj->base;
	struct drm_device *drm = obj->dev;
	struct etnaviv_drm_private *priv = to_etnaviv_priv(drm);
	unsigned int npages = obj->size >> PAGE_SHIFT;
	struct drm_mm_node *vram_np;
	struct page **ppages;
	u64 gpu_paddr;
	unsigned int i;
	int ret;

	lockdep_assert_held(&etnaviv_obj->lock);

	vram_np = kzalloc(sizeof(*vram_np), GFP_KERNEL);
	if (!vram_np)
		return -ENOMEM;

	ret = drm_mm_insert_node(&priv->vram.mm, vram_np, obj->size);
	if (ret) {
		drm_err(drm, "Failed to insert %zu KiB\n", obj->size >> 10);
		goto fail_allocate_range;
	}

	ppages = kvmalloc_array(npages, sizeof(*ppages), GFP_KERNEL);
	if (!ppages) {
		ret = -ENOMEM;
		goto fail_allocate_pages;
	}

	etnaviv_obj->vram_np = vram_np;

	gpu_paddr = etnaviv_obj_gpu_phys_addr(etnaviv_obj);
	for (i = 0; i < npages; ++i) {
		ppages[i] = pfn_to_page(__phys_to_pfn(gpu_paddr));
		gpu_paddr += PAGE_SIZE;
	}

	etnaviv_obj->pages = ppages;

	return 0;

fail_allocate_pages:
	drm_mm_remove_node(vram_np);
fail_allocate_range:
	kfree(vram_np);

	return ret;
}

static void *etnaviv_gem_vram_vmap(struct etnaviv_gem_object *etnaviv_obj)
{
	struct drm_device *drm = etnaviv_obj->base.dev;
	u64 offset;
	u64 size;
	int ret;

	lockdep_assert_held(&etnaviv_obj->lock);

	/*
	 * This is equivalent to pin, we need the dedicated vram node
	 * inserted into the whole drm_mm, to get a valid offset.
	 */
	if (!etnaviv_obj->pages) {
		ret = etnaviv_obj->ops->get_pages(etnaviv_obj);
		if (ret) {
			drm_err(drm, "Failed to pin %p\n", etnaviv_obj);
			return NULL;
		}
	}

	offset = etnaviv_obj_cpu_phys_addr(etnaviv_obj);
	size = etnaviv_obj->base.size;

	drm_dbg(drm, "offset : 0x%llx, %llu\n", offset, size >> 10);

	if (etnaviv_obj->flags & ETNA_BO_WC)
		return ioremap_wc(offset, size);

	return ioremap(offset, size);
}

static void etnaviv_gem_vram_vunmap(struct etnaviv_gem_object *etnaviv_obj)
{
	lockdep_assert_held(&etnaviv_obj->lock);

	iounmap(etnaviv_obj->vaddr);
}

static void etnaviv_gem_vram_release(struct etnaviv_gem_object *etnaviv_obj)
{
	if (etnaviv_obj->sgt) {
		sg_free_table(etnaviv_obj->sgt);
		kfree(etnaviv_obj->sgt);
		etnaviv_obj->sgt = NULL;
	}

	if (etnaviv_obj->pages) {
		kvfree(etnaviv_obj->pages);
		etnaviv_obj->pages = NULL;
	}

	if (etnaviv_obj->vram_np) {
		drm_mm_remove_node(etnaviv_obj->vram_np);
		kfree(etnaviv_obj->vram_np);
		etnaviv_obj->vram_np = NULL;
	}
}

static int etnaviv_gem_vram_mmap(struct etnaviv_gem_object *etnaviv_obj,
				 struct vm_area_struct *vma)
{
	pgprot_t vm_page_prot;

	vm_flags_set(vma, VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP);

	vm_page_prot = vm_get_page_prot(vma->vm_flags);

	if (etnaviv_obj->flags & ETNA_BO_WC)
		vma->vm_page_prot = pgprot_writecombine(vm_page_prot);
	else if (etnaviv_obj->flags & ETNA_BO_UNCACHED)
		vma->vm_page_prot = pgprot_noncached(vm_page_prot);
	else
		vma->vm_page_prot = vm_page_prot;

	return 0;
}

static const struct etnaviv_gem_ops etnaviv_gem_vram_ops = {
	.get_pages = etnaviv_gem_vram_get_pages,
	.release = etnaviv_gem_vram_release,
	.vmap = etnaviv_gem_vram_vmap,
	.vunmap = etnaviv_gem_vram_vunmap,
	.mmap = etnaviv_gem_vram_mmap,
};

const struct etnaviv_gem_ops *etnaviv_gem_get_vram_ops(void)
{
	return &etnaviv_gem_vram_ops;
}

int etnaviv_gem_new_vram(struct drm_device *dev, struct drm_file *file,
			 u32 size, u32 flags, u32 *handle)
{
	struct etnaviv_gem_object *etnaviv_obj;
	struct drm_gem_object *obj;
	int ret;

	size = PAGE_ALIGN(size);

	ret = etnaviv_gem_new_private(dev, size, flags, false,
				      &etnaviv_gem_vram_ops, &etnaviv_obj);
	if (ret)
		return ret;

	lockdep_set_class(&etnaviv_obj->lock, &etnaviv_vram_lock_class);

	obj = &etnaviv_obj->base;

	ret = drm_gem_handle_create(file, obj, handle);

	/* drop reference from allocate - handle holds it now */
	drm_gem_object_put(obj);

	return ret;
}

static int etnaviv_pci_init_dedicated_vram(struct device *dev,
					   struct etnaviv_drm_private *priv)
{
	struct pci_dev *gpu = to_pci_dev(dev);
	static const struct etnaviv_pci_gpu_data *pdata;
	resource_size_t base, size;
	u32 bar;

	pdata = etnaviv_pci_get_match_data(dev);
	if (!pdata)
		return -ENOENT;

	if (pdata->num_vram <= 0) {
		dev_err(dev, "Don't has a dedicated VRAM\n");
		return -ENODEV;
	}

	/* Using the first vram bar */
	bar = pdata->vram_bars[0];

	base = pci_resource_start(gpu, bar);
	size = pci_resource_len(gpu, bar);
	if (!base || !size)
		return -ENOSPC;

	priv->vram.gpu_base = pci_bus_address(gpu, bar);
	priv->vram.cpu_base = base;
	priv->vram.size = size;
	priv->dedicated_vram = true;

	dev_info(dev, "GPU Bar %u contains dedicated VRAM\n", bar);

	return 0;
}

int etnaviv_init_dedicated_vram(struct device *dev,
				struct etnaviv_drm_private *priv)
{
	int ret;

	if (dev->parent && dev_is_pci(dev->parent)) {
		ret = etnaviv_pci_init_dedicated_vram(dev->parent, priv);
		if (ret)
			return ret;
	}

	if (!priv->vram.size)
		return 0;

	/* CPU physical address */
	drm_mm_init(&priv->vram.mm, 0, priv->vram.size);

	dev_info(dev, "VRAM device address range: %08llx-%08llx\n",
		 priv->vram.gpu_base, priv->vram.gpu_base + priv->vram.size);
	dev_info(dev, "VRAM CPU physical address range: %08llx~%08llx\n",
		 priv->vram.cpu_base, priv->vram.cpu_base + priv->vram.size);

	return 0;
}
