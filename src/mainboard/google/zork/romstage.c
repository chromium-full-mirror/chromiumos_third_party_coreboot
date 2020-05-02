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
#include <baseboard/espi.h>
#include <ec/ec.h>
#include <ec/google/chromeec/ec.h>
#include <soc/espi.h>
#include <soc/gpio.h>
#include <soc/romstage.h>
#include <variant/ec.h>
#include <console/console.h>

void __weak variant_romstage_entry()
{
	/* By default, don't do anything */
}

void romstage_mainboard_early_init(void)
{
	size_t num_gpios;
	const struct soc_amd_gpio *gpios;

	gpios = variant_romstage_gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);
	gpios = variant_wifi_romstage_gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);

	enable_espi_early();

	mainboard_ec_init();

	variant_romstage_entry();
}
