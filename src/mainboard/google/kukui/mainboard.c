/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 MediaTek Inc.
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

#include <bootmode.h>
#include <console/console.h>
#include <delay.h>
#include <device/device.h>
#include <soc/bl31_plat_params.h>
#include <edid.h>
#include <gpio.h>
#include <soc/auxadc.h>
#include <soc/ddp.h>
#include <soc/dsi.h>
#include <soc/gpio.h>
#include <soc/mmu_operations.h>
#include <soc/mtcmos.h>
#include <soc/usb.h>

#include "display.h"
#include "gpio.h"

static void configure_emmc(void)
{
	const gpio_t emmc_pin[] = {
		GPIO(MSDC0_DAT0), GPIO(MSDC0_DAT1),
		GPIO(MSDC0_DAT2), GPIO(MSDC0_DAT3),
		GPIO(MSDC0_DAT4), GPIO(MSDC0_DAT5),
		GPIO(MSDC0_DAT6), GPIO(MSDC0_DAT7),
		GPIO(MSDC0_CMD), GPIO(MSDC0_RSTB),
	};

	for (size_t i = 0; i < ARRAY_SIZE(emmc_pin); i++)
		gpio_set_pull(emmc_pin[i], GPIO_PULL_ENABLE, GPIO_PULL_UP);
}

static void configure_usb(void)
{
	setup_usb_host();
}

static void register_reset_to_bl31(void)
{
	static struct bl31_gpio_param param_reset = {
		.h = {
			.type = PARAM_RESET,
		},
		.gpio = {
			.polarity = BL31_GPIO_LEVEL_HIGH,
		},
	};

	param_reset.gpio.index = GPIO_RESET.id;
	register_bl31_param(&param_reset.h);
}


static void mainboard_init(struct device *dev)
{
	static enum panel_id panel_id = PANEL_UNKNOWN;
	if (display_init_required()) {
		printk(BIOS_INFO, "Starting display init.\n");

		panel_id = get_panel_id();
		if (panel_id == PANEL_UNKNOWN)
			printk(BIOS_INFO, "Detect wrong panel.Skipping display init.\n");
		else {
			mtcmos_display_power_on();
			mtcmos_protect_display_bus();
			configure_backlight(panel_id);
			configure_display(panel_id);
			display_startup(panel_id);
		}
	} else
		printk(BIOS_INFO, "Skipping display init.\n");

	configure_emmc();
	configure_usb();

	register_reset_to_bl31();
}

static void mainboard_enable(struct device *dev)
{
	dev->ops->init = &mainboard_init;
}

struct chip_operations mainboard_ops = {
	.name = CONFIG_MAINBOARD_PART_NUMBER,
	.enable_dev = mainboard_enable,
};
