/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/boothart.h>

extern uint64_t boot_hartid;

uint64_t get_boot_hartid(void)
{
	return boot_hartid;
}
