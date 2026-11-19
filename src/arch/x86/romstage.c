/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <console/console.h>
#include <timestamp.h>
#include <romstage_common.h>
#include <security/vboot/vboot_common.h>

asmlinkage void car_stage_entry(void)
{
	timestamp_add_now(TS_ROMSTAGE_START);

	/* Assumes the hardware was set up during the bootblock */
	console_init();

	/* Must run before anything that can reset the system. */
	if (!CONFIG(VBOOT_STARTS_BEFORE_BOOTBLOCK))
		vboot_rw_backport_hook();

	romstage_main();
}
