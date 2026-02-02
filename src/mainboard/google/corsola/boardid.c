/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <boardid.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include <soc/auxadc.h>
#include <soc/cpu_id.h>

#include "panel.h"

/* board_id is provided by ec/google/chromeec/ec_boardid.c */

#define ADC_LEVELS 12

#define WUGTRIO_SKU_0_7 8
#define WUGTRIO_PANEL_INDEX_MAX 5

#define CROS_SKU_UNPROVISIONED_MT8186T 0x7FFFFEFF

enum {
	/* RAM IDs */
	RAM_ID_LOW_CHANNEL = 2,
	RAM_ID_HIGH_CHANNEL = 3,
	/* SKU IDs */
	SKU_ID_LOW_CHANNEL = 4,
	SKU_ID_HIGH_CHANNEL = 5,
};

static const unsigned int lcm_voltages[ADC_LEVELS] = {
	/* ID : Voltage (unit: uV) */
	[0]  =       0,
	[1]  =   89191,
	[2]  =  148419,
	[3]  =  205240,
	[4]  =  282774,
	[5]  =  393556,
	[6]  =  495561,
	[7]  =  641944,
	[8]  =  798049,
	[9]  =  960054,
	[10] = 1191264,
	[11] = 1433691,
};

static const unsigned int ram_voltages[ADC_LEVELS] = {
	/* ID : Voltage (unit: uV) */
	[0]  =   74300,
	[1]  =  211700,
	[2]  =  318800,
	[3]  =  428600,
	[4]  =  541700,
	[5]  =  665800,
	[6]  =  781400,
	[7]  =  900000,
	[8]  = 1023100,
	[9]  = 1137000,
	[10] = 1240000,
	[11] = 1342600,
};

static const unsigned int *adc_voltages[] = {
	[RAM_ID_LOW_CHANNEL] = ram_voltages,
	[RAM_ID_HIGH_CHANNEL] = ram_voltages,
	[SKU_ID_LOW_CHANNEL] = ram_voltages,
	[SKU_ID_HIGH_CHANNEL] = ram_voltages,
};

static const unsigned int *adc_voltages_detachable[] = {
	[RAM_ID_LOW_CHANNEL] = ram_voltages,
	[RAM_ID_HIGH_CHANNEL] = ram_voltages,
	[SKU_ID_LOW_CHANNEL] = ram_voltages,
	[SKU_ID_HIGH_CHANNEL] = lcm_voltages,
};

/* SKU matrix for Wugtrio SKUs 0 to 7 to remap SKU IDs with panels */
#define SKU_GROUP_0_3_4_6 { CROS_SKU_UNKNOWN, 0, 4, 3, 6 }  /* LTE skus */
#define SKU_GROUP_1_2_5_7 { CROS_SKU_UNKNOWN, 2, 1, 5, 7 }  /* non-LTE skus */

static const uint32_t wugtrio_sku_matrix
	[WUGTRIO_SKU_0_7][WUGTRIO_PANEL_INDEX_MAX] = {
	/*
	 * 1st dimension: CBI SKU ID
	 * | CBI SKU ID | Original Panel     | LTE     |
	 * |------------|--------------------|---------|
	 * | 0          | KD_KD101NE3_40TI   | TRUE    |
	 * | 1          | STA_ER88577        | NO      |
	 * | 2          | KD_KD101NE3_40TI   | NO      |
	 * | 3          | LCE_LMFBX101117480 | TRUE    |
	 * | 4          | STA_ER88577        | TRUE    |
	 * | 5          | LCE_LMFBX101117480 | NO      |
	 * | 6          | TG_XTI05101        | TRUE    |
	 * | 7          | TG_XTI05101        | NO      |

	 * 2nd dimension: Panel ID (simplified)
	 * | Panel ID  | Detected Panel     |
	 * |-----------|--------------------|
	 * | 0         | Reserve            |
	 * | 1         | KD_KD101NE3_40TI   |
	 * | 2         | STA_ER88577        |
	 * | 3         | LCE_LMFBX101117480 |
	 * | 4         | TG_XTI05101        |
	 *
	 * Cell: The actual SKU ID
	 */
	[0] = SKU_GROUP_0_3_4_6,
	[1] = SKU_GROUP_1_2_5_7,
	[2] = SKU_GROUP_1_2_5_7,
	[3] = SKU_GROUP_0_3_4_6,
	[4] = SKU_GROUP_0_3_4_6,
	[5] = SKU_GROUP_1_2_5_7,
	[6] = SKU_GROUP_0_3_4_6,
	[7] = SKU_GROUP_1_2_5_7,
};

#undef SKU_GROUP_0_3_4_6
#undef SKU_GROUP_1_2_5_7

