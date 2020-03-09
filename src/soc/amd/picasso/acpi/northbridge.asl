/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Sage Electronic Engineering, LLC
 * Copyright (C) 2016 Advanced Micro Devices, Inc.
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

/* Note: Only need HID on Primary Bus */
External (TOM1)
External (TOM2)
Name(_HID, EISAID("PNP0A08"))	/* PCI Express Root Bridge */
Name(_CID, EISAID("PNP0A03"))	/* PCI Root Bridge */
Name(_ADR, 0x00180000)	/* Dev# = BSP Dev#, Func# = 0 */

/* Describe the Northbridge devices */

Method(_BBN, 0, NotSerialized)	/* Bus number = 0 */
{
	Return(Zero)
}

Method(_STA, 0, NotSerialized)
{
	Return(0x0B)	/* Status is visible */
}

/* PCI Routing Table */
Name(PR0, Package(){
	/* Bus 0, Dev 0x00 - F2: IOMMU */
	Package() { 0x0000FFFF, 0, INTA, 0 },
	Package() { 0x0000FFFF, 0, INTB, 0 },
	Package() { 0x0000FFFF, 0, INTC, 0 },
	Package() { 0x0000FFFF, 0, INTD, 0 },

	/* Bus 0, Dev 0x01 - F[1-7]: GPP PCI Bridge */
	Package() { 0x0001FFFF, 0, INTA, 0 },
	Package() { 0x0001FFFF, 1, INTB, 0 },
	Package() { 0x0001FFFF, 2, INTC, 0 },
	Package() { 0x0001FFFF, 3, INTD, 0 },

	/* Bus 0, Dev 0x08 - F1:PCI Bridge to Bus A, F2: PCI Bridge to Bus B */
	Package() { 0x0008FFFF, 0, INTA, 0 },
	Package() { 0x0008FFFF, 1, INTB, 0 },
	Package() { 0x0008FFFF, 2, INTC, 0 },
	Package() { 0x0008FFFF, 3, INTD, 0 },

	/* Bus 0, Dev 0x14 - F0:SMBus F3:LPC */
	Package() { 0x0014FFFF, 0, INTA, 0 },
	Package() { 0x0014FFFF, 1, INTB, 0 },
	Package() { 0x0014FFFF, 2, INTC, 0 },
	Package() { 0x0014FFFF, 3, INTD, 0 },
})

Method(_PRT,0, NotSerialized)
{
	Return(PR0)
}

Device(AMRT) {
	Name(_ADR, 0x00000000)
} /* end AMRT */

/* Internal Graphics */
Device(IGFX) {
	Name(_ADR, 0x00010000)
}

Device(AZHD) {	/* 0:9.2 - HD Audio */
	Name(_ADR, 0x00090002)
	OperationRegion(AZPD, PCI_Config, 0x00, 0x100)
		Field(AZPD, AnyAcc, NoLock, Preserve) {
		offset (0x42),
		NSDI, 1,
		NSDO, 1,
		NSEN, 1,
		offset (0x44),
		IPCR, 4,
		offset (0x54),
		PWST, 2,
		, 6,
		PMEB, 1,
		, 6,
		PMST, 1,
		offset (0x62),
		MMCR, 1,
		offset (0x64),
		MMLA, 32,
		offset (0x68),
		MMHA, 32,
		offset (0x6c),
		MMDT, 16,
	}

	Method (_INI, 0, NotSerialized)
	{
		If (LEqual (OSVR, 0x03))
		{
			Store (Zero, NSEN)
			Store (One, NSDO)
			Store (One, NSDI)
		}
	}
} /* end AZHD */
