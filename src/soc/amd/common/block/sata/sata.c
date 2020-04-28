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

#include <arch/acpigen.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <amdblocks/sata.h>

void __weak soc_enable_sata_features(struct device *dev) { }

static const char *sata_acpi_name(const struct device *dev)
{
	return "STCR";
}

static struct device_operations sata_ops = {
	.read_resources		= pci_dev_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.init			= soc_enable_sata_features,
	.ops_pci		= &pci_dev_ops_pci,
	.acpi_name		= sata_acpi_name,
	.acpi_fill_ssdt_generator	= acpi_device_write_pci_dev,
};

static const unsigned short pci_device_ids[] = {
	PCI_DEVICE_ID_AMD_CZ_SATA,
	PCI_DEVICE_ID_AMD_CZ_SATA_AHCI,
	PCI_DEVICE_ID_AMD_FAM17H_SATA_AHCI_VER0,
	PCI_DEVICE_ID_AMD_FAM17H_SATA_AHCI_VER1,
	PCI_DEVICE_ID_AMD_FAM17H_SATA_AHCI_RAID_VER0,
	PCI_DEVICE_ID_AMD_FAM17H_SATA_AHCI_RAID_VER1,
	0
};

static const struct pci_driver sata0_driver __pci_driver = {
	.ops			= &sata_ops,
	.vendor			= PCI_VENDOR_ID_AMD,
	.devices		= pci_device_ids,
};
