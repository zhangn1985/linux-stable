// SPDX-License-Identifier: GPL-2.0

#include <drm/drm_debugfs.h>

#include "etnaviv_debugfs.h"
#include "etnaviv_drv.h"
#include "etnaviv_gem.h"
#include "etnaviv_gem_vram.h"

static void bo_test_write_to_vram_by_cpu(void *addr, unsigned int num)
{
	u32 val = 0;

	while (num--) {
		writel(val, addr);
		++val;
		addr += 4;
	}
}

static unsigned int bo_test_read_from_vram_by_cpu(void *addr, unsigned int num)
{
	unsigned int i = 0;

	while (i < num) {
		u32 val = readl(addr);

		if (val != i)
			return i;

		addr += 4;
		++i;
	}

	return 0;
}

void etnaviv_sanity_test_vram_impl(struct drm_device *drm, struct drm_printer *p)
{
	struct etnaviv_gem_object *etnaviv_obj;
	unsigned int size = 1920 * 1080 * 4;
	void *addr;
	int ret;

	size = ALIGN(size, PAGE_SIZE);

	drm_printf(p, "Test Write to VRAM %u bytes\n", size);

	ret = etnaviv_gem_new_private(drm, size, ETNA_BO_UNCACHED, false,
				      etnaviv_gem_get_vram_ops(),
				      &etnaviv_obj);
	if (ret) {
		drm_printf(p, "create dst bo failed\n");
		return;
	}

	addr = etnaviv_gem_vmap(&etnaviv_obj->base);
	if (!addr) {
		drm_printf(p, "write to vram by cpu failed: vmap\n");
		goto out;
	}

	etnaviv_gem_vunmap(&etnaviv_obj->base);

	addr = etnaviv_gem_vmap(&etnaviv_obj->base);

	bo_test_write_to_vram_by_cpu(addr, size / 4);

	ret = bo_test_read_from_vram_by_cpu(addr, size / 4);

	drm_printf(p, "Write to VRAM %s: %u bytes\n",
		   ret ? "not pass" : "Passed", ret ? ret : size);

	etnaviv_gem_vunmap(&etnaviv_obj->base);
out:
	drm_gem_object_put(&etnaviv_obj->base);
}

void etnaviv_sanity_test_shmem_impl(struct drm_device *drm, struct drm_printer *p)
{
	struct etnaviv_gem_object *etnaviv_obj;
	unsigned int size = 1920 * 1080 * 4;
	void *addr;
	int ret;

	size = ALIGN(size, PAGE_SIZE);

	drm_printf(p, "Test Write to SHMEM %u bytes\n", size);

	ret = etnaviv_gem_new_private(drm, size, ETNA_BO_CACHED, true,
				      etnaviv_gem_get_shmem_ops(),
				      &etnaviv_obj);
	if (ret) {
		drm_printf(p, "create dst bo failed\n");
		return;
	}

	addr = etnaviv_gem_vmap(&etnaviv_obj->base);
	if (!addr) {
		drm_printf(p, "write to shmem by cpu failed: vmap\n");
		goto out;
	}

	etnaviv_gem_vunmap(&etnaviv_obj->base);

	addr = etnaviv_gem_vmap(&etnaviv_obj->base);

	bo_test_write_to_vram_by_cpu(addr, size / 4);

	ret = bo_test_read_from_vram_by_cpu(addr, size / 4);

	drm_printf(p, "Write to SHMEM %s: %u bytes\n",
		   ret ? "not pass" : "Passed", ret ? ret : size);

	etnaviv_gem_vunmap(&etnaviv_obj->base);
out:
	drm_gem_object_put(&etnaviv_obj->base);
}
