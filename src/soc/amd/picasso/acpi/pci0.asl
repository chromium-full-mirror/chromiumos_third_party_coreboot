/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

Device(PCI0) {
    /* Note: Only need HID on Primary Bus */
    External (TOM1)
    External (TOM2)
    Name(_HID, EISAID("PNP0A08"))	/* PCI Express Root Bridge */
    Name(_CID, EISAID("PNP0A03"))	/* PCI Root Bridge */

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

    /*************************/
    /* Devices */
    #include "pci0_devices.asl"

    /*************************/
    /* Methods */
    #include "pci0_methods.asl"

    /*************************/
    /* Misc */
    External(\_SB.ALIB, MethodObj)

    /*
    * Arg0:device:
    *  5=I2C0, 6=I2C1, 7=I2C2, 8=I2C3, 11=UART0, 12=UART1,
    *  18=EHCI, 23=xHCI, 24=SD
    * Arg1:D-state
    */
    Mutex (FDAS, 0) /* FCH Device AOAC Semophore */

    /*************************/
    /* Named Objects */
    Name(CRES, ResourceTemplate() {
        /* Set the Bus number and Secondary Bus number for the PCI0 device
        * The Secondary bus range for PCI0 lets the system
        * know what bus values are allowed on the downstream
        * side of this PCI bus if there is a PCI-PCI bridge.
        * PCI busses can have 256 secondary busses which
        * range from [0-0xFF] but they do not need to be
        * sequential.
        */
        WordBusNumber (ResourceProducer, MinFixed, MaxFixed, PosDecode,
            0x0000,		/* address granularity */
            0x0000,		/* range minimum */
            0x00ff,		/* range maximum */
            0x0000,		/* translation */
            0x0100,		/* length */
            ,, PSB0)		/* ResourceSourceIndex, ResourceSource, DescriptorName */

        IO(Decode16, 0x0cf8, 0x0cf8, 1,	8)

        WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
            0x0000,		/* address granularity */
            0x0000,		/* range minimum */
            0x0cf7,		/* range maximum */
            0x0000,		/* translation */
            0x0cf8		/* length */
        )
        WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
            0x0000,		/* address granularity */
            0x03b0,		/* range minimum */
            0x03df,		/* range maximum */
            0x0000,		/* translation */
            0x0030		/* length */
        )

        WORDIO(ResourceProducer, MinFixed, MaxFixed, PosDecode, EntireRange,
            0x0000,		/* address granularity */
            0x0d00,		/* range minimum */
            0xffff,		/* range maximum */
            0x0000,		/* translation */
            0xf300		/* length */
        )

        Memory32Fixed(READONLY, 0x000a0000, 0x00020000, VGAM)	/* VGA memory space */
        Memory32Fixed(READONLY, 0x000c0000, 0x00020000, EMM1)	/* Assume C0000-E0000 empty */

        /* memory space for PCI BARs below 4GB */
        Memory32Fixed(ReadOnly, 0x00000000, 0x00000000, MMIO)
    }) /* End Name(_SB.PCI0.CRES) */

    /*************************/
    /* Operation Regions + Fields */

    // OperationRegion(SMIC, SystemMemory, 0xfed80000, 0x80000)
    // Field( SMIC, ByteAcc, NoLock, Preserve) {
    //     /* MISC registers */
    //     offset (0x03ee),
    //     U3PS, 2,  /* Usb3PowerSel */

    //     offset (0x0e28),
    //     ,29 ,
    //     SARP, 1,  /* Sata Ref Clock Powerdown */
    //     U2RP, 1,  /* Usb2 Ref Clock Powerdown */
    //     U3RP, 1,  /* Usb3 Ref Clock Powerdown */

    //     /* AOAC Registers */
    //     offset (0x1e4a), /* I2C0 D3 Control */
    //     I0TD, 2,
    //     , 1,
    //     I0PD, 1,
    //     offset (0x1e4b), /* I2C0 D3 State */
    //     I0DS, 3,

    //     offset (0x1e4c), /* I2C1 D3 Control */
    //     I1TD, 2,
    //     , 1,
    //     I1PD, 1,
    //     offset (0x1e4d), /* I2C1 D3 State */
    //     I1DS, 3,

    //     offset (0x1e4e), /* I2C2 D3 Control */
    //     I2TD, 2,
    //     , 1,
    //     I2PD, 1,
    //     offset (0x1e4f), /* I2C2 D3 State */
    //     I2DS, 3,

    //     offset (0x1e50), /* I2C3 D3 Control */
    //     I3TD, 2,
    //     , 1,
    //     I3PD, 1,
    //     offset (0x1e51), /* I2C3 D3 State */
    //     I3DS, 3,

    //     offset (0x1e56), /* UART0 D3 Control */
    //     U0TD, 2,
    //     , 1,
    //     U0PD, 1,
    //     offset (0x1e57), /* UART0 D3 State */
    //     U0DS, 3,

    //     offset (0x1e58), /* UART1 D3 Control */
    //     U1TD, 2,
    //     , 1,
    //     U1PD, 1,
    //     offset (0x1e59), /* UART1 D3 State */
    //     U1DS, 3,

    //     offset (0x1e60), /* UART2 D3 Control */
    //     U2TD, 2,
    //     , 1,
    //     U2PD, 1,
    //     offset (0x1e61), /* UART2 D3 State */
    //     U2DS, 3,

    //     offset (0x1e71), /* SD D3 State */
    //     SDDS, 3,

    //     offset (0x1e74), /* UART3 D3 Control */
    //     U3TD, 2,
    //     , 1,
    //     U3PD, 1,
    //     offset (0x1e75), /* UART3 D3 State */
    //     U3DS, 3,

    //     offset (0x1e80), /* Shadow Register Request */
    //     , 15,
    //     RQ15, 1,
    //     , 2,
    //     RQ18, 1,
    //     , 4,
    //     RQ23, 1,
    //     RQ24, 1,
    //     , 5,
    //     RQTY, 1,
    //     offset (0x1e84), /* Shadow Register Status */
    //     , 15,
    //     SASR, 1,  /* SATA 15 Shadow Reg Request Status Register */
    //     , 2,
    //     U2SR, 1,  /* USB2 18 Shadow Reg Request Status Register */
    //     , 4,
    //     U3SR, 1,  /* USB3 23 Shadow Reg Request Status Register */
    //     SDSR, 1,  /* SD 24 Shadow Reg Request Status Register */

    //     offset (0x1ea0), /* PwrGood Control */
    //     PG1A, 1,
    //     PG2_, 1,
    //     ,1,
    //     U3PG, 1,  /* Usb3 Power Good BIT3 */

    //     offset (0x1ea3), /* PwrGood Control b[31:24] */
    //     PGA3, 8 ,
    // }

    // OperationRegion(FCFG, SystemMemory, PCBA, 0x01000000)
    // Field(FCFG, DwordAcc, NoLock, Preserve)
    // {
    //     /* XHCI */
    //     Offset(0x00080010), /* Base address */
    //     XHBA, 32,
    //     Offset(0x0008002c), /* Subsystem ID / Vendor ID */
    //     XH2C, 32,

    //     Offset(0x00080048), /* Indirect PCI Index Register */
    //     IDEX, 32,
    //     DATA, 32,
    //     Offset(0x00080054), /* PME Control / Status */
    //     U_PS, 2,

    //     /* EHCI */
    //     Offset(0x00090004), /* Control */
    //     , 1,
    //     EHME, 1,
    //     Offset(0x00090010), /* Base address */
    //     EHBA, 32,
    //     Offset(0x0009002c), /* Subsystem ID / Vendor ID */
    //     EH2C, 32,
    //     Offset(0x00090054), /* EHCI Spare 1 */
    //     EH54, 8,
    //     Offset(0x00090064), /* Misc Control 2 */
    //     EH64, 8,

    //     Offset(0x000900c4), /* PME Control / Status */
    //     E_PS, 2,

    //     /* LPC Bridge */
    //     Offset(0x000a30cb), /* ClientRomProtect[31:24] */
    //     ,  7,
    //     AUSS,  1, /* AutoSizeStart */
    // }
}