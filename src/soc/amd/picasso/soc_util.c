/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <arch/cpu.h>
#include <soc/cpu.h>
#include <soc/soc_util.h>
#include <string.h>

int soc_is_pollock(void)
{
	return soc_is_raven2() && CONFIG(AMD_FT5);
}

int soc_is_dali(void)
{
	return soc_is_raven2() && CONFIG(AMD_FP5);
}

int soc_is_picasso(void)
{
	return soc_is_zen_plus() && CONFIG(AMD_FP5);
}

int soc_is_raven2(void)
{
	return cpuid_eax(1) >> 8 == RAVEN2_CPUID >> 8 || soc_is_dali_3250U();
}

int soc_is_zen_plus(void)
{
	return cpuid_eax(1) >> 8 == PICASSO_CPUID >> 8 && (soc_is_dali_3250U() == 0);
}

int soc_is_dali_3250U(void)
{
	struct cpuid_result regs;
	static char *cpu_name;
	const  char *dali_3250u_name = DALI_3250U_STR;
	uint32_t val[18];
	int i;

	for (i = 0; i <= 3; i++) {
		regs = cpuid(0x80000002 + i);
		val[i * 4 + 0] = regs.eax;
		val[i * 4 + 1] = regs.ebx;
		val[i * 4 + 2] = regs.ecx;
		val[i * 4 + 3] = regs.edx;
	}

	val[17] = 0;
	cpu_name = (char*)val;

	return strncmp(cpu_name, dali_3250u_name, strlen(dali_3250u_name))?  0 : 1;
}
