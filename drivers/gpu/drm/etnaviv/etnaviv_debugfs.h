/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ETNAVIV_DEBUGFS_H__
#define __ETNAVIV_DEBUGFS_H__

#include "etnaviv_drv.h"
#include "etnaviv_gem.h"

void etnaviv_sanity_test_vram_impl(struct drm_device *ddev,
				   struct drm_printer *p);

void etnaviv_sanity_test_shmem_impl(struct drm_device *ddev,
				    struct drm_printer *p);

#endif
