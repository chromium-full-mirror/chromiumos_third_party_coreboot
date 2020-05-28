/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Intel Corp.
 * (Written by Andrey Petrov <andrey.petrov@intel.com> for Intel Corp.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <cbfs.h>
#include <cbmem.h>
#include <commonlib/fsp.h>
#include <commonlib/stdlib.h>
#include <console/console.h>
#include <fsp/api.h>
#include <fsp/util.h>
#include <program_loading.h>
#include <soc/intel/common/vbt.h>
#include <stage_cache.h>
#include <string.h>
#include <timestamp.h>
#include <types.h>

struct fsp_header fsps_hdr;

static void do_silicon_init(struct fsp_header *hdr)
{
	FSPS_UPD *upd, *supd;
	fsp_silicon_init_fn silicon_init;
	uint32_t status;
	uint8_t postcode;

	supd = (FSPS_UPD *) (hdr->cfg_region_offset + hdr->image_base);

	if (supd->FspUpdHeader.Signature != FSPS_UPD_SIGNATURE)
		die_with_post_code(POST_INVALID_VENDOR_BINARY,
			"Invalid FSPS signature\n");

	upd = xmalloc(sizeof(FSPS_UPD));

	memcpy(upd, supd, sizeof(FSPS_UPD));

	/* Give SoC/mainboard a chance to populate entries */
	platform_fsp_silicon_init_params_cb(upd);

	/* Call SiliconInit */
	silicon_init = (void *) (hdr->image_base +
				 hdr->silicon_init_entry_offset);
	fsp_debug_before_silicon_init(silicon_init, supd, upd);

	timestamp_add_now(TS_FSP_SILICON_INIT_START);
	post_code(POST_FSP_SILICON_INIT);
	status = silicon_init(upd);
	timestamp_add_now(TS_FSP_SILICON_INIT_END);
	post_code(POST_FSP_SILICON_EXIT);

	fsp_debug_after_silicon_init(status);

	/* Handle any errors returned by FspSiliconInit */
	fsp_handle_reset(status);
	if (status != FSP_SUCCESS) {
		if (vbt_get()) {
			/* Attempted to initialize graphics. Assume failure
			 * is related to a video failure.
			 */
			postcode = POST_VIDEO_FAILURE;
		} else {
			/* Other silicon initialization failed */
			postcode = POST_HW_INIT_FAILURE;
		}
		printk(BIOS_SPEW, "FspSiliconInit returned 0x%08x\n", status);
		die_with_post_code(postcode,
			"FspSiliconInit returned an error!\n");
	}
}

static int fsps_get_dest(const struct fsp_load_descriptor *fspld, void **dest,
				size_t size, const struct region_device *source)
{
	*dest = cbmem_add(CBMEM_ID_REFCODE, size);

	if (*dest == NULL)
		return -1;

	return 0;
}

void fsps_load(bool s3wake)
{
	struct fsp_load_descriptor fspld = {
		.fsp_prog = PROG_INIT(PROG_REFCODE, CONFIG_FSP_S_CBFS),
		.get_destination = fsps_get_dest,
	};
	struct prog *fsps = &fspld.fsp_prog;
	static int load_done;

	if (load_done)
		return;

	if (s3wake && !CONFIG(NO_STAGE_CACHE)) {
		printk(BIOS_DEBUG, "Loading FSPS from stage_cache\n");
		stage_cache_load_stage(STAGE_REFCODE, fsps);
		if (fsp_validate_component(&fsps_hdr, prog_rdev(fsps)) != CB_SUCCESS)
			die("On resume fsps header is invalid\n");
		load_done = 1;
		return;
	}

	if (fsp_load_component(&fspld, &fsps_hdr) != CB_SUCCESS)
		die("FSP-S failed to load\n");

	stage_cache_add(STAGE_REFCODE, fsps);

	load_done = 1;
}

void fsp_silicon_init(bool s3wake)
{
	fsps_load(s3wake);
	do_silicon_init(&fsps_hdr);
}
