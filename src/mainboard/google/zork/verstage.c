/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <amdblocks/gpio_banks.h>
#include <baseboard/espi.h>
#include <baseboard/variants.h>
#include <console/console.h>
#include <security/vboot/vboot_common.h>
#include <soc/southbridge.h>

static void setup_gpio(void)
{
	const struct soc_amd_gpio *gpios;
	size_t num_gpios;

	printk(BIOS_DEBUG, "Setting GPIOs\n");
	gpios = variant_romstage_gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);
	printk(BIOS_DEBUG, "GPIOs setup\n");
}

static void setup_espi(void)
{
	printk(BIOS_DEBUG, "Setting up eSPI\n");
	enable_espi_early();
	printk(BIOS_DEBUG, "eSPI setup\n");
}

static void setup_i2c(void)
{
	printk(BIOS_DEBUG, "Setting up i2c\n");
	i2c_soc_early_init();
	printk(BIOS_DEBUG, "i2c setup\n");
}

void verstage_mainboard_init(void)
{
	setup_gpio();

	setup_espi();

	setup_i2c();
}
