/*
 * This file is part of the coreboot project.
 *
 * Copyright 2019 Huaqin Telecom Inc.
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

#ifndef __MAINBOARD_GOOGLE_DISPLAY_H__
#define __MAINBOARD_GOOGLE_DISPLAY_H__

#include <soc/dsi.h>
#include <soc/gpio.h>

#define PP3300_LCM_EN		GPIO(SIM2_SIO)
#define PP1800_LCM_EN		GPIO(SIM2_SRST)

#define FLAPJACK_PANEL_ADC_ID 2
#define FLAPJACK_PANEL_ID_BIT_POSITION 16

#define MAKE_AS_A_STRING(arg) #arg

#define PANEL(_edid, _init_table, _panel_id)\
	{\
	 .edid = &_edid,\
	 .init_table = _init_table,\
	 .table_size = ARRAY_SIZE(_init_table),\
	 .panel_name = MAKE_AS_A_STRING(_panel_id)}

enum panel_id {
	PANEL_KUKUI_INNOLUX = 0,
	PANEL_BOE_TV101WUM_NG0,
	PANEL_BOE_TV080WUM_NG0,
	PANEL_INX_OTA7290D10P,
	PANEL_AUO_NT51021D8P,
	PANEL_UNKNOWN,
};

struct panel_id_voltage {
	enum panel_id id;
	int voltage;
};

struct panel_info {
	struct edid *edid;
	struct lcm_init_table *init_table;
	u32 table_size;
	const char *panel_name;
};

void configure_backlight(enum panel_id);
void configure_display(enum panel_id);
void display_startup(enum panel_id);

enum panel_id get_panel_id(void);
#endif
