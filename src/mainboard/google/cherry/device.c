/* SPDX-License-Identifier: GPL-2.0-only */

#include <boardid.h>
#include <soc/device.h>

bool mainboard_needs_pcie_init(void)
{
	uint32_t sku;

	if (!CONFIG(BOARD_GOOGLE_DOJO))
		return false;

	sku = sku_id();
	switch (sku) {
	case 2:
	case 3:
	case 6:
	case 7:
		return true;
	default:
		return false;
	}
}
