/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2019 Advanced Micro Devices, Inc.
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

#include <console/console.h>
#include <soc/southbridge.h>
#include <stdlib.h>
#include <soc/gpio.h>

#include "gpio.h"

/*
 * As a rule of thumb, GPIO pins used by coreboot should be initialized at
 * bootblock while GPIO pins used only by the OS should be initialized at
 * ramstage.
 */
static const struct soc_amd_gpio gpio_set_stage_reset[] = {
	/* not LLB */
	PAD_GPI(GPIO_12, PULL_UP),
	/* not USB_OC5_L */
	PAD_GPI(GPIO_13, PULL_UP),
	/* not USB_OC4_L */
	PAD_GPI(GPIO_14, PULL_UP),
	/* not USB_OC1_L */
	PAD_GPI(GPIO_17, PULL_UP),
	/* not USB_OC2_L */
	PAD_GPI(GPIO_18, PULL_UP),
	/* SDIO eMMC power control */
	PAD_NF(GPIO_22, EMMC_PRW_CTRL, PULL_NONE),
	/* PCIe SSD power enable */
	PAD_GPO(GPIO_23, HIGH),
	/* PCIe Reset to DP0, DP1, J2105, TP, FP */
	PAD_NF(GPIO_27, PCIE_RST1_L, PULL_NONE),
	/* eSPI CS# */
	PAD_NF(GPIO_30, ESPI_CS_L, PULL_NONE),
	/* FANOUT0 */
	PAD_NF(GPIO_85, FANOUT0, PULL_NONE),
	/* PC beep to codec */
	PAD_NF(GPIO_91, SPKR, PULL_NONE),
};

static const struct soc_amd_gpio gpio_set_stage_ram[] = {
	/* SSD DEVSLP */
	PAD_NF(GPIO_5, DEVSLP0_S5, PULL_NONE),
	/* Defeature SATA Express DEVSLP, as some boards are reworked
	 * to tie this to GPIO23 to control power */
	PAD_GPI(GPIO_6, PULL_UP),
	/* I2S SDIN */
	PAD_NF(GPIO_7, ACP_I2S_SDIN, PULL_NONE),
	/* I2S LRCLK */
	PAD_NF(GPIO_8, ACP_I2S_LRCLK, PULL_NONE),
	/* Blink */
	PAD_NF(GPIO_11, BLINK, PULL_NONE),
	/* APU_ALS_INT# */
	PAD_SCI(GPIO_24, PULL_UP, EDGE_LOW),
	/* Finger print CS# */
	PAD_GPO(GPIO_31, HIGH),
	/* NFC IRQ */
	PAD_INT(GPIO_69, PULL_UP, EDGE_LOW, STATUS),
	/* Rear camera power enable */
	PAD_GPO(GPIO_89, HIGH),
};

/* eMMC controller driving either an SD card or eMMC device. */
static const struct soc_amd_gpio emmc_gpios[] = {
	PAD_NF(GPIO_21,  EMMC_CMD,	PULL_UP),
	PAD_NF(GPIO_22,  EMMC_PRW_CTRL,	PULL_UP),
	PAD_NF(GPIO_68,  EMMC_CD,	PULL_UP),
	PAD_NF(GPIO_70,  EMMC_CLK,	PULL_NONE),
	PAD_NF(GPIO_104, EMMC_SDATA0,	PULL_UP),
	PAD_NF(GPIO_105, EMMC_DATA1,	PULL_UP),
	PAD_NF(GPIO_106, EMMC_DATA2,	PULL_UP),
	PAD_NF(GPIO_107, EMMC_DATA3,	PULL_NONE),
	PAD_NF(GPIO_74,  EMMC_DATA4,	PULL_UP),
	PAD_NF(GPIO_75,  EMMC_DATA6,	PULL_UP),
	PAD_NF(GPIO_87,  EMMC_DATA7,	PULL_UP),
	PAD_NF(GPIO_88,  EMMC_DATA5,	PULL_UP),
	PAD_NF(GPIO_109, EMMC_DS,	PULL_UP),
};

void mainboard_program_early_gpios(void)
{
	program_gpios(gpio_set_stage_reset, ARRAY_SIZE(gpio_set_stage_reset));
}

void mainboard_program_gpios(void)
{
	program_gpios(gpio_set_stage_ram, ARRAY_SIZE(gpio_set_stage_ram));

	/* Re-muxing LPCCLK0 can hang the system if LPC is in use. */
	if (CONFIG(AMD_LPC_DEBUG_CARD))
		printk(BIOS_INFO, "eMMC not available due to LPC requirement\n");
	else
		program_gpios(emmc_gpios, ARRAY_SIZE(emmc_gpios));
}
