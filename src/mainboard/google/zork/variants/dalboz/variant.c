/*
 * This file is part of the coreboot project.
 *
 * Copyright 2020 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <baseboard/variants.h>
#include <soc/pci_devs.h>
#include <ec/google/chromeec/ec.h>

void variant_devtree_update(void)
{
	uint32_t sku_id;
	struct device *ssd_host;
	ssd_host = pcidev_path_on_root(SATA_DEVFN);

	sku_id = get_board_sku();
	/* Only SKU 3 has not SSD, hence disable it. */
	if (sku_id == 0x5A80000C) {
		if (ssd_host == NULL)
			return;
		ssd_host->enabled = 0;
	}
}
