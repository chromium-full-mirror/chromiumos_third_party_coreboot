## SPDX-License-Identifier: GPL-2.0-or-later

romstage-y += romstage.c

ramstage-y += gpio.c
ramstage-$(CONFIG_FW_CONFIG) += variant.c
ramstage-y += ramstage.c
