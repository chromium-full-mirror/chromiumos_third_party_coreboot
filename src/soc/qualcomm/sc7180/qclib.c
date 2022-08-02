/* SPDX-License-Identifier: GPL-2.0-only */

#include <cbfs.h>
#include <fmap.h>
#include <console/console.h>
#include <soc/symbols.h>
#include <soc/qclib_common.h>
#include <device/mmio.h>

#define LONG_SYS_DCB_REG 0x7801C0
#define FUSE_BIT 15

static int dcb_fuse_longsys1p8(void)
{
	unsigned int fuse_value, bit_value;
	fuse_value = read32((unsigned int *)LONG_SYS_DCB_REG);
	bit_value = (fuse_value >> FUSE_BIT) & 0x1;
	return bit_value;
}

int qclib_soc_blob_load(void)
{
	size_t size;
	const char *dcb = CONFIG_CBFS_PREFIX "/dcb";

	/* Attempt to load PMICCFG Blob */
	size = cbfs_boot_load_file(CONFIG_CBFS_PREFIX "/pmiccfg",
			_pmic, REGION_SIZE(pmic), CBFS_TYPE_RAW);
	if (!size)
		return -1;
	qclib_add_if_table_entry(QCLIB_TE_PMIC_SETTINGS, _pmic, size, 0);

	/* Attempt to load DCB Blob */
	if (dcb_fuse_longsys1p8()) {
		printk(BIOS_INFO, "Using DCB for Longsys 1.8V memory based on fuse setting\n");
		dcb = CONFIG_CBFS_PREFIX "/dcb_longsys1p8";
	}
	size = cbfs_boot_load_file(dcb, _dcb, REGION_SIZE(dcb), CBFS_TYPE_RAW);
	if (!size)
		return -1;
	qclib_add_if_table_entry(QCLIB_TE_DCB_SETTINGS, _dcb, size, 0);

	return 0;
}
