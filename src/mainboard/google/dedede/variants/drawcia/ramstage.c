/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootstate.h>
#include <pc80/mc146818rtc.h>
#include <variant/ec.h>
#include <baseboard/variants.h>

static struct acpi_gpio lte_reset_gpio = ACPI_GPIO_OUTPUT_ACTIVE_LOW(GPP_H0);
static struct acpi_gpio lte_enable_gpio = ACPI_GPIO_OUTPUT_ACTIVE_HIGH(GPP_A10);

void variant_devtree_update(void)
{
	update_lte_device(&lte_reset_gpio, &lte_enable_gpio);
}

static void cmos_reset_ec_trusted(void *unused)
{
	cmos_write(0, CMOS_EC_TRUSTED_OFFSET);
}

/*
 * This is done in BS_PAYLOAD_LOAD state on entry because finalize call locks down
 * upper CMOS bank in BS_PAYLOAD_LOAD state on exit.
 */
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_ENTRY, cmos_reset_ec_trusted, NULL);
