/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PICASSO_CPU_H__
#define __PICASSO_CPU_H__

#include <device/device.h>
#include <commonlib/helpers.h>

void picasso_init_cpus(struct device *dev);
int get_cpu_count(void);
void check_mca(void);

#define ROMSTAGE_SIZE (64 * KiB) /* Enforced by Makefile.inc for PSP info */
#define ROMSTAGE_TOP (CONFIG_ROMSTAGE_ADDR + ROMSTAGE_SIZE)
#if ROMSTAGE_TOP & 0xffff
# error Top of the BIOS binary image must be 64KB aligned
#endif

#endif /* __PICASSO_CPU_H__ */
