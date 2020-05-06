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

static const char *usb_acpi_name(const struct device *device)
{
	switch(device->device) {
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL20H_XHCI0:
		return "XHC0";
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI0:
		return "XHC0";
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI1:
		return "XHC1";
	default:
		return NULL;
	}
}

static void xhci_fill_ssdt_generator(struct device *device)
{
	printk(BIOS_INFO, "xHCI SSDT generation\n");
	switch (device->device) {
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL20H_XHCI0:
		/* Scope: \_SB.PCI0.PBRA.XHC0.RHUB */
		{
			char pscope[] = "\\_SB.PCI0.PBRA.XHC0.RHUB";
			acpigen_write_scope(pscope);

			acpigen_write_device("HS01");
			acpigen_write_name_byte("_ADR", 1);
			acpigen_pop_len();

			acpigen_write_device("HS02");
			acpigen_write_name_byte("_ADR", 2);
			acpigen_pop_len();

			acpigen_write_device("HS03");
			acpigen_write_name_byte("_ADR", 3);
			acpigen_pop_len();

			acpigen_write_device("HS04");
			acpigen_write_name_byte("_ADR", 4);
			acpigen_pop_len();

			acpigen_write_device("HS05");
			acpigen_write_name_byte("_ADR", 5);
			acpigen_pop_len();

			acpigen_write_device("HS06");
			acpigen_write_name_byte("_ADR", 6);
			acpigen_pop_len();

			acpigen_write_device("SS01");
			acpigen_write_name_byte("_ADR", 7);
			acpigen_pop_len();

			acpigen_write_device("SS02");
			acpigen_write_name_byte("_ADR", 8);
			acpigen_pop_len();

			acpigen_write_device("SS03");
			acpigen_write_name_byte("_ADR", 9);
			acpigen_pop_len();

			acpigen_write_device("SS04");
			acpigen_write_name_byte("_ADR", 0xa);
			acpigen_pop_len();

			acpigen_write_device("SS05");
			acpigen_write_name_byte("_ADR", 0xb);
			acpigen_pop_len();

			acpigen_pop_len(); // Exit scope
		}
		break;
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI0:
		/* Scope: \_SB_.PCI0.PBRA.XHC0.RHUB */
		{
			char pscope[] = "\\_SB.PCI0.PBRA.XHC0.RHUB";
			acpigen_write_scope(pscope);

			acpigen_write_device("HS01");
			acpigen_write_name_byte("_ADR", 1);
			acpigen_pop_len();

			acpigen_write_device("HS02");
			acpigen_write_name_byte("_ADR", 2);
			acpigen_pop_len();

			acpigen_write_device("HS03");
			acpigen_write_name_byte("_ADR", 3);
			acpigen_pop_len();

			acpigen_write_device("HS04");
			acpigen_write_name_byte("_ADR", 4);
			acpigen_pop_len();

			acpigen_write_device("SS01");
			acpigen_write_name_byte("_ADR", 5);
			acpigen_pop_len();

			acpigen_write_device("SS02");
			acpigen_write_name_byte("_ADR", 6);
			acpigen_pop_len();

			acpigen_write_device("SS03");
			acpigen_write_name_byte("_ADR", 7);
			acpigen_pop_len();

			acpigen_write_device("SS04");
			acpigen_write_name_byte("_ADR", 8);
			acpigen_pop_len();

			acpigen_pop_len(); // Exit scope
		}
		break;
	case PCI_DEVICE_ID_AMD_FAM17H_MODEL18H_XHCI1:
		//TODO (pshoroff): Add XHC1 generation in separate patch
		printk(BIOS_INFO,
		       "xHCI SSDT generation: attempted generation for device:0x%04x\n",
			device->device);
		break;
	}
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
