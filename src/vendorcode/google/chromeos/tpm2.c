/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * Needed to get access to VBNV_RECOVERY_RO_S3_RESUME from vboot_reference.
 * TODO: Formally expose a way to set this recovery mode from within
 * vboot2 API.
 */
#define NEED_VB20_INTERNALS

#include <bootstate.h>
#include <console/console.h>
#include <lib/tpm2_tlcl_structures.h>
#include <tpm_lite/tlcl.h>
#include <vb2_api.h>

#include "vboot/misc.h"
#include "vboot/vbnv.h"

static void disable_platform_hierarchy(void *unused)
{
	int ret;

	if (!IS_ENABLED(CONFIG_TPM2))
		return;

	if (!IS_ENABLED(CONFIG_RESUME_PATH_SAME_AS_BOOT))
		return;

	ret = tlcl_lib_init();

	if (ret != VB2_SUCCESS) {
		printk(BIOS_ERR, "tlcl_lib_init() failed: %x\n", ret);
		return;
	}

	ret = tlcl_disable_platform_hierarchy();
	if (ret != TPM_SUCCESS)
		printk(BIOS_ERR, "Platform hierarchy disablement failed: %x\n",
			ret);
}

static int disable_tpm_helper(void *unused)
{
	if (tlcl_lib_init() != VB2_SUCCESS)
		printk(BIOS_ERR,
		       "TPM driver initialization failed, aborting resume\n");
	else if (tlcl_save_state() != TPM_SUCCESS)
		printk(BIOS_ERR,
		       "Could not save TPM state, aborting resume\n");
	else if (tlcl_cr50_set_tpm_mode(TpmModeDisabled))
		printk(BIOS_ERR,
		       "Could not disable TPM, aborting resume\n");
	else
		return 0;
	return 1;
}

static void disable_tpm(void *unused)
{
	/**
	 * If Alt OS legacy mode was selected on boot, then we are in the middle
	 * of an S3 resume, and we must do some extra work to disable the TPM.
	 */
	if (vboot_handoff_check_alt_os_legacy_boot_flag()) {
		printk(BIOS_INFO,
		       "Alt OS legacy boot detected - "
		       "disable TPM before resuming\n");
		if (disable_tpm_helper(unused)) {
			printk(BIOS_ERR,
			       "TPM disable failed, "
			       "setting recovery reason and rebooting\n");
			set_recovery_mode_into_vbnv(VBNV_RECOVERY_RO_S3_RESUME);
			vboot_reboot();
		} else
			printk(BIOS_INFO, "TPM disabled successfully\n");
	}
}

static void tpm_tasks(void *unused)
{
	disable_platform_hierarchy(unused);
	disable_tpm(unused);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, tpm_tasks, NULL);
