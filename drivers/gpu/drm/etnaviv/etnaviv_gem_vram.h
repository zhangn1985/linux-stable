/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ETNAVIV_GEM_VRAM_H__
#define __ETNAVIV_GEM_VRAM_H__

const struct etnaviv_gem_ops *etnaviv_gem_get_vram_ops(void);

int etnaviv_gem_new_vram(struct drm_device *dev, struct drm_file *file,
			 u32 size, u32 flags, u32 *handle);

int etnaviv_init_dedicated_vram(struct device *dev,
				struct etnaviv_drm_private *priv);

#endif
