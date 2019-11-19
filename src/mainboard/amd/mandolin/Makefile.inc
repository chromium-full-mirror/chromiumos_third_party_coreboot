#
# This file is part of the coreboot project.
#
# Copyright (C) 2019 Advanced Micro Devices, Inc.
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

romstage-y += gpio.c
ramstage-y += gpio.c

PHONY+=add_mchp_fw
INTERMEDIATE+=add_mchp_fw

ifeq ($(CONFIG_MANDOLIN_HAVE_MCHP_FW),y)

MANDOLIN_MICROCHIP_FW_OFFSET=0
add_mchp_fw: $(obj)/coreboot.pre
	printf "    DD         Adding Microchip firmware at ROM offset 0x%x\n" \
		"$(MANDOLIN_MICROCHIP_FW_OFFSET)"
	dd if=$(CONFIG_MANDOLIN_MCHP_FW_FILE) \
		of=$(obj)/coreboot.pre conv=notrunc bs=1 \
		seek=$(MANDOLIN_MICROCHIP_FW_OFFSET) >/dev/null 2>&1

else
files_added:: warn_no_mchp
endif # CONFIG_MANDOLIN_HAVE_MCHP_FW

PHONY+=warn_no_mchp
warn_no_mchp:
	printf "\n\t** WARNING **\n"
	printf "coreboot has been built without an the Microchip EC.\n"
	printf "Do not flash this image.  Your Mandolin's power button\n"
	printf "will not respond when you press it.\n\n"

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/acpi
