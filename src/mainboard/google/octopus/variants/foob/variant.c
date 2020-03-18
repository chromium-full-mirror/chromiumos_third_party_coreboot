/*
 * This file is part of the coreboot project.
 *
 * Copyright 2019 Google LLC
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

#include <baseboard/variants.h>
#include <soc/pci_devs.h>
#include <string.h>
#include <ec/google/chromeec/ec.h>
#include <sar.h>

#define SKU_UNKNOWN     0xFFFFFFFF

void variant_update_devtree(struct device *dev)
{
	uint32_t sku_id = SKU_UNKNOWN;
	struct device *touchscreen_i2c_host;

	touchscreen_i2c_host = dev_find_slot(0, PCH_DEVFN_I2C7);

	if (touchscreen_i2c_host == NULL)
		return;

	/* SKU ID 1 does not have a touchscreen device, hence disable it. */
	google_chromeec_cbi_get_sku_id(&sku_id);
	if (no_touchscreen_sku(sku_id))
		touchscreen_i2c_host->enabled = 0;
}

const char *get_wifi_sar_cbfs_filename(void)
{
	const char *filename = NULL;
	uint32_t sku_id = SKU_UNKNOWN;

	google_chromeec_cbi_get_sku_id(&sku_id);
	if (sku_id == 9)
		filename = "wifi_sar-foob360.hex";

	return filename;
}
