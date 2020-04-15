#
# This file is part of the coreboot project.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

bootblock-$(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE) += gpio_baseboard_trembyle.c
bootblock-$(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ) += gpio_baseboard_dalboz.c

ifeq ($(CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK),y)
verstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE) += gpio_baseboard_trembyle.c
verstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ) += gpio_baseboard_dalboz.c
else
verstage-y += tpm_tis.c
endif

romstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE) += gpio_baseboard_trembyle.c
romstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ) += gpio_baseboard_dalboz.c
romstage-y += tpm_tis.c

ramstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE) += gpio_baseboard_trembyle.c
ramstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE) += fsps_baseboard_trembyle.c
ramstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ) += gpio_baseboard_dalboz.c
ramstage-$(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ) += fsps_baseboard_dalboz.c
ramstage-y += helpers.c
ramstage-y += tpm_tis.c

# Add OEM ID table
ifeq ($(CONFIG_USE_OEM_BIN),y)
cbfs-files-y += oem.bin
oem.bin-file := $(call strip_quotes,$(CONFIG_OEM_BIN_FILE))
oem.bin-type := raw
endif #($(CONFIG_USE_OEM_BIN),y)

# APCB Board ID GPIO configuration.
# These GPIOs determine which memory SPD will be used during boot.
# APCB_BOARD_ID_GPIO[0-3] = GPIO_NUMBER GPIO_IO_MUX GPIO_BANK_CTL
# GPIO_NUMBER: FCH GPIO number
# GPIO_IO_MUX: Value write to IOMUX to configure this GPIO
# GPIO_BANK_CTL: Value write to GPIOBankCtl[23:16] to configure this GPIO
ifeq ($(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE),y)
APCB_BOARD_ID_GPIO0 = 121 1 0
APCB_BOARD_ID_GPIO1 = 120 1 0
APCB_BOARD_ID_GPIO2 = 131 3 0
APCB_BOARD_ID_GPIO3 = 116 1 0
else ifeq ($(CONFIG_BOARD_GOOGLE_BASEBOARD_DALBOZ),y)
APCB_BOARD_ID_GPIO0 = 132 1 0
APCB_BOARD_ID_GPIO1 = 90 1 0
APCB_BOARD_ID_GPIO2 = 86 3 0
APCB_BOARD_ID_GPIO3 = 69 1 0
else
$(error Undefined APCB selection GPIOS for Zork baseboard)
endif #($(CONFIG_BOARD_GOOGLE_BASEBOARD_TREMBYLE),y)

# SPD_SOURCES is used to generate spd.bin, which is used by mosys
# APCB_SOURCES and SPD_SOURCES can be merged after b/141434940 is integrated.
SPD_SOURCES = $(patsubst %_x1,%,$(patsubst %_x2,%,$(APCB_SOURCES)))
