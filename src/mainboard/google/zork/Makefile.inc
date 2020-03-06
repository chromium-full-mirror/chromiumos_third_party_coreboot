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

romstage-y += chromeos.c
romstage-y += ec.c

ramstage-y += chromeos.c
ramstage-y += ec.c
ramstage-y += acpi.c

ifneq ($(CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK),y)
verstage-y += chromeos.c
verstage-y += ec.c
endif

smm-y += smihandler.c

subdirs-y += variants/baseboard
subdirs-y += spd

CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/baseboard/include
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/baseboard/include/baseboard/acpi

subdirs-y += variants/$(VARIANT_DIR)
CPPFLAGS_common += -I$(src)/mainboard/$(MAINBOARDDIR)/variants/$(VARIANT_DIR)/include
