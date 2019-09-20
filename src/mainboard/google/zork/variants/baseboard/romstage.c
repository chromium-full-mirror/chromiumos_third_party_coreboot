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

#include <soc/gpio.h>
#include "gpio.h"

void variant_romstage_entry(int s3_resume)
{
	uint32_t sku = google_chromeec_get_sku_id();

	if (!s3_resume) {
		/* Based on SKU, turn on keyboard backlight */
		switch (sku) {
		default:
			// TODO: Add SKU Specific items here
			break;
		}
	}
}
