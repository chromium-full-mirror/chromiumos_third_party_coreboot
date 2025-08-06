/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <boardid.h>
#include <console/console.h>
#include <ec/google/chromeec/ec.h>
#include <soc/auxadc.h>
#include "panel.h"

/* board_id is provided by ec/google/chromeec/ec_boardid.c */

#define ADC_LEVELS 8
#define CIRI_SKU_0_11 12
#define PANEL_ID_MAX 4

enum {
	/* RAM IDs */
	RAM_ID_LOW_CHANNEL = 2,
	RAM_ID_HIGH_CHANNEL = 3,
	/* PANEL IDs */
	PANEL_ID_HIGH_CHANNEL = 4,
	PANEL_ID_LOW_CHANNEL = 5,
};

static const unsigned int ram_voltages[] = {
	/* ID : Voltage (unit: uV) */
	[0] =   74296,
	[1] =  211673,
	[2] =  365055,
	[3] =  524272,
	[4] =  706302,
	[5] =  899119,
	[6] = 1108941,
	[7] = 1342616,
};

_Static_assert(ARRAY_SIZE(ram_voltages) == ADC_LEVELS, "Wrong array size of ram_voltages");

static const unsigned int panel_voltages[] = {
	/* ID : Voltage (unit: uV) */
	[0] =       0,
	[1] =  282774,
	[2] =  472379,
	[3] =  652542,
	[4] =  830258,
	[5] = 1011767,
	[6] = 1209862,
	[7] = 1427880,
};

_Static_assert(ARRAY_SIZE(panel_voltages) == ADC_LEVELS, "Wrong array size of panel_voltages");

static const unsigned int *adc_voltages[] = {
	[RAM_ID_LOW_CHANNEL] = ram_voltages,
	[RAM_ID_HIGH_CHANNEL] = ram_voltages,
	[PANEL_ID_HIGH_CHANNEL] = panel_voltages,
	[PANEL_ID_LOW_CHANNEL] = panel_voltages,
};

/* SKU matrix for Ciri SKU 0 to 11 to remap SKU IDs with different panels */
static const uint32_t sku_matrix[CIRI_SKU_0_11][PANEL_ID_MAX] = {
	/*
	 * 1st dimension: CBI SKU ID
	 * | CBI SKU ID | Original Panel | Audio Codec | Smart Amp |
	 * |------------|----------------|-------------|-----------|
	 * | 0          | BOE            | RT5682S     | MAX98390  |
	 * | 1          | IVO            | ES8326      | MAX98390  |
	 * | 2          | BOE            | ES8326      | MAX98390  |
	 * | 3          | IVO            | RT5682S     | MAX98390  |
	 * | 4          | BOE            | RT5682S     | TAS2563   |
	 * | 5          | IVO            | ES8326      | TAS2563   |
	 * | 6          | BOE            | ES8326      | TAS2563   |
	 * | 7          | IVO            | RT5682S     | TAS2563   |
	 * | 8          | CSOT           | RT5682S     | MAX98390  |
	 * | 9          | CSOT           | ES8326      | MAX98390  |
	 * | 10         | CSOT           | RT5682S     | TAS2563   |
	 * | 11         | CSOT           | ES8326      | TAS2563   |
	 *
	 * 2nd dimension: Panel ID (simplified)
	 * | Panel ID  | Detected Panel |
	 * |-----------|----------------|
	 * | 0         | <Reserved>     |
	 * | 1         | BOE            |
	 * | 2         | IVO            |
	 * | 3         | CSOT           |
	 *
	 * Cell: The actual SKU ID
	 */
	[0] =  { CROS_SKU_UNKNOWN, 0, 3, 8 },
	[1] =  { CROS_SKU_UNKNOWN, 2, 1, 9 },
	[2] =  { CROS_SKU_UNKNOWN, 2, 1, 9 },
	[3] =  { CROS_SKU_UNKNOWN, 0, 3, 8 },
	[4] =  { CROS_SKU_UNKNOWN, 4, 7, 10 },
	[5] =  { CROS_SKU_UNKNOWN, 6, 5, 11 },
	[6] =  { CROS_SKU_UNKNOWN, 6, 5, 11 },
	[7] =  { CROS_SKU_UNKNOWN, 4, 7, 10 },
	[8] =  { CROS_SKU_UNKNOWN, 0, 3, 8 },
	[9] =  { CROS_SKU_UNKNOWN, 2, 1, 9 },
	[10] = { CROS_SKU_UNKNOWN, 4, 7, 10 },
	[11] = { CROS_SKU_UNKNOWN, 6, 5, 11 },
};

