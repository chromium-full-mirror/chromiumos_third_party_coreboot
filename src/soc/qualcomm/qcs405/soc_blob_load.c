/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018, The Linux Foundation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <cbfs.h>
#include <console/console.h>
#include <symbols.h>
#include <boardid.h>
#include <soc/symbols.h>
#include <soc/qclib_common.h>

enum buck_type {
	ext_buck = 1,
	int_buck,
};

struct board_config {
	u8 buck_type;
} qcs405_board_config;

void qclib_set_buck_type(void)
{
	qcs405_board_config.buck_type = ext_buck;
	if ((uint8_t)board_id() == 2)
		qcs405_board_config.buck_type = int_buck;

	*_board_config = qcs405_board_config.buck_type;
}

int qclib_soc_blob_load(void)
{
	size_t size;

	/* Attempt to load PMICCFG Blob */

	size = cbfs_boot_load_file(CONFIG_CBFS_PREFIX "/pmiccfg",
			_pmic, REGION_SIZE(pmic), CBFS_TYPE_RAW);
	if (!size)
		goto fail;

	qclib_add_if_table_entry(QCLIB_TE_PMIC_SETTINGS, _pmic, size, 0);

	/* Attempt to load DCB Blob */
	size = cbfs_boot_load_file(CONFIG_CBFS_PREFIX "/dcb",
			_dcb, REGION_SIZE(dcb), CBFS_TYPE_RAW);
	if (!size)
		goto fail;

	qclib_add_if_table_entry(QCLIB_TE_DCB_SETTINGS, _dcb, size, 0);

	qclib_add_if_table_entry(QCLIB_TE_BOARD_CONFIG, _board_config,
				REGION_SIZE(board_config), 0);

	return 0;

fail:
	die("Couldn't run soc_blob_load.\n");
	return 1;
}
