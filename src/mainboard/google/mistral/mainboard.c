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

#include <device/device.h>
#include <boot/coreboot_tables.h>
#include <bootblock_common.h>
#include <security/vboot/vboot_common.h>
#include <vendorcode/google/chromeos/chromeos.h>
#include <soc/usb.h>

static struct usb_board_data usb1_board_data = {
	.parameter_override_x0 = 0x63,
	.parameter_override_x1 = 0x03,
	.parameter_override_x0 = 0x1d,
	.parameter_override_x1 = 0x03,
};

static void setup_usb(void)
{
	/* Setting Secondary usb controller */
	setup_usb_host(HSUSB_HS_PORT_1, &usb1_board_data);
}

static void mainboard_init(struct device *dev)
{
	if (CONFIG(CHROMEOS)) {
		/* Copy WIFI calibration data into CBMEM. */
		cbmem_add_vpd_calibration_data();
	}

	setup_usb();
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->init = &mainboard_init;
}

struct chip_operations mainboard_ops = {
	.name = CONFIG_MAINBOARD_PART_NUMBER,
	.enable_dev = mainboard_enable,
};

void lb_board(struct lb_header *header)
{
	struct lb_modeflags {
		struct lb_range range;
		uint32_t reserved;
		uint32_t out_flags;
	} *entry;

	entry = (struct lb_modeflags *)lb_new_record(header);
	entry->range.tag = LB_TAG_VBOOT_HANDOFF;
	entry->range.size = sizeof(*entry);
	entry->range.range_start = (uintptr_t)&entry->reserved;
	entry->range.range_size = 2*sizeof(uint32_t);
	entry->reserved = 0;
	entry->out_flags = 0;
	if (vboot_developer_mode_enabled()) {
		entry->out_flags |= VB_INIT_OUT_ENABLE_DEVELOPER;
	}
}
