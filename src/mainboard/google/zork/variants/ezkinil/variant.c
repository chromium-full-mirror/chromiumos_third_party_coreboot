/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <baseboard/variants.h>
#include <soc/pci_devs.h>
#include <ec/google/chromeec/ec.h>

static int sku_has_emmc(void)
{
	uint32_t board_sku = sku_id();

	if ((board_sku == 0x5A020001) || (board_sku == 0x5A020002) ||
	    (board_sku == 0x5A020005) || (board_sku == 0x5A020006) ||
	    (board_sku == 0x5A020009) || (board_sku == 0x5A02000A) ||
	    (board_sku == 0x5A02000D) || (board_sku == 0x5A02000E))
		return 1;

	return 0;
}

void variant_devtree_update(void)
{
	struct device *ssd_host;

	if (!sku_has_emmc())
		return;

	ssd_host = pcidev_path_on_root(SATA_DEVFN);

	if (ssd_host == NULL)
		return;

	ssd_host->enabled = 0;
}

void variant_update_fsps_params(FSP_S_CONFIG *scfg)
{
	if (sku_has_emmc()) {
		scfg->emmc0_mode = EMMC_HS400;
	} else {
		scfg->emmc0_mode = SD_DISABLE;
	}
}

