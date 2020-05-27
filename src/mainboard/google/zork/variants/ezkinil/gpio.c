/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <baseboard/gpio.h>
#include <baseboard/variants.h>
#include <boardid.h>
#include <gpio.h>
#include <soc/gpio.h>
#include <ec/google/chromeec/ec.h>
static const struct soc_amd_gpio ezkinil_v1_gpio_set_stage_ram[] = {
	/* USB_OC4_L - USB_A1 */
	PAD_NF(GPIO_14, USB_OC4_L, PULL_UP),
	/* USB_OC2_L - USB A0 */
	PAD_NF(GPIO_18, USB_OC2_L, PULL_UP),
};

static const struct soc_amd_gpio ezkinil_gpio_set_stage_ram[] = {
	/* MST_GPIO_2 (Fw Update HDMI hub) Change NC */
	PAD_GPI(GPIO_86, PULL_UP),
	/* MST_GPIO_3 (Fw Update HDMI hub) Change NC */
	PAD_GPI(GPIO_90, PULL_UP),
	/* FPMCU_RST_L Change NC */
	PAD_GPI(GPIO_11, PULL_UP),
	/* FPMCU_BOOT0 Change NC */
	PAD_GPI(GPIO_69, PULL_UP),
};

const struct soc_amd_gpio *variant_override_gpio_table(size_t *size)
{
	uint32_t board_version;

	/*
	 * If board version cannot be read, assume that this is an older revision of the board
	 * and so apply overrides. If board version is provided by the EC, then apply overrides
	 * if version < 2.
	 */
	if (google_chromeec_cbi_get_board_version(&board_version))
		board_version = 1;

	if (board_version <= 1) {
		*size = ARRAY_SIZE(ezkinil_v1_gpio_set_stage_ram);
		return ezkinil_v1_gpio_set_stage_ram;
	}

	*size = ARRAY_SIZE(ezkinil_gpio_set_stage_ram);
	return ezkinil_gpio_set_stage_ram;
}
