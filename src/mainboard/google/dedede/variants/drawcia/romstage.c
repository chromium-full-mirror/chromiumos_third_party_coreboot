/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <arch/romstage.h>
#include <console/console.h>
#include <fmap.h>
#include <gpio.h>
#include <halt.h>
#include <pc80/mc146818rtc.h>
#include <security/tpm/tss.h>
#include <security/vboot/vboot_common.h>
#include <security/vboot/misc.h>
#include <variant/ec.h>
#include <variant/gpio.h>
#include <vb2_api.h>

/*
 * This function ensures that EC has been reset at least once to restart execution in RO when
 * booting in developer mode. This is to protect against any EC compromises of the EC-RW at
 * runtime or in flash.
 *
 * Steps followed:
 *  1. Check if developer mode is enabled. If not, nothing to do.
 *  2. Check if EC_IN_RW is 0. If yes, nothing to do.
 *  3. Check if CMOS offset (CMOS_EC_TRUSTED_OFFSET) is 1. If yes, nothing to do. Coreboot has
 *  already triggered a H1 reset. So, we trust that EC started in RO. If EC-RO was able to
 *  verify RW using EFS, then we trust EC-RW as well. If EC-RW verification failed by EFS, we
 *  would never reach this step.
 *  4. If this step is reached, it means that we are in developer mode with EC_IN_RW=1 and
 *  haven't requested H1 to perform an EC reset. Request H1 to perform an EC reset and set
 *  CMOS_EC_TRUSTED_OFFSET to 1.
 *  5. In case of failure to talk to H1, we cannot really do anything more. So, halt().
 *
 * CMOS_EC_TRUSTED_OFFSET lives in upper CMOS bank offset 0x38 which is set to 0 in ramstage
 * before jumping to payload and this byte gets locked down in SMI handler finalize call. It
 * ensures that the byte cannot be written by OS or payload.
 */
void mainboard_romstage_pre_ec_sync_entry(void)
{
	static char current_ro_fw_id[100];
	static const char *bad_ro_fw_id1 = "Google_Drawcia.13606.52.0";
	static const char *bad_ro_fw_id2 = "Google_Drawcia.13606.74.0";
	struct vb2_context *ctx = vboot_get_context();

	/* Read the current RO Firmware ID and halt if it cannot be read. */
	if (fmap_read_area("RO_FRID", current_ro_fw_id, sizeof(current_ro_fw_id)) == -1)
		halt();

	/* No need to perform pre EC sync check for devices in field without bad RO FW. */
	if (strncmp(current_ro_fw_id, bad_ro_fw_id1, strlen(bad_ro_fw_id1)) &&
	    strncmp(current_ro_fw_id, bad_ro_fw_id2, strlen(bad_ro_fw_id2))) {
		printk(BIOS_DEBUG, "RO is known good.. return!\n");
		return;
	}

	gpio_input(GPIO_EC_IN_RW);

	if (!vboot_developer_mode_enabled()) {
		printk(BIOS_DEBUG, "Not in developer mode.. return!\n");
		return;
	}

	if (gpio_get(GPIO_EC_IN_RW) == 0) {
		printk(BIOS_DEBUG, "EC not in RW.. return!\n");
		return;
	}

	uint8_t val = cmos_read(CMOS_EC_TRUSTED_OFFSET);
	if (val == 1) {
		printk(BIOS_DEBUG, "EC is trusted.. return!\n");
		return;
	}

	int ret = tlcl_lib_init();
	if (ret != VB2_SUCCESS) {
		printk(BIOS_ERR, "Can't talk to cr50.. die!\n");
		halt();
	}

	cmos_write(1, CMOS_EC_TRUSTED_OFFSET);
	vb2api_prepare_for_extra_reboot(ctx);
	vboot_save_data(ctx);
	ret = tlcl_cr50_reset_ec();
	if (ret != TPM_SUCCESS) {
		cmos_write(0, CMOS_EC_TRUSTED_OFFSET);
		printk(BIOS_ERR, "Can't reset EC.. die!\n");
		halt();
	}
}
