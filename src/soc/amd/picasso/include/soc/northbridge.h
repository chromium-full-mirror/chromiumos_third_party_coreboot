/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Advanced Micro Devices, Inc.
 * Copyright (C) 2015 Intel Corp.
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

#ifndef __PI_PICASSO_NORTHBRIDGE_H__
#define __PI_PICASSO_NORTHBRIDGE_H__

#include <device/device.h>
#include <types.h>

/* D18F0 - Fabric Configuration registers */
#define FABRIC_ID_IOMS0		9

#define D18F0_VGAEN		0x80
#define   VGA_ADDR_ENABLE	BIT(0)
#define   VGA_DST_ID_SH	4

#define D18F0_MMIO_BASE0	0x200
#define D18F0_MMIO_LIMIT0	0x204
#define   D18F0_MMIO_SHIFT	16
#define D18F0_MMIO_CTRL0	0x208
#define   MMIO_NP		BIT(12)
#define   MMIO_DST_ID_SH	4
#define   MMIO_DST_ID_MASK	(0xff << MMIO_DST_ID_SH)
#define   MMIO_WE		BIT(1)
#define   MMIO_RE		BIT(0)
#define NUM_NB_MMIO_REGS	8
#define NB_MMIO_BASE(reg)	((reg) * 4 * sizeof(uint32_t) + D18F0_MMIO_BASE0)
#define NB_MMIO_LIMIT(reg)	((reg) * 4 * sizeof(uint32_t) + D18F0_MMIO_LIMIT0)
#define NB_MMIO_CONTROL(reg)	((reg) * 4 * sizeof(uint32_t) + D18F0_MMIO_CTRL0)

/* Bus A D0F5 - Audio Processor */
#define D0F5_I2S_PIN_CONFIG	0x1400	/* HDA, Soundwire, I2S */

void amd_initcpuio(void);

void domain_enable_resources(struct device *dev);
void domain_set_resources(struct device *dev);
void northbridge_init(void);
void set_warm_reset_flag(void);
int is_warm_reset(void);

#endif /* __PI_PICASSO_NORTHBRIDGE_H__ */
