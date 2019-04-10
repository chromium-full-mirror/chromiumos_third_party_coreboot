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

#include <boot/coreboot_tables.h>
#include <bootmode.h>
#include <console/console.h>
#include <security/tpm/tis.h>
#include <timer.h>
#include "board.h"

void setup_chromeos_gpios(void)
{
	gpio_input(GPIO_WP_STATE);
	gpio_input(GPIO_REC_STATE);
	gpio_input(GPIO_DEV_STATE);
}

void fill_lb_gpios(struct lb_gpios *gpios)
{
	struct lb_gpio chromeos_gpios[] = {
		{GPIO_DEV_STATE.addr, ACTIVE_HIGH,
			gpio_get(GPIO_DEV_STATE), "developer"},
		{GPIO_REC_STATE.addr, ACTIVE_LOW,
			gpio_get(GPIO_REC_STATE), "recovery"},
		{GPIO_WP_STATE.addr, ACTIVE_LOW,
			gpio_get(GPIO_WP_STATE), "write protect"},
		{-1, ACTIVE_LOW, 1, "power"},
		{-1, ACTIVE_LOW, 0, "lid"},
	};

	lb_add_gpios(gpios, chromeos_gpios, ARRAY_SIZE(chromeos_gpios));
}

#define WIPEOUT_MODE_DELAY_MS (8 * 1000)
#define RECOVERY_MODE_EXTRA_DELAY_MS (8 * 1000)

/*
 * The recovery switch: it needs to be pressed for a
 * certain duration at startup to signal different requests:
 *
 * - keeping it pressed for 8 to 16 seconds after startup signals the need for
 *   factory reset (wipeout);
 * - keeping it pressed for longer than 16 seconds signals the need for Chrome
 *   OS recovery.
 *
 * The state is read once and cached for following inquiries. The below enum
 * lists possible states.
 */
enum switch_state {
	not_probed = -1,
	no_req,
	recovery_req,
	wipeout_req
};

static enum switch_state get_rec_sw_state(void)
{
	struct stopwatch sw;
	int sampled_value;
	gpio_t rec_sw;
	static enum switch_state saved_state = not_probed;

	if (is_tpm_detected() == 0)
		return saved_state;

	if (saved_state != not_probed)
		return saved_state;

	rec_sw = GPIO_REC_STATE;
	sampled_value = !gpio_get(rec_sw);

	if (!sampled_value) {
		saved_state = no_req;
		//display_pattern(WWR_NORMAL_BOOT);
		return saved_state;
	}

	//display_pattern(WWR_RECOVERY_PUSHED);
	printk(BIOS_INFO, "recovery button pressed\n");

	stopwatch_init_msecs_expire(&sw, WIPEOUT_MODE_DELAY_MS);

	do {
		sampled_value = !gpio_get(rec_sw);
		if (!sampled_value)
			break;
	} while (!stopwatch_expired(&sw));

	if (sampled_value) {
		//display_pattern(WWR_WIPEOUT_REQUEST);
		printk(BIOS_INFO, "wipeout requested, checking recovery\n");
		stopwatch_init_msecs_expire(&sw, RECOVERY_MODE_EXTRA_DELAY_MS);
		do {
			sampled_value = !gpio_get(rec_sw);
			if (!sampled_value)
				break;
		} while (!stopwatch_expired(&sw));

		if (sampled_value) {
			saved_state = recovery_req;
			//display_pattern(WWR_RECOVERY_REQUEST);
			printk(BIOS_INFO, "recovery requested\n");
		} else {
			saved_state = wipeout_req;
		}
	} else {
		saved_state = no_req;
		//display_pattern(WWR_NORMAL_BOOT);
	}

	return saved_state;
}

int get_recovery_mode_switch(void)
{
	return get_rec_sw_state() == recovery_req;
}

int get_wipeout_mode_switch(void)
{
	return get_rec_sw_state() == wipeout_req;
}

int get_write_protect_state(void)
{
	return !gpio_get(GPIO_WP_STATE);
}
