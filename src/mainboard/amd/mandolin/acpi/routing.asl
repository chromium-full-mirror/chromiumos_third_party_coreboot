/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015, 2016 Advanced Micro Devices, Inc.
 * Copyright (C) 2013 Sage Electronic Engineering, LLC
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
//
//
//
// todo: check file for accuracy
//
//
//

/*
#include <arch/acpi.h>
DefinitionBlock ("DSDT.AML", "DSDT", 0x01, OEM_ID, ACPI_TABLE_CREATOR, 0x00010001
		)
	{
		#include "routing.asl"
	}
*/

/* Routing is in System Bus scope */
Name(PR0, Package(){
	/* NB devices */

	/* 0:08.0 PCIe bridge */
	Package() { 0x0008FFFF, 0, INTA, 0 },
	Package() { 0x0008FFFF, 1, INTB, 0 },
	Package() { 0x0008FFFF, 2, INTC, 0 },
	Package() { 0x0008FFFF, 3, INTD, 0 },

	/* FCH devices */
	/* Bus 0, Dev 20 - F0:SMBu ;F3:LPC */
	Package() { 0x0014FFFF, 0, INTA, 0 },
	Package() { 0x0014FFFF, 1, INTB, 0 },
	Package() { 0x0014FFFF, 2, INTC, 0 },
	Package() { 0x0014FFFF, 3, INTD, 0 },
})

Name(APR0, Package(){
	/* NB devices are behind PCI bridges. Not in here, KTHXBYE */

	/* SB devices in APIC mode */
	/* Bus 0, Dev 20 - F0:SMBus F3:LPC */
	Package() { 0x0014FFFF, 0, 0, 16 },
	Package() { 0x0014FFFF, 1, 0, 17 },
	Package() { 0x0014FFFF, 2, 0, 18 },
	Package() { 0x0014FFFF, 3, 0, 19 },
})

Name(PS4, Package(){
})

Name(PS5, Package(){
})

Name(PS6, Package(){
})

Name(PS7, Package(){
})

Name(PS8, Package(){
})
