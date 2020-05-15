/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/stages.h>
#include <console/console.h>
#include <fmap.h>
#include <soc/dramc_param.h>
#include <soc/emi.h>
#include <soc/mmu_operations.h>
#include <soc/mt6358.h>
#include <soc/pll.h>
#include <soc/rtc.h>

#include "early_init.h"

/* This must be defined in chromeos.fmd in same name and size. */
#define CALIBRATION_REGION		"RW_DDR_TRAINING"
#define CALIBRATION_REGION_SIZE		0x2000

_Static_assert(sizeof(struct dramc_param) <= CALIBRATION_REGION_SIZE,
	       "sizeof(struct dramc_param) exceeds " CALIBRATION_REGION);

static bool read_calibration_data_from_flash(struct dramc_param *dparam)
{
	const size_t length = sizeof(*dparam);
	size_t ret = fmap_read_area(CALIBRATION_REGION, dparam, length);
	printk(BIOS_DEBUG, "%s: ret=%#lx, length=%#lx\n",
	       __func__, ret, length);

	return ret == length;
}

static bool write_calibration_data_to_flash(const struct dramc_param *dparam)
{
	const size_t length = sizeof(*dparam);
	size_t ret = fmap_overwrite_area(CALIBRATION_REGION, dparam, length);
	printk(BIOS_DEBUG, "%s: ret=%#lx, length=%#lx\n",
	       __func__, ret, length);

	return ret == length;
}

static void write_calibration_message(unsigned char c)
{
	static char message[4096];
	static int cursor, flash_cursor, has_rdev = -1;
	static struct region_device rdev;
	const int chunk_size = sizeof(message);

	message[cursor++] = c;

	if (c) {
		do_putchar(c);
		if (cursor < chunk_size)
			return;
	}

	/* Flush message buffer. */
	if (has_rdev == -1)
		has_rdev = !fmap_locate_area_as_rdev_rw("DRAMK_LOG", &rdev);

	if (has_rdev &&
	    flash_cursor + chunk_size <= region_device_sz(&rdev) &&
	    rdev_eraseat(&rdev, flash_cursor, chunk_size) >= 0)
		rdev_writeat(&rdev, message, flash_cursor, cursor);

	flash_cursor += chunk_size;
	cursor = 0;
}

/* dramc_param is ~2K and too large to fit in stack. */
static struct dramc_param dramc_parameter;

static struct dramc_param_ops dparam_ops = {
	.param = &dramc_parameter,
	.read_from_flash = &read_calibration_data_from_flash,
	.write_to_flash = &write_calibration_data_to_flash,
	.do_putc = write_calibration_message,
};

void platform_romstage_main(void)
{
	/* This will be done in verstage if CONFIG_VBOOT is enabled. */
	if (!CONFIG(VBOOT))
		mainboard_early_init();

	mt6358_init();
	/* Adjust VSIM2 down to 2.7V because it is shared with IT6505. */
	pmic_set_vsim2_cali(2700);
	mt_pll_raise_ca53_freq(1989 * MHz);
	pmic_init_scp_voltage();
	rtc_boot();
	mt_mem_init(&dparam_ops);
	mtk_mmu_after_dram();
}
