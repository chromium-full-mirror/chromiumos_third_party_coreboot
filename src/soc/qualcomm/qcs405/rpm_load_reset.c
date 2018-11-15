/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018, The Linux Foundation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <device/mmio.h>
#include <string.h>
#include <arch/cache.h>
#include <cbfs.h>
#include <halt.h>
#include <console/console.h>
#include <timestamp.h>
#include <soc/mmu.h>
#include <soc/rpm.h>
#include <soc/clock.h>

#define GCC_APSS_MISC		0x1860000
#define RPM_RESET		0

static void clock_reset_rpm(void)
{
	/* Bring RPM out of RESET */
	clrbits_le32((void *)(GCC_APSS_MISC), BIT(RPM_RESET));
}

void rpm_fw_load_reset(void)
{
	bool rpm_fw_entry;

	struct prog rpm_fw_prog =
		PROG_INIT(PROG_PAYLOAD, CONFIG_CBFS_PREFIX "/rpm");

	if (prog_locate(&rpm_fw_prog))
		die("SOC image: RPM_FW not found");

	rpm_fw_entry = selfload(&rpm_fw_prog);
	if (!rpm_fw_entry)
		die("SOC image: RPM load failed");

	clock_reset_rpm();
}
