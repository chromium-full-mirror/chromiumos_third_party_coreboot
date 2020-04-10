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


#ifndef __BASEBOARD_VARIANTS_H__
#define __BASEBOARD_VARIANTS_H__

#include <stddef.h>
#include <soc/platform_descriptors.h>

const struct sci_source *get_gpe_table(size_t *num);
uint8_t variant_memory_sku(void);
uint32_t variant_board_sku(void);
const struct soc_amd_gpio *variant_early_gpio_table(size_t *size);
const struct soc_amd_gpio *variant_romstage_gpio_table(size_t *size);
const struct soc_amd_gpio *variant_wifi_romstage_gpio_table(size_t *size);
const struct soc_amd_gpio *variant_gpio_table(size_t *size);
void variant_romstage_entry(int s3_resume);
void variant_mainboard_suspend_resume(void);
int variant_has_emmc(uint8_t sku);
/* Return board SKU */
uint32_t get_board_sku(void);
/* Modify devictree settings during ramstage. */
void variant_devtree_update(void);
void mainboard_fsp_silicon_update_params_pci_ddi(FSP_S_CONFIG *scfg);

/* Per variant FSP-S initialization, default implementation in baseboard and
 * overrideable by the variant. */
void variant_get_pcie_ddi_descriptors(
		const picasso_fsp_pcie_descriptor **pcie_descs, size_t *pcie_num,
		const picasso_fsp_ddi_descriptor **ddi_descs, size_t *ddi_num);

/* Provide the descriptors for the associated baseboard for the variant. These functions
 * can be used for obtaining the baseboard's descriptors if the variant followed the
 * baseboard. */
const picasso_fsp_pcie_descriptor *baseboard_get_pcie_descriptors(size_t *num);
const picasso_fsp_ddi_descriptor *baseboard_get_ddi_descriptors(size_t *num);

#endif /* __BASEBOARD_VARIANTS_H__ */
