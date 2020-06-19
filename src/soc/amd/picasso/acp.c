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

#include <acpi/acpi_device.h>
#include <acpi/acpigen.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include "chip.h"
#include <soc/acpi.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <amdblocks/acpimmio.h>
#include <commonlib/helpers.h>

static void acp_init(struct device *dev)
{
	const struct soc_amd_picasso_config *cfg;
	const struct device *nb_dev = pcidev_path_on_root(GNB_DEVFN);
	struct resource *res;
	uintptr_t bar;

	/* Set the proper I2S_PIN_CONFIG state */
	if (!nb_dev || !nb_dev->chip_info)
		return;

	cfg = nb_dev->chip_info;

	res = dev->resource_list;
	if (!res || !res->base) {
		printk(BIOS_ERR, "Error, unable to configure pin in %s\n", __func__);
		return;
	}

	bar = (uintptr_t)res->base;
	write32((void *)(bar + ACP_I2S_PIN_CONFIG), cfg->acp_pin_cfg);

	/* Enable ACP_PME_EN and ACP_I2S_WAKE_EN for I2S_WAKE event*/
	write32((void *)(bar + ACP_I2S_WAKE_EN), 1);
	write32((void *)(bar + ACP_PME_EN), 1);


	if (cfg->acp_pin_cfg == I2S_PINS_I2S_TDM)
		sb_clk_output_48Mhz(); /* Internal connection to I2S */

}

static const char *acp_acpi_name(const struct device *dev)
{
	return "ACPD";
}

#define AMD_I2S_ACPI_NAME	"I2SM"
#define AMD_I2S_ACPI_HID	"AMDI5682"
#define AMD_I2S_ACPI_DESC	"I2S machine driver"

static void acp_fill_i2s_machine_dev(const struct device *dev)
{
	const char *scope = acpi_device_path(dev);
	const struct soc_amd_picasso_config *cfg = config_of_soc();
	const struct acpi_gpio *dmic_select_gpio = &cfg->dmic_select_gpio;
	struct acpi_dp *dsd;

	if (dmic_select_gpio->pin_count == 0)
		return;

	acpigen_write_scope(scope); /* Scope */
	acpigen_write_device(AMD_I2S_ACPI_NAME); /* Device */
	acpigen_write_name_string("_HID", AMD_I2S_ACPI_HID);
	acpigen_write_name_integer("_UID", 1);
	acpigen_write_name_string("_DDN", AMD_I2S_ACPI_DESC);

	acpigen_write_STA(ACPI_STATUS_DEVICE_ALL_ON);

	/* Resources */
	acpigen_write_name("_CRS");
	acpigen_write_resourcetemplate_header();
	acpi_device_write_gpio(dmic_select_gpio);
	acpigen_write_resourcetemplate_footer();

	dsd = acpi_dp_new_table("_DSD");
	/*
	 * This GPIO is used to select DMIC0 or DMIC1 by the kernel driver. It does not
	 * really have a polarity since low and high control the selection of DMIC and
	 * hence does not have an active polarity.
	 * Kernel driver does not use the polarity field and instead treats the GPIO
	 * selection as follows:
	 * Set low (0) = Select DMIC0
	 * Set high (1) = Select DMIC1
	 */
	acpi_dp_add_gpio(dsd, "dmic-gpios", acpi_device_path_join(dev, AMD_I2S_ACPI_NAME),
			 0, /* Index = 0 (There is a single GPIO entry in _CRS). */
			 0, /* Pin = 0 (There is a single pin in the GPIO resource). */
			 0);/* Active low = 0 (Kernel driver does not use active polarity). */
	acpi_dp_write(dsd);

	acpigen_pop_len(); /* Device */
	acpigen_pop_len(); /* Scope */
}

static void acp_fill_ssdt(struct device *dev)
{
	acpi_device_write_pci_dev(dev);
	acp_fill_i2s_machine_dev(dev);
}

static struct pci_operations lops_pci = {
	.set_subsystem = pci_dev_set_subsystem,
};

static struct device_operations acp_ops = {
	.read_resources = pci_dev_read_resources,
	.set_resources = pci_dev_set_resources,
	.enable_resources = pci_dev_enable_resources,
	.init = acp_init,
	.ops_pci = &lops_pci,
	.acpi_name = acp_acpi_name,
	.acpi_fill_ssdt_generator = acp_fill_ssdt,
};

static const struct pci_driver acp_driver __pci_driver = {
	.ops = &acp_ops,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = PCI_DEVICE_ID_AMD_FAM17H_ACP,
};
