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

Method(_PRT,0, NotSerialized)
{
	If(PMOD)
	{
		Return(APR0)	/* APIC mode */
	}
	Return (PR0)		/* PIC Mode */
}

Device(AMRT) {
	Name(_ADR, 0x00000000)
} /* end AMRT */

/* Internal Graphics */
Device(IGFX) {
	Name(_ADR, 0x00010000)
}

/*
 * Routing definitions:
 * These would, by convention, be provided by mainboard code. On picasso, they
 * can also be generated automatically by northbridge code. When they are
 * dynamically generated, they will become part of the SSDT. IN the latter case,
 * the External() declaration is required.
 */
External(APS0)
External(APS1)
External(APS2)
External(APS3)
External(APS4)
External(APS5)
External(APS6)
External(APS7)
External(APS8)
External(PS0)
External(PS1)
External(PS2)
External(PS3)
External(PS4)
External(PS5)
External(PS6)
External(PS7)
External(PS8)

/* 00:01.2 - GPP Bridge 0 */
Device(PBR0) {
	Name(_ADR, 0x00010001)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS0) }	/* APIC mode */
		Return (PS0)			/* PIC Mode */
	}
}

/* 00:01.2 - GPP Bridge 1 */
Device(PBR1) {
	Name(_ADR, 0x00010002)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS1) }	/* APIC mode */
		Return (PS0)			/* PIC Mode */
	}
}

/* 00:01.3 - GPP Bridge 2 */
Device(PBR2) {
	Name(_ADR, 0x00010003)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS2) }	/* APIC mode */
		Return (PS0)			/* PIC Mode */
	}
}

/* 00:01.4 - GPP Bridge 3 */
Device(PBR3) {
	Name(_ADR, 0x00010004)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS3) }	/* APIC mode */
		Return (PS0)			/* PIC Mode */
	}
}

/* 00:01.5 - GPP Bridge 4 */
Device(PBR4) {
	Name(_ADR, 0x00010005)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS4) }	/* APIC mode */
		Return (PS4)			/* PIC Mode */
	}
}

/* 00:01.6 - GPP Bridge 5 */
Device(PBR5) {
	Name(_ADR, 0x00010006)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS5) }	/* APIC mode */
		Return (PS5)			/* PIC Mode */
	}
}

/* 00:01.7 - GPP Bridge 6 */
Device(PBR6) {
	Name(_ADR, 0x00010007)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS6) }	/* APIC mode */
		Return (PS6)			/* PIC Mode */
	}
}

/* 00:08.1 - GPP Internal bridge to bus A */
Device(PBR7) {
	Name(_ADR, 0x00080001)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS7) }	/* APIC mode */
		Return (PS7)			/* PIC Mode */
	}
}

/* 00:08.2 - GPP Internal bridge to bus B */
Device(PBR8) {
	Name(_ADR, 0x00080002)
	Method(_PRT, 0) {
		If(PMOD) { Return(APS8) }	/* APIC mode */
		Return (PS8)			/* PIC Mode */
	}
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
