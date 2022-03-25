/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <bootstate.h>
#include <pc80/mc146818rtc.h>
#include <variant/ec.h>
#include <baseboard/variants.h>
#include <boardid.h>
#include <fw_config.h>
#include <soc/soc_chip.h>

static struct acpi_gpio lte_reset_gpio = ACPI_GPIO_OUTPUT_ACTIVE_LOW(GPP_H0);
static struct acpi_gpio lte_enable_gpio = ACPI_GPIO_OUTPUT_ACTIVE_HIGH(GPP_A10);
/* New lte reset for drapwer DVT*/
static struct acpi_gpio lte_new_reset_gpio =  ACPI_GPIO_OUTPUT_ACTIVE_LOW(GPP_H17);

static void ext_vr_update(void)
{
	struct soc_intel_jasperlake_config *cfg = config_of_soc();
	if (fw_config_probe(FW_CONFIG(EXT_VR, EXT_VR_ABSENT)))
		cfg->disable_external_bypass_vr = 1;
}

void variant_devtree_update(void)
{

	uint32_t board_version = board_id();

	if (board_version <= 9) /* board version 9 is drawper EVT */
		update_lte_device(&lte_reset_gpio, &lte_enable_gpio);
	else
		update_lte_device(&lte_new_reset_gpio, &lte_enable_gpio);
	ext_vr_update();
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