static uint32_t get_adc_index(unsigned int channel)
{
	unsigned int value = auxadc_get_voltage_uv(channel);

	assert(channel < ARRAY_SIZE(adc_voltages));
	const unsigned int *voltages = adc_voltages[channel];
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
 * The panel can be replaced during RMA process, meaning the panel implied in
 * SKU 0 to 11 may be inaccurate.
 * Resolve the SKU ID based on CBI SKU ID and panel ID for those SKUs.
 */
static uint32_t resolve_sku_id(uint32_t cbi_sku_id, uint32_t panel_id)
{
	/*
	 * Valid panel IDs: 0x11 (BOE), 0x22 (IVO), 0x33(CSOT).
	 * The low channel can uniquely identify the panel on Ciri.
	 */
	uint32_t panel_high_ch = (panel_id >> 4) & 0xF;
	uint32_t panel_low_ch = panel_id & 0xF;

	if (panel_high_ch != panel_low_ch)
		return CROS_SKU_UNKNOWN;

	if (panel_low_ch >= PANEL_ID_MAX)
		return CROS_SKU_UNKNOWN;

	return sku_matrix[cbi_sku_id][panel_low_ch];
}

/* Returns the ID for LCD module (type of panel). */
uint32_t panel_id(void)
{
	static uint32_t cached_panel_id = BOARD_ID_INIT;

	if (cached_panel_id == BOARD_ID_INIT)
		cached_panel_id = get_adc_index(PANEL_ID_HIGH_CHANNEL) << 4 |
				  get_adc_index(PANEL_ID_LOW_CHANNEL);

	return cached_panel_id;
}

uint32_t sku_id(void)
{
	static uint32_t cached_sku_code = BOARD_ID_INIT;

	if (cached_sku_code != BOARD_ID_INIT)
		return cached_sku_code;

	const uint32_t cbi_sku_id = google_chromeec_get_board_sku();

	if (cbi_sku_id == CROS_SKU_UNKNOWN ||
	    cbi_sku_id == CROS_SKU_UNPROVISIONED) {
		printk(BIOS_WARNING, "SKU code from EC: %s\n",
		       (cbi_sku_id == CROS_SKU_UNKNOWN) ?
		       "CROS_SKU_UNKNOWN" : "CROS_SKU_UNPROVISIONED");
		/* Reserve last 8 bits to report PANEL_IDs */
		cached_sku_code = 0x7FFFFF00UL | panel_id();
	} else if (CONFIG(BOARD_GOOGLE_CIRI) && cbi_sku_id < CIRI_SKU_0_11) {
		/* Workaround for SKU 0 to 11 */
		cached_sku_code = resolve_sku_id(cbi_sku_id, panel_id());

		if (cached_sku_code == CROS_SKU_UNKNOWN)
			printk(BIOS_ERR, "Failed to resolve SKU ID\n");
	} else {
		/* Encode panel ID dynamically for newer boards and SKUs */
		cached_sku_code = (panel_id() << 8) | (cbi_sku_id & 0xFF);
	}
	printk(BIOS_DEBUG,
	       "CBI SKU ID: %#02x, panel ID: %#02x, cached SKU code: %#02x\n",
	       cbi_sku_id, panel_id(), cached_sku_code);

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
