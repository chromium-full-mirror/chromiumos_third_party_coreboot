/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
 * (Written by Hugh Edward Richard <hugh.e.dick@silverbackltd.com> for AMD Inc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <amdblocks/acpimmio.h>
#include <amdblocks/lpc.h>
#include <arch/ioapic.h>
#include <cbmem.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pnp.h>
#include <soc/acpi.h>
#include <soc/espi.h>
#include <soc/iomap.h>
#include <soc/southbridge.h>


static void d14_f3_read_resources(struct device *dev)
{
	/* Vague name: this reads resources of the d14f3 bridge, not LPC bus. */
	lpc_read_resources(dev);
}

static void d14_f3_set_resources(struct device *dev)
{
	/* Vague name: this sets resources of the d14f3 bridge, not LPC bus. */
	lpc_set_resources(dev);
}

static void d14_f3_child_bus_enable(struct device *dev)
{
	if (dev->ops && dev->ops->scan_bus)
		dev->ops->scan_bus(dev);
}

/*
 * eSPI bus should be under 'device generic a0.1'
 *
 * Due to limitations in the devicetree.cb format, we cannot specify more than a
 * bus behind a device in a simple and clean manner. The workaround is to treat
 * children of generic device a0.1 as the espi devices. 'a0' has no special
 * meaning to the correct operation of devicetree. It was chosen simply for the
 * reason that 0xa0 is the SPI/ESPI base address register.
 */
static bool is_espi_bus(struct device *dev)
{
	if (dev->path.type != DEVICE_PATH_GENERIC)
		return false;

	return dev->path.generic.id == 0xa0 && dev->path.generic.subid == 1;
}

/* Similarly, LPC would be under 'device generic 0.0' */
static bool is_lpc_bus(struct device *dev)
{
	if (dev->path.type != DEVICE_PATH_GENERIC)
		return false;

	return dev->path.generic.id == 0 && dev->path.generic.subid == 0;
}

static struct device_operations espi_bus_ops = {
	.read_resources		= DEVICE_NOOP,
	.set_resources		= DEVICE_NOOP,
	.enable_resources	= espi_enable_children_resources,
	.scan_bus		= scan_generic_bus,
	.enable			= d14_f3_child_bus_enable,
};

static struct device_operations lpc_bus_ops = {
	.read_resources		= DEVICE_NOOP,
	.set_resources		= DEVICE_NOOP,
	.enable_resources	= lpc_enable_childrens_resources,
	.init			= lpc_init,
	.scan_bus		= scan_lpc_bus,
	.enable			= d14_f3_child_bus_enable,
};

static void find_special_bus_devices(struct device *bus)
{
	struct device *child;
	struct bus *link;

	for (link = bus->link_list; link; link = link->next) {
		for (child = link->children; child; child = child->sibling) {

			if (is_espi_bus(child))
				child->ops = &espi_bus_ops;

			if (is_lpc_bus(child))
				child->ops = &lpc_bus_ops;
		}
	}
}

static void d14_f3_scan_buses(struct device *bus)
{
	printk(BIOS_SPEW, "%s for %s\n", __func__, dev_path(bus));

	find_special_bus_devices(bus);
	scan_generic_bus(bus);

	printk(BIOS_SPEW, "%s for %s done\n", __func__, dev_path(bus));
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations d14_f3_bridge_ops = {
	.read_resources = d14_f3_read_resources,
	.set_resources = d14_f3_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.acpi_inject_dsdt_generator = southbridge_inject_dsdt,
	.write_acpi_tables = southbridge_write_acpi_tables,
	.init = lpc_init,
	.scan_bus = d14_f3_scan_buses,
	.ops_pci = &lops_pci,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_AMD_PCO_LPC,
	0
};

static const struct pci_driver d14_f3_bridge_driver __pci_driver = {
	.ops = &d14_f3_bridge_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.devices = pci_device_ids,
};
