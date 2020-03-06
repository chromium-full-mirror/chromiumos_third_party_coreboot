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

bootblock-$(CONFIG_AMD_FP5) += gpio_fp5.c
bootblock-$(CONFIG_AMD_FT5) += gpio_ft5.c

ifneq ($(CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK),y)
verstage-y += tpm_tis.c
endif

romstage-$(CONFIG_AMD_FP5) += gpio_fp5.c
romstage-$(CONFIG_AMD_FT5) += gpio_ft5.c
romstage-y += tpm_tis.c

ramstage-$(CONFIG_AMD_FP5) += gpio_fp5.c
ramstage-$(CONFIG_AMD_FT5) += gpio_ft5.c
ramstage-y += tpm_tis.c

# Add OEM ID table
ifeq ($(CONFIG_USE_OEM_BIN),y)
cbfs-files-y += oem.bin
oem.bin-file := $(call strip_quotes,$(CONFIG_OEM_BIN_FILE))
oem.bin-type := raw
endif #($(CONFIG_USE_OEM_BIN),y)
