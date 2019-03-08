/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 Google LLC
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

#include <boardid.h>
#include <gpio.h>
#include <console/console.h>
#include <security/tpm/tis.h>
#include <security/tpm/tss.h>
#include <stdlib.h>

/*
 * Mistral boards dedicate to the board ID three GPIOs in ternary mode: 105, 106
 * and 107.
 */

static uint32_t board_id_val = UNDEFINED_STRAPPING_ID;

static uint32_t get_board_id(void)
{
	uint32_t bid;
	const gpio_t pins[] = {[2] = GPIO(107),
				[1] = GPIO(106),
				[0] = GPIO(105)};

	tlcl_lib_init();
	if (is_tpm_detected() == 0) {
		bid = 25;        /* Assign 25 for EVB boards */
		printk(BIOS_DEBUG, "EVB Board ID: %d\n", bid);
	} else {
		bid = 0;         /* Assign 0 for Proto boards */

		/* The board id assigned for Proto boards is 0.
		 * Since gpios are not wired in the initial phase,
		 * we will get 0 whihch is a coincidence.
		 * To make sure it starts working, after gpios are
		 * wired, reassign the value read from gpios to id.
		 */
		bid = gpio_binary_first_base3_value(pins, ARRAY_SIZE(pins));

		printk(BIOS_DEBUG, "Proto Board ID: %d\n", bid);
	}

	return bid;
}

uint32_t board_id(void)
{
	if (board_id_val == UNDEFINED_STRAPPING_ID)
		board_id_val = get_board_id();

	return board_id_val;
}
