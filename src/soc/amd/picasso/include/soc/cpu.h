/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
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

#define MSR_CSTATE_ADDRESS 0xC0010073
#define CSTATE_IO_BASE_ADDRESS 0x413

#define DALI_3250U_STR "AMD Ryzen 3 3250U"

void picasso_init_cpus(struct device *dev);
int get_cpu_count(void);
void check_mca(void);

#define ROMSTAGE_TOP (CONFIG_X86_RESET_VECTOR + 0x10)
#define ROMSTAGE_SIZE (ROMSTAGE_TOP - CONFIG_ROMSTAGE_ADDR)
#if ROMSTAGE_TOP & 0xffff
# error Top of the BIOS binary image must be 64KB aligned
#endif

#define EARLY_DRAM_MTRR_BASE ALIGN_DOWN(CONFIG_ROMSTAGE_ADDR, 32 * MiB)
#define EARLY_DRAM_MTRR_SIZE ALIGN_UP(ROMSTAGE_SIZE, 32 * MiB)
#define EARLY_DRAM_MTRR_TOP ROMSTAGE_TOP

#define EARLY_RAMSTAGE_MTRR_SZ (32 * MiB + CONFIG_SMM_TSEG_SIZE)

#define PICASSO_CPUID			0x00810f81
#define RAVEN2_CPUID			0x00820f01

#define PICASSO_VBIOS_VID_DID		0x100215d8
#define PICASSO_VBIOS_REV		0xc1
#define DALI_VBIOS_VID_DID		0x100215dd  /* Match value in rom */
#define DALI_VBIOS_REV			0xc4

#endif /* __PICASSO_CPU_H__ */
