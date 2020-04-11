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
#include <console/console.h>
#include <device/device.h>
#include <soc/pci_devs.h>
#include <ec/google/chromeec/ec.h>

static int sku_has_emmc(void)
{
	uint32_t board_sku = sku_id();

	/* FIXME: This needs to be fw_config controlled. */
	/* Enable emmc0 for unknown skus. Only sku3/0xC really has it. */
	if (board_sku == 0x5A80000C || board_sku == 0x5A800003 ||
	    board_sku == CROS_SKU_UNKNOWN)
		return 1;

	return 0;
}

void variant_devtree_update(void)
{
	struct device *ssd_host;
	struct soc_amd_picasso_config *cfg;

	ssd_host = pcidev_path_on_root(SATA_DEVFN);
	cfg = config_of(ssd_host);

	if (sku_has_emmc())
		ssd_host->enabled = 0;
	else
		cfg->sd_emmc_config = SD_EMMC_DISABLE;
}

/* FIXME: Comments seem to suggest these are not entirely correct. */
static const picasso_fsp_ddi_descriptor non_hdmi_ddi_descriptors[] =
{
	{ // DDI0, DP0, eDP
		.connector_type = EDP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1, DP1, DB OPT2 USB-C1 / DB OPT3 MST hub
		.connector_type = DP,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	// DP2 pins not connected on Dali
	{ // DDI2, DP3, USB-C0
		.connector_type = DP,
		.aux_index = AUX4,
		.hdp_index = HDP4,
	}
};

static const picasso_fsp_ddi_descriptor hdmi_ddi_descriptors[] = {
	{ // DDI0, DP0, eDP
		.connector_type = EDP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1, DP1, DB OPT2 USB-C1 / DB OPT3 MST hub
		.connector_type = HDMI,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	// DP2 pins not connected on Dali
	{ // DDI2, DP3, USB-C0
		.connector_type = DP,
		.aux_index = AUX4,
		.hdp_index = HDP4,
	}
};

void variant_get_pcie_ddi_descriptors(
		const picasso_fsp_pcie_descriptor **pcie_descs, size_t *pcie_num,
		const picasso_fsp_ddi_descriptor **ddi_descs, size_t *ddi_num)
{
	uint32_t board_sku = sku_id();

	*pcie_descs = baseboard_get_pcie_descriptors(pcie_num);

	/* SKU 1, A, and D DB have HDMI, as well as unknown */
	/* FIXME: this needs to be fw_config controlled. */
	if ((board_sku == 0x5A80000A) || (board_sku == 0x5A80000D) ||
	    (board_sku == 0x5A800001) || (board_sku == CROS_SKU_UNKNOWN)) {
		*ddi_descs = &hdmi_ddi_descriptors[0];
		*ddi_num = ARRAY_SIZE(hdmi_ddi_descriptors);
	} else {
		*ddi_descs = &non_hdmi_ddi_descriptors[0];
		*ddi_num = ARRAY_SIZE(non_hdmi_ddi_descriptors);
	}
}
