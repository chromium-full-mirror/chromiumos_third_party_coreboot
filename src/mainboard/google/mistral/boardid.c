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

	bid = gpio_binary_first_base3_value(pins, ARRAY_SIZE(pins));

	return bid;
}

uint32_t board_id(void)
{
	if (board_id_val == UNDEFINED_STRAPPING_ID)
		board_id_val = get_board_id();

	return board_id_val;
}
