/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

/* Operating System Capabilities Method */
Method(_OSC,4) {
    /* Check for proper PCI/PCIe UUID
     * defined in "6.2.11.3 OSC Implementation Example
     * for PCI Host Bridge Devices" of ACPI Spec v6.3 */
    If(LEqual(Arg0,ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")))
    {
        /* Let OS control everything */
        Return (Arg3)
    } Else {
        CreateDWordField(Arg3,0,CDW1)
        Or(CDW1,4,CDW1)	// Unrecognized UUID
        Return(Arg3)
    }
}

Method(_CRS, 0) {
    /* DBGO("\\_SB\\PCI0\\_CRS\n") */
    CreateDWordField(CRES, ^MMIO._BAS, MM1B)
    CreateDWordField(CRES, ^MMIO._LEN, MM1L)

    /*
    * Declare memory between TOM1 and 4GB as available
    * for PCI MMIO.
    * Use ShiftLeft to avoid 64bit constant (for XP).
    * This will work even if the OS does 32bit arithmetic, as
    * 32bit (0x00000000 - TOM1) will wrap and give the same
    * result as 64bit (0x100000000 - TOM1).
    */
    Store(TOM1, MM1B)
    ShiftLeft(0x10000000, 4, Local0)
    Subtract(Local0, TOM1, Local0)
    Store(Local0, MM1L)

    Return(CRES) /* note to change the Name buffer */
} /* end of Method(_SB.PCI0._CRS) */

/*
*
*               FIRST METHOD CALLED UPON BOOT
*
*  1. If debugging, print current OS and ACPI interpreter.
*  2. Get PCI Interrupt routing from ACPI VSM, this
*     value is based on user choice in BIOS setup.
*/
Method(_INI, 0, Serialized) {
    /* DBGO("\\_SB\\_INI\n") */
    /* DBGO("   DSDT.ASL code from ") */
    /* DBGO(__DATE__) */
    /* DBGO(" ") */
    /* DBGO(__TIME__) */
    /* DBGO("\n   Sleep states supported: ") */
    /* DBGO("\n") */
    /* DBGO("   \\_OS=") */
    /* DBGO(\_OS) */
    /* DBGO("\n   \\_REV=") */
    /* DBGO(\_REV) */
    /* DBGO("\n") */

    /* Determine the OS we're running on */
    OSFL()

    /* Send ALIB Function 1 the AC/DC state */
    Name(F1BF, Buffer(0x03){})
    CreateWordField(F1BF, 0, F1SZ)
    CreateByteField(F1BF, 2, F1DA)

    Store(3, F1SZ)
    Store(\PWRS, F1DA)

    \_SB.ALIB(1, F1BF)

} /* End Method(_SB._INI) */

Method(OSFL, 0){

    if (LNotEqual(OSVR, Ones)) {Return(OSVR)}	/* OS version was already detected */

    if (CondRefOf(\_OSI))
    {
        Store(1, OSVR)					/* Assume some form of XP */
        if (\_OSI("Windows 2006"))		/* Vista */
        {
            Store(2, OSVR)
        }
    } else {
        If(WCMP(\_OS,"Linux")) {
            Store(3, OSVR)				/* Linux */
        } Else {
            Store(4, OSVR)				/* Gotta be WinCE */
        }
    }
    Return(OSVR)
}

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
    Return(PR0)
}

// Method(FDDC, 2, Serialized)
// {
//     Acquire(FDAS, 0xffff)

//     if(LEqual(Arg1, 0)) {
//         Switch(ToInteger(Arg0)) {
//             Case(Package() {5, 15, 24}) {
//                 Store(One, PG1A)
//             }
//             Case(Package() {6, 7, 8, 11, 12, 18}) {
//                 Store(One, PG2_)
//             }
//         }
//         /* put device into D0 */
//         Switch(ToInteger(Arg0))
//         {
//             Case(5) {
//                 Store(0x00, I0TD)
//                 Store(One, I0PD)
//                 Store(I0DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(I0DS, Local0)
//                 }
//             }
//             Case(6) {
//                 Store(0x00, I1TD)
//                 Store(One, I1PD)
//                 Store(I1DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(I1DS, Local0)
//                 }
//             }
//             Case(7) {
//                 Store(0x00, I2TD)
//                 Store(One, I2PD)
//                 Store(I2DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(I2DS, Local0)
//                 }
//             }
//             Case(8) {Store(0x00, I3TD)
//                 Store(One, I3PD)
//                 Store(I3DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(I3DS, Local0)
//                 }
//             }
//             Case(11) {
//                 Store(0x00, U0TD)
//                 Store(One, U0PD)
//                 Store(U0DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(U0DS, Local0)
//                 }
//             }
//             Case(12) {
//                 Store(0x00, U1TD)
//                 Store(One, U1PD)
//                 Store(U1DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(U1DS, Local0)
//                 }
//             }
//             Case(16) {
//                 Store(0x00, U2TD)
//                 Store(One, U2PD)
//                 Store(U2DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(U2DS, Local0)
//                 }
//             }
//             Case(26) {
//                 Store(0x00, U3TD)
//                 Store(One, U3PD)
//                 Store(U3DS, Local0)
//                 while(LNotEqual(Local0,0x7)) {
//                     Store(U3DS, Local0)
//                 }
//             }
//         }
//     } else {
//         /* put device into D3cold */
//         Switch(ToInteger(Arg0))
//         {
//             Case(5) {
//                 Store(Zero, I0PD)
//                 Store(I0DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(I0DS, Local0)
//                 }
//                 Store(0x03, I0TD)
//             }
//             Case(6) {
//                 Store(Zero, I1PD)
//                 Store(I1DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(I1DS, Local0)
//                 }
//                 Store(0x03, I1TD)
//             }
//             Case(7)  {
//                 Store(Zero, I2PD)
//                 Store(I2DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(I2DS, Local0)
//                 }
//                 Store(0x03, I2TD)}
//             Case(8) {
//                 Store(Zero, I3PD)
//                 Store(I3DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(I3DS, Local0)
//                 }
//                 Store(0x03, I3TD)
//             }
//             Case(11) {
//                 Store(Zero, U0PD)
//                 Store(U0DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(U0DS, Local0)
//                 }
//                 Store(0x03, U0TD)
//             }
//             Case(12) {
//                 Store(Zero, U1PD)
//                 Store(U1DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(U1DS, Local0)
//                 }
//                 Store(0x03, U1TD)
//             }
//             Case(16) {
//                 Store(Zero, U2PD)
//                 Store(U2DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(U2DS, Local0)
//                 }
//                 Store(0x03, U2TD)
//             }
//             Case(26) {
//                 Store(Zero, U3PD)
//                 Store(U3DS, Local0)
//                 while(LNotEqual(Local0,0x0)) {
//                     Store(U3DS, Local0)
//                 }
//                 Store(0x03, U3TD)
//             }
//         }
//         if(LEqual(I1TD, 3)) {
//             if(LEqual(I2TD, 3)) {
//                 if(LEqual(I3TD, 3)) {
//                     if(LEqual(U0TD, 3)) {
//                         if(LEqual(U1TD, 3)) {
//                             Store(Zero, PG2_)
//                         }
//                     }
//                 }
//             }
//         }
//     }
//     Release(FDAS)
// }

Method(FPTS,0, Serialized)  /* FCH _PTS *//* Operating System Capabilities Method */

{
}

// Method(FWAK,0, Serialized)  /* FCH _WAK */
// {
//     if(LEqual(\UT0E, zero)) {
//         if(LNotEqual(U0TD, 0x03)) {
//             FDDC(11, 3)
//         }
//     }
//     if(LEqual(\UT1E, zero)) {
//         if(LNotEqual(U1TD, 0x03)) {
//             FDDC(12, 3)
//         }
//     }
//     if(LEqual(\IC2E, zero)) {
//         if(LNotEqual(I2TD, 0x03)) {
//             FDDC(7, 3)
//         }
//     }
//     if(LEqual(\IC3E, zero)) {
//         if(LNotEqual(I3TD, 0x03)) {
//             FDDC(8, 3)
//         }
//     }
// }

/*
* Helper for setting a bit in AOACxA0 PwrGood Control
* Arg0: bit to set or clear
* Arg1: 0 = clear bit[Arg0], non-zero = set bit[Arg0]
*/
// Method(PWGC,2, Serialized)
// {
//     And (PGA3, 0xdf, Local0)  /* do SwUsb3SlpShutdown below */
//     if(Arg1) {
//         Or(Arg0, Local0, Local0)
//     } else {
//         Not(Arg0, Local1)
//         And(Local1, Local0, Local0)
//     }
//     Store(Local0, PGA3)
//     if(LEqual(Arg0, 0x20)) { /* if SwUsb3SlpShutdown */
//         Store(PGA3, Local0)
//         And(Arg0, Local0, Local0)
//         while(LNot(Local0)) { /* wait SwUsb3SlpShutdown to complete */
//             Store(PGA3, Local0)
//             And(Arg0, Local0, Local0)
//         }
//     }
// }

