/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Google Inc.
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

#include <arch/acpi.h>
#include <cpu/x86/smm.h>
#include <ec/google/chromeec/smm.h>
#include <gpio.h>
#include <soc/smm.h>

#include "ec.h"
#include "gpio.h"

void mainboard_smi_espi_handler(void)
{
	chromeec_smi_process_events();
}

static void mainboard_gpio_smi_sleep(void)
{
	int i;

	const gpio_t active_high_signals[] = {
		GPIO_APEX_VCOM_EN,
		GPIO_APEX0_VR_EN,
		GPIO_APEX1_VR_EN,
	};

	for (i = 0; i < ARRAY_SIZE(active_high_signals); i++)
		gpio_set(active_high_signals[i], 0);

}

void mainboard_smi_sleep(u8 slp_typ)
{
	chromeec_smi_sleep(slp_typ, MAINBOARD_EC_S3_WAKE_EVENTS,
			MAINBOARD_EC_S5_WAKE_EVENTS);

	/* Power down Apex chips on S3-S5 sleep type. */
	if (slp_typ >= ACPI_S3)
		mainboard_gpio_smi_sleep();
}

int mainboard_smi_apmc(u8 apmc)
{
	chromeec_smi_apmc(apmc, MAINBOARD_EC_SCI_EVENTS, 0);
	return 0;
}
