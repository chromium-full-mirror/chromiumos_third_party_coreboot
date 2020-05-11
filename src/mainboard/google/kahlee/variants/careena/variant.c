/*
 * This file is part of the coreboot project.
 *
 * Copyright 2020 Google LLC
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
#include <variant/sku.h>
#include <gpio.h>
#include <variant/gpio.h>

static const struct soc_amd_gpio variant_gpio_wlan_rst_early_reset[] = {
	/* GPIO_70 - WLAN_PE_RST_L */
	PAD_GPO(GPIO_70, LOW),
};

const struct soc_amd_gpio *variant_wlan_rst_early_gpio_table(size_t *size)
{
	*size = ARRAY_SIZE(variant_gpio_wlan_rst_early_reset);
	return variant_gpio_wlan_rst_early_reset;
}

void variant_romstage_entry(int s3_resume)
{
	uint32_t sku = google_chromeec_get_sku_id();
	uint32_t bid;

	if (!s3_resume) {
		/* Based on SKU, turn on keyboard backlight */
		switch (sku) {
		default:
			google_chromeec_kbbacklight(75);
			break;
		case SKU_CAREENA_KB_BACKLIGHT18:
		case SKU_CAREENA_KB_BACKLIGHT19:
		case SKU_CAREENA_KB_BACKLIGHT22:
		case SKU_CAREENA_KB_BACKLIGHT23:
			google_chromeec_kbbacklight(10);
			break;
		}
	}

	google_chromeec_get_board_version(&bid);

	if (bid == 7)
		/*
		 * Config WLAN RST - GPIO70 PU: release RST
		 * From RTK RTL8822CE spec, WLAN RST needs to be active
		 * at least 50 ms since WLAN power on
		 */
		gpio_set(GPIO_70, 1);
}
