/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2014 Vladimir Serbinenko
 * Copyright (C) 2018 Intel Corporation.
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

#include <arch/acpi.h>
#include <arch/acpigen.h>
#include <string.h>
#include <soc/i915.h>

void
intel_igd_displays_ssdt_generate(struct i915_gpu_controller_info *conf)
{
	size_t i;
	const char **names = conf->names;
	int counters[ARRAY_SIZE(names)];

	memset(counters, 0, sizeof(counters));

	acpigen_write_scope("\\_SB.PCI0.GFX0");

	/* Method (_DOD, 0) */
	acpigen_write_method("_DOD", 0);
	acpigen_emit_byte(0xa4); /* ReturnOp.  */
	acpigen_write_package(conf->ndid);

	for (i = 0; i < conf->ndid; i++) {
		acpigen_write_dword (conf->did[i]);
	}
	acpigen_pop_len(); /* End Package. */
	acpigen_pop_len(); /* End Method. */

	for (i = 0; i < conf->ndid; i++) {
		acpigen_write_device(names[i]);
		/* Name (_ADR, 0x<>) */
		acpigen_write_name_dword("_ADR", conf->did[i] & 0xffff);

		/* ACPI brightness for LCD.  */
		if (!strcmp(names[i],"LCD")) {
			printk(BIOS_ERR,"generate ACPI methods for LCD\n");
			/*
			  Method (_BCL, 0, NotSerialized)
			  {
				Return (^^XBCL())
			  }
			*/
			acpigen_write_method("_BCL", 0);
			acpigen_emit_byte(0xa4); /* ReturnOp.  */
			acpigen_emit_namestring("^^XBCL");
			acpigen_pop_len();

			/*
			  Method (_BCM, 1, NotSerialized)
			  {
				^^XBCM(Arg0)
			  }
			*/
			acpigen_write_method("_BCM", 1);
			acpigen_emit_namestring("^^XBCM");
			acpigen_emit_byte(0x68); /* Arg0Op.  */
			acpigen_pop_len();

			/*
			  Method (_BQC, 0, NotSerialized)
			  {
				Return (^^XBQC())
			  }
			*/
			acpigen_write_method("_BQC", 0);
			acpigen_emit_byte(0xa4); /* ReturnOp.  */
			acpigen_emit_namestring("^^XBQC");
			acpigen_pop_len();
		}
		acpigen_pop_len();
	}

	acpigen_pop_len();
}
