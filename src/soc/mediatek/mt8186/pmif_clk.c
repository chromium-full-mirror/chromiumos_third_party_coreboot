/* SPDX-License-Identifier: GPL-2.0-only OR MIT */

#include <commonlib/helpers.h>
#include <console/console.h>
#include <delay.h>
#include <device/mmio.h>
#include <soc/infracfg.h>
#include <soc/pll.h>
#include <soc/pll_common.h>
#include <soc/pmif.h>
#include <soc/pmif_clk_common.h>
#include <soc/pmif_sw.h>
#include <soc/pmif_spmi.h>
#include <soc/spm.h>

static void ulposc1_calibration(void)
{
	write32(&mtk_apmixed->ap_pll_con0, 0x000CE41F);
	write32(&mtk_apmixed->ulposc_con0, 0x00980110);

	setbits32((void *)0x105C4004, BIT(1));
	udelay(150);
	setbits32((void *)0x105C4004, BIT(2));

	printk(BIOS_DEBUG, "ulposc1: %u KHz\n", mt_fmeter_get_freq_khz(FMETER_ABIST, 35));
}

int pmif_clk_init(void)
{
	ulposc1_calibration();
	spmi_mux_select();

	return 0;
}
