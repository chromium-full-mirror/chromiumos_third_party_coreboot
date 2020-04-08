# SPDX-License-Identifier: GPL-2.0-only
# This file is part of the coreboot project.

ifeq ($(CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK),y)

CPPFLAGS_verstage += -I$(src)/soc/amd/picasso/psp_verstage/include

verstage-y += psp_verstage.c
verstage-y += psp_startup.S
verstage-y += psp_end.S
verstage-y += psp.c
verstage-y += svc.c

$(obj)/psp_verstage.bin: $(objcbfs)/verstage.elf
	$(OBJCOPY_verstage) -O binary $^ $@

endif # CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK
