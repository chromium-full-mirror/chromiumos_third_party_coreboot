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

#include <baseboard/variants.h>
#include <soc/gpio.h>
#include <soc/romstage.h>

void __weak variant_romstage_entry(int s3_resume)
{
	/* By default, don't do anything */
}

void mainboard_romstage_early_init(void)
{
	int s3_resume = 0; //TODO: Handle S3
	size_t num_gpios;
	const struct soc_amd_gpio *gpios;

	gpios = variant_romstage_gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);

	variant_romstage_entry(s3_resume);
}

void mainboard_fsp_memory_init_params_cb(FSP_M_CONFIG *mcfg, uint32_t version)
{
}
