/* SPDX-License-Identifier: GPL-2.0-only */

#include <assert.h>
#include <bootmode.h>
#include <console/console.h>
#include <reset.h>
#include <security/vboot/misc.h>
#include <security/vboot/vboot_common.h>
#include <vb2_api.h>

static int gfx_init_done = -1;

int gfx_get_init_done(void)
{
	if (gfx_init_done < 0)
		return 0;
	return gfx_init_done;
}

void gfx_set_init_done(int done)
{
	gfx_init_done = done;
}

int display_init_required(void)
{
	/* For vboot, honor VB2_CONTEXT_DISPLAY_INIT. */
	if (CONFIG(VBOOT)) {
		/*
		 * Display init requires VBOOT_MUST_REQUEST_DISPLAY || VBOOT_ALWAYS_ENABLE_DISPLAY;
		 * else assert build.
		 */
		if (!CONFIG(VBOOT_MUST_REQUEST_DISPLAY) && !CONFIG(VBOOT_ALWAYS_ENABLE_DISPLAY))
			dead_code();
		return vboot_get_context()->flags & VB2_CONTEXT_DISPLAY_INIT;
	}

	/* By default always initialize display. */
	return 1;
}

__weak int platform_is_resuming(void)
{
	return 0;
}

/*
 * Check if the platform is resuming from S3 and force a reboot when
 * S3 resume is not supported.
 */
void prevent_unsupported_s3_resume(void)
{
	if (platform_is_resuming()) {
		printk(BIOS_EMERG, "ACPI S3 resume is not supported on this platform\n");
		/*
		 * Prepare for reboot to clear the sleep state such that the board
		 * is not stuck in a reboot loop.
		 */
		if (CONFIG(VBOOT))
			vboot_platform_prepare_reboot();
		board_reset();
	}
}
