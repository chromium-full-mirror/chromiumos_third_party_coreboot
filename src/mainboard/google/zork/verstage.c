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

/* TODO(154636850): See if we can include device tree config */
static const struct soc_amd_picasso_config config = {
	/* Enable I2C3 for H1 400kHz */
	.i2c[3] = {
		.speed = I2C_SPEED_FAST,
		.early_init = true,
	}
};

const struct soc_amd_picasso_config *get_soc_config(void)
{
	return &config;
}

void verstage_mainboard_init(void)
{
	setup_gpio();

	setup_espi();

	setup_i2c();
}