static uint32_t get_adc_index(unsigned int channel)
{
	unsigned int value = auxadc_get_voltage_uv(channel);
	const unsigned int *voltages;

	if (CONFIG(BOARD_GOOGLE_STARYU_COMMON)) {
		assert(channel < ARRAY_SIZE(adc_voltages_detachable));
		voltages = adc_voltages_detachable[channel];
	} else {
		assert(channel < ARRAY_SIZE(adc_voltages));
		voltages = adc_voltages[channel];
	}

	assert(voltages);

	/* Find the closest voltage */
	uint32_t id;
	for (id = 0; id < ADC_LEVELS - 1; id++)
		if (value < (voltages[id] + voltages[id + 1]) / 2)
			break;

	printk(BIOS_DEBUG, "ADC[%u]: Raw value=%u ID=%u\n", channel, value, id);
	return id;
}

/*
 * The panel of Wugtrio can be replaced during RMA process,
 * meaning the panel implied in SKUs 0-7 may not be feasible.
 * Resolve the SKU ID based on CBI SKU ID and panel ID for those SKUs.
 * Valid panel IDs: 0x0 (STA_ER88577), 0x4 (KD_KD101NE3_40TI),
 * 0x7(LCE_LMFBX101117480), 0xA(TG_XTI05101).
 */
static int panel_id_to_index(uint32_t panel_id)
{
	switch (panel_id) {
	case 0x0: /* STA_ER88577 */
		return 2;
	case 0x4: /* KD_KD101NE3_40TI */
		return 1;
	case 0x7: /* LCE_LMFBX101117480 */
		return 3;
	case 0xA: /* TG_XTI05101 */
		return 4;
	default:
		return 0;
	}
}

static uint32_t resolve_sku_id(uint32_t cbi_sku_id, uint32_t panel_id)
{
	int panel_index = panel_id_to_index(panel_id);
	return wugtrio_sku_matrix[cbi_sku_id][panel_index];
}

/* Detachables use ADC channel 5 for panel ID */
uint32_t panel_id(void)
{
	static uint32_t cached_panel_id = BOARD_ID_INIT;

	if (cached_panel_id == BOARD_ID_INIT) {
		cached_panel_id = get_adc_index(SKU_ID_HIGH_CHANNEL);
		printk(BIOS_DEBUG, "%s: %#02x\n", __func__, cached_panel_id);
	}

	return cached_panel_id;
}

uint32_t sku_id(void)
{
	static uint32_t cached_sku_code = BOARD_ID_INIT;

	if (cached_sku_code != BOARD_ID_INIT)
		return cached_sku_code;

	const uint32_t cbi_sku_id = google_chromeec_get_board_sku();
	cached_sku_code = cbi_sku_id;

	if (cbi_sku_id == CROS_SKU_UNPROVISIONED ||
	    cbi_sku_id == CROS_SKU_UNKNOWN) {
		printk(BIOS_WARNING, "SKU code from EC: 0x%x\n", cbi_sku_id);
		cached_sku_code = CROS_SKU_UNPROVISIONED;
		if (get_cpu_id() == MTK_CPU_ID_MT8186T)
			cached_sku_code = CROS_SKU_UNPROVISIONED_MT8186T;

		if (CONFIG(BOARD_GOOGLE_STARYU_COMMON)) {
			/* Reserve last 4 bits to report PANEL_ID */
			cached_sku_code &= ~0xF;
			cached_sku_code |= panel_id();
		}
	} else if (CONFIG(BOARD_GOOGLE_WUGTRIO) &&
		   cbi_sku_id < WUGTRIO_SKU_0_7) {
		/* Workaround for SKU 0 to 7 */
		cached_sku_code = resolve_sku_id(cbi_sku_id, panel_id());

		if (cached_sku_code == CROS_SKU_UNKNOWN)
			printk(BIOS_ERR, "Failed to resolve SKU ID\n");
	} else if (CONFIG(BOARD_GOOGLE_STARMIE) ||
		   CONFIG(BOARD_GOOGLE_WYRDEER)) {
		/* resolve_sku_id() not implemented for these boards. */
	} else if (CONFIG(BOARD_GOOGLE_STARYU_COMMON)) {
		/* Encode panel ID for new boards and SKUs. */
		cached_sku_code = (panel_id() << 8) | (cbi_sku_id & 0xFF);
	}

	printk(BIOS_DEBUG,
	       "CBI SKU ID: %#02x, cached SKU code: %#02x\n",
	       cbi_sku_id, cached_sku_code);

	return cached_sku_code;
}

uint32_t ram_code(void)
{
	static uint32_t cached_ram_code = BOARD_ID_INIT;

	if (cached_ram_code == BOARD_ID_INIT) {
		cached_ram_code = (get_adc_index(RAM_ID_HIGH_CHANNEL) << 4 |
				   get_adc_index(RAM_ID_LOW_CHANNEL));
		printk(BIOS_DEBUG, "RAM Code: %#02x\n", cached_ram_code);
	}

	return cached_ram_code;
}
