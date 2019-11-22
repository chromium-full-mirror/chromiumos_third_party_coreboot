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

#include <stddef.h>
#include <soc/romstage.h>
#include <baseboard/variants.h>
#include <ec/google/chromeec/ec.h>

#include <gpio.h>
#include <soc/gpio.h>
#include <variant/gpio.h>

void variant_romstage_entry(int s3_resume)
{
	uint32_t sku = google_chromeec_get_sku_id();

	/* Power the wifi card */
	gpio_set(EN_PWR_WIFI, 1);

	if (!s3_resume) {
		/* Based on SKU, turn on keyboard backlight */
		switch (sku) {
		default:
			// TODO: Add SKU Specific items here
			break;
		}
	}
}

static const struct soc_amd_gpio morphius_gpio_set_wifi[] = {
	/* EN_PWR_WIFI - Power off. Pull high in romstage.c */
	PAD_GPO(GPIO_29, LOW),
};

const struct soc_amd_gpio *variant_wifi_romstage_gpio_table(size_t *size)
{
	*size = ARRAY_SIZE(morphius_gpio_set_wifi);
	return morphius_gpio_set_wifi;
}
