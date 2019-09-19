/*
 * This file is part of the coreboot project.
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

#include <ec/google/chromeec/ec.h>
#include <baseboard/variants.h>
#include <boardid.h>
#include <cbfs.h>
#include <gpio.h>
#include <smbios.h>
#include <variant/gpio.h>
#include <device/mmio.h>
#include <device/pci.h>
#include <device/pci_ops.h>

uint32_t sku_id(void)
{
	static int sku = -1;

	if (sku == -1)
		sku = google_chromeec_get_sku_id();

	return sku;
}

uint8_t variant_board_sku(void)
{
	return sku_id();
}

void variant_mainboard_suspend_resume(void)
{
	/* Enable backlight - GPIO 85 active low */
	gpio_set(GPIO_85, 0); /*  APU_EDP_BL_DISABLE */
}



const char *smbios_mainboard_manufacturer(void)
{
	static char oem_bin_data[11];
	static const char *manuf;

	if (!CONFIG(USE_OEM_BIN))
		return CONFIG_MAINBOARD_SMBIOS_MANUFACTURER;

	if (manuf)
		return manuf;

	if (cbfs_boot_load_file("oem.bin", oem_bin_data,
					    sizeof(oem_bin_data) - 1,
					    CBFS_TYPE_RAW))
		manuf = &oem_bin_data[0];
	else
		manuf = CONFIG_MAINBOARD_SMBIOS_MANUFACTURER;

	return manuf;
}
