# SPDX-License-Identifier: GPL-2.0-only

romstage-y += memory.c
romstage-y += romstage.c

bootblock-y += gpio.c

ramstage-y += gpio.c
ramstage-y += ramstage.c

ramstage-y += variant.c
