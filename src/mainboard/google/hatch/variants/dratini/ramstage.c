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

#include <delay.h>
#include <gpio.h>
#include <baseboard/variants.h>
#include <soc/gpio.h>
#include <variant/sku.h>
#include <ec/google/chromeec/ec.h>

void variant_ramstage_init(void)
{
	uint32_t sku_id = google_chromeec_get_board_sku();

	switch (sku_id) {
	case SKU_2_DRATINI:
	case SKU_4_DRATINI:
	case SKU_6_DRATINI:
	case SKU_8_DRATINI:
	case SKU_21_DRAGONAIR:
	case SKU_22_DRAGONAIR:
		/*
		* Enable power to FPMCU, wait for power rail to stabilize,
		* and then deassert FPMCU reset.
		* Waiting for the power rail to stabilize can take a while,
		* a minimum of 400us on Kohaku.
		*/
		gpio_output(GPP_C11, 1);
		mdelay(4);
		gpio_output(GPP_A12, 1);
		break;
	default:
		/* SKU does not have FP Sensor, do not enable FPMCU */
		break;
	}
}
