/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
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

#include <acpi/acpigen.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pci_ehci.h>
#include <soc/acpi.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <amdblocks/acpimmio.h>

static void picasso_usb_init(struct device *dev)
{
	/* USB overcurrent configuration is programmed inside the FSP */

	printk(BIOS_DEBUG, "%s\n", __func__);
}

static const struct xhci_port_info {
	unsigned int did;
	const char *acpi_device_name;
	int hs_count;
	int ss_count;
} xhci_port_info[] = {
	{
		.did = PCI_DEVICE_ID_AMD_FAM17H_MODEL20H_XHCI0,
		.acpi_device_name = "XHC0",
		.hs_count = 6,
		.ss_count = 5,
	},
	{
		.did = PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI0,
		.acpi_device_name = "XHC0",
		.hs_count = 4,
		.ss_count = 4,
	},
	{
		.did = PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI1,
		.acpi_device_name = "XHC1",
		.hs_count = 2,
		.ss_count = 1,
	},
};

static const struct xhci_port_info *find_device_cfg(unsigned int did)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(xhci_port_info); i++) {
		if (xhci_port_info[i].did == did)
			return &xhci_port_info[i];
	}
	return NULL;
}

static const char *usb_acpi_name(const struct device *device)
{
	const struct xhci_port_info *pi = find_device_cfg(device->device);
	if (pi != NULL)
		return pi->acpi_device_name;
	else
		return NULL;
}

static void xhci_add_devices(const struct xhci_port_info *controller)
{
	int i;
	int addr = 1;
	char buf[16];

	acpigen_write_device("RHUB");
	acpigen_write_name_integer("_ADR", 0x00000000);

	/* Write HS devices */
	for (i = 1; i <= controller->hs_count; i++){
		snprintf(buf, sizeof(buf), "HS%02d", i);
		acpigen_write_device(buf);
		acpigen_write_name_byte("_ADR", addr);
		acpigen_pop_len();
		addr++;
	}

	/* Write SS devices */
	for (i = 1; i <= controller->ss_count; i++){
		snprintf(buf, sizeof(buf), "SS%02d", i);
		acpigen_write_device(buf);
		acpigen_write_name_byte("_ADR", addr);
		acpigen_pop_len();
		addr++;
	}

	/* Exit RHUB device */
	acpigen_pop_len();
}

static void xhci_fill_ssdt_generator(struct device *device)
{
	printk(BIOS_INFO, "xHCI SSDT generation\n");

	const struct xhci_port_info *pi = find_device_cfg(device->device);

	if (pi == NULL) {
		printk(BIOS_ERR, "Unsupported xHCI device: VendorID:0x%0x4 DeviceID:0x%04x\n",
		       device->vendor, device->device);
		return;
	}

	/* Write SSDT entry for xHCI controller */
	acpigen_write_scope(acpi_device_scope(device));
	acpigen_write_device(pi->acpi_device_name);
	acpigen_write_ADR_pci_device(device);
	acpigen_write_PRW(0xb, 3);

	xhci_add_devices(pi);

        // Method(_S0W,0) {
        // 	Return(0)
        // }
	acpigen_write_name_integer("_S0W", 0);

        // Method(_S3W,0) {
        // 	Return(4)
        // }
	acpigen_write_name_integer("_S3W", 4);

        // Method(_S4W,0) {
        // 	Return(4)
        // }
	acpigen_write_name_integer("_S4W", 4);

	acpigen_pop_len(); // xHCI device

	acpigen_pop_len(); // PRBA scope
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations usb_ops = {
	.read_resources = pci_dev_read_resources,
	.set_resources = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init = picasso_usb_init,
	.scan_bus = scan_static_bus,
	.acpi_name = usb_acpi_name,
	.ops_pci = &lops_pci,
	.acpi_fill_ssdt_generator = xhci_fill_ssdt_generator,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI0,
	PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI1,
	PCI_DEVICE_ID_AMD_FAM17H_MODEL20H_XHCI0,
	0
};

static const struct pci_driver usb_0_driver __pci_driver = {
	.ops = &usb_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.devices = pci_device_ids,
};
