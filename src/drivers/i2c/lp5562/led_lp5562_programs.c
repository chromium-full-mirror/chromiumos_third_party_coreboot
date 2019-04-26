/*
 * Copyright (C) 2019 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This is a driver for the TI LP5562 (http://www.ti.com/product/lp5562),
 * driving a tri-color LED.
 *
 * The only connection between the LED and the main board is an i2c bus.
 *
 * This driver imitates a depthcharge display device. On initialization the
 * driver sets up the controllers to prepare them to accept programs to run.
 *
 * When a certain vboot state needs to be indicated, the program for that
 * state is loaded into the controllers, resulting in the state appropriate
 * LED behavior.
 */

#include "drivers/i2c/lp5562/led_lp5562_programs.h"

/****************************************************************
 *   LED ring program definitions for different patterns.
 *
 * Comments below are real lp5562 source code, they are compiled using
 * lasm.exe, the TI tool available from their Web site (search for lp5562)
 * and running only on Windows :P.
 *
 * Different hex dumps are results of tweaking the source code parameters to
 * achieve desirable LED ring behavior. It is possible to use just one code
 * dump and replace certain values in the body to achieve different behaviour
 * with the same basic dump, but keeping track of location to tweak with every
 * code change would be quite tedious.
 */

/*
 * Solid LED display, the arguments of the set_pwm commands set intensity and
 * color of the display:

  1 00		.ENGINE1(R)
  2 00 4080		set_pwm 128
  3 01 C000		end
  4
  5 10		.ENGINE2(G)
  6 10 4080		set_pwm 128
  7 11 C000		end
  8
  9 20		.ENGINE3(B)
 10 20 4080		set_pwm 128
 11 21 C000		end
*/

/* RGB set to 000000, resulting in all LEDs off. */
static const uint8_t solid_00_text[] = {
	0x40, 0x00, 0xc0, 0x00
};

/* Rgb set to 128, resulting in a brightish white color. */
static const uint8_t solid_80_text[] = {
	0x40, 0x80, 0xc0, 0x00
};

static const ti_lp5562_program solid_000000_program = {
	{
		{ /* Engine1:Blue */
			solid_00_text,
			sizeof(solid_00_text),
			0,
		},
		{ /* Engine2:Green */
			solid_00_text,
			sizeof(solid_00_text),
			0,
		},
		{ /* Engine3:Red */
			solid_00_text,
			sizeof(solid_00_text),
			0,
		},
	}
};

static const ti_lp5562_program solid_808080_program = {
	{
		{ /* Engine1:Blue */
			solid_80_text,
			sizeof(solid_80_text),
			0,
		},
		{ /* Engine2:Green */
			solid_80_text,
			sizeof(solid_80_text),
			0,
		},
		{ /* Engine3:Red */
			solid_80_text,
			sizeof(solid_80_text),
			0,
		},
	}
};

/*
  1 00          .ENGINE1(B)
  2 00 E200             trigger w3
  3 01 409B             set_pwm 155
  4 02 E200             trigger w3
  5 03 4000             set_pwm 0
  6 04 0000             gotostart
  7
  8 10          .ENGINE2(G)
  9 10 E200             trigger w3
 10 11 4032             set_pwm 50
 11 12 E200             trigger w3
 12 13 4000             set_pwm 0
 13 14 0000             gotostart
 14
 15 20          .ENGINE3(R)
 16 20 E006             trigger s21
 17 21 4001             set_pwm 1
 18 22 5300             wait 300
 19 23 E006             trigger s21
 20 24 4000             set_pwm 0
 21 25 5300             wait 300
 22 26 0000             gotostart
*/

static const uint8_t blink_wipeout1_b_text[] = {
	0xe2,  0x00,  0x40,   155,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static const uint8_t blink_wipeout1_g_text[] = {
	0xe2,  0x00,  0x40,    50,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static const uint8_t blink_wipeout1_r_text[] = {
	0xe0,  0x06,  0x40,     1,  0x53,  0x00,  0xe0,  0x06,
	0x40,     0,  0x53,  0x00,  0x00,  0x00
};

static const ti_lp5562_program blink_wipeout1_program = {
	{
		{
			blink_wipeout1_b_text,
			sizeof(blink_wipeout1_b_text),
			0,
		},
		{
			blink_wipeout1_g_text,
			sizeof(blink_wipeout1_g_text),
			0,
		},
		{
			blink_wipeout1_r_text,
			sizeof(blink_wipeout1_r_text),
			0,
		},
	}
};

static const uint8_t blink_recovery1_b_text[] = {
	0xe2,  0x00,  0x40,    10,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static const uint8_t blink_recovery1_g_text[] = {
	0xe2,  0x00,  0x40,   100,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static const uint8_t blink_recovery1_r_text[] = {
	0xe0,  0x06,  0x40,   255,  0x53,  0x00,  0xe0,  0x06,
	0x40,     0,  0x53,  0x00,  0x00,  0x00
};

static const ti_lp5562_program blink_recovery1_program = {
	{
		{
			blink_recovery1_b_text,
			sizeof(blink_wipeout1_b_text),
			0,
		},
		{
			blink_recovery1_g_text,
			sizeof(blink_wipeout1_g_text),
			0,
		},
		{
			blink_recovery1_r_text,
			sizeof(blink_wipeout1_r_text),
			0,
		},
	}
};

/*
 * fade_in1.src
 *
  1 00          .ENGINE1(B)
  2 00 4000             set_pwm 0
  3 01 E200             trigger w3
  4 02 1B4C             ramp 1000, 77 # ramp up to 155 for 2 seconds
  5 03 1A4D             ramp 1000, 78
  6 04 C000             end
  7
  8 10          .ENGINE2(G)
  9 10 4000             set_pwm 0
 10 11 E200             trigger w3
 11 12 4318             ramp 1000, 25 # ramp up to 50 for 2 seconds
 12 13 4318             ramp 1000, 25
 13 14 C000             end
 14
 15 20          .ENGINE3(R)
 16 20 E006             trigger s21
 17 21 4001             set_pwm 1
 18 22 E006             trigger s21
 19 23 C000             end
*/

static const uint8_t fade_in1_b_text[] = {
	0x40,     0,  0xe2,  0x00,  0x1b,  0x4c,  0x1a,  0x4d,
	0xc0,  0x00
};
static const uint8_t fade_in1_g_text[] = {
	0x40,     0,  0xe2,  0x00,  0x43,  0x18,  0x43,  0x18,
	0xc0,  0x00
};
static const uint8_t fade_in1_r_text[] = {
	0xe0,  0x06,  0x40,     1,  0xe0,  0x06,  0x00
};

static const ti_lp5562_program fade_in1_program = {
	{
		{
			fade_in1_b_text,
			sizeof(fade_in1_b_text),
			0,
		},
		{
			fade_in1_g_text,
			sizeof(fade_in1_g_text),
			0,
		},
		{
			fade_in1_r_text,
			sizeof(fade_in1_r_text),
			0,
		},
	}
};

const led5562_state_prog led_lp5562_state_programs[] = {
	/*
	 * for test purposes the blank screen program is set to blinking, will
	 * be changed soon.
	 */
	{LED_ALL_OFF, {&solid_000000_program} },
	{LED_RECOVERY_PUSHED, {&solid_808080_program} },
	{LED_WIPEOUT_REQUEST, {&blink_wipeout1_program} },
	{LED_RECOVERY_REQUEST, {&blink_recovery1_program} },
	{LED_NORMAL_BOOT, {&fade_in1_program} },
	{}, /* Empty record to mark the end of the table. */
};
