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
#include "chip.h"
#include <console/console.h>
#include <device/device.h>
#include <device/pnp.h>
#include <ec/acpi/ec.h>


static void man_ec_init(struct device *dev)
{
	if (!dev->enabled)
		return;

	printk(BIOS_SPEW, "Mandolin EC: %s\n", __func__);

	ec_set_ports(0x666, 0x662);
}

static void man_ec_read_resources(struct device *dev)
{
	struct resource * res;

	res = new_resource(dev, 0x60);
	res->base = 0x60;
	res->size = 1;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	res = new_resource(dev, 0x64);
	res->base = 0x64;
	res->size = 1;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;

	res = new_resource(dev, 0x62);
	res->base = 0x662;
	res->size = 8;
	res->flags = IORESOURCE_IO | IORESOURCE_ASSIGNED | IORESOURCE_FIXED;
}

static struct device_operations ops = {
	.init             = man_ec_init,
	.read_resources   = man_ec_read_resources,
	.enable_resources = DEVICE_NOOP,
	.set_resources    = DEVICE_NOOP
};

static struct pnp_info pnp_dev_info[] = {
	{ NULL, 0, 0, 0, }
};

static void man_ec_enable_dev(struct device *dev)
{
	printk(BIOS_SPEW, "Mandolin EC: %s for %s\n", __func__, dev_path(dev));
	pnp_enable_devices(dev, &ops, ARRAY_SIZE(pnp_dev_info), pnp_dev_info);
}

struct chip_operations ec_amd_mandolin_ec_ops = {
	CHIP_NAME("AMD Mandolin EC - MEC1701")
	.enable_dev = man_ec_enable_dev
};
