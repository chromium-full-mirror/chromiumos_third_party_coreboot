/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <intelblocks/early_graphics.h>
#include <pc80/vga.h>
#include <timestamp.h>

#include "ux.h"

bool ux_inform_user_of_update_operation(const char *name)
{
	timestamp_add_now(TS_ESOL_START);

	if (!CONFIG(CHROMEOS_ENABLE_ESOL) ||
	    !early_graphics_init()) {
		timestamp_add_now(TS_ESOL_END);
		return false;
	}

	printk(BIOS_INFO, "Informing user on-display of %s.\n", name);
	vga_write_text(VGA_TEXT_CENTER, VGA_TEXT_HORIZONTAL_MIDDLE,
		       "Your device is finishing an update. This may take 1-2 minutes.\nPlease do not turn off your device.");

	timestamp_add_now(TS_ESOL_END);
	return true;
}
