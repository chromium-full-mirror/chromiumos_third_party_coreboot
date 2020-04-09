/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <soc/platform_descriptors.h>

void soc_fill_pcie_descriptors(FSP_S_CONFIG *scfg,
			const picasso_fsp_pcie_descriptor *descs, size_t num)
{
	size_t i;
	picasso_fsp_pcie_descriptor *fsp_pcie;

	/* FIXME: this violates C rules. */
	fsp_pcie = (picasso_fsp_pcie_descriptor *)(scfg->dxio_descriptor0);

	for (i = 0; i < num; i++) {
		fsp_pcie[i] = descs[i];
	}
}

void soc_fill_ddi_descriptors(FSP_S_CONFIG *scfg,
			const picasso_fsp_ddi_descriptor *descs, size_t num)
{
	size_t i;
	picasso_fsp_ddi_descriptor *fsp_ddi;

	/* FIXME: this violates C rules. */
	fsp_ddi = (picasso_fsp_ddi_descriptor *)&(scfg->ddi_descriptor0);

	for (i = 0; i < num; i++) {
		fsp_ddi[i] = descs[i];
	}
}
