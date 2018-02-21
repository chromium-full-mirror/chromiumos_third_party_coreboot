
Scope (\_SB.PCI0.XHCI.RHUB)
{
    Name(UPC3, Package() { 0xFF, 0x0A, 0x00, 0x00 }) // Type-C connector - USB2 and SS without Switch

    //
    // Method for creating generic _PLD buffers
    // _PLD contains lots of data, but for purpose of internal validation we care only about
    // ports' visibility and pairing (this requires group position)
    // so these are the only 2 configurable parameters
    //
    Method(GPLD, 2, Serialized) {
      Name(PCKG, Package() { Buffer(0x10) {} } )
      CreateField(DerefOf(Index(PCKG,0)), 0, 7, REV)
      Store(1,REV)
      CreateField(DerefOf(Index(PCKG,0)), 64, 1, VISI)
      Store(Arg0, VISI)
      CreateField(DerefOf(Index(PCKG,0)), 87, 8, GPOS)
      Store(Arg1, GPOS)
      return (PCKG)
    }

    //
    // Method for creating generic _UPC buffers
    // Similar to _PLD, for internal testing we only care about 1 parameter, Connectable
    //
    Method(GUPC, 1, Serialized) {
      Name(PCKG, Package(4) { 0, 0xFF, 0, 0 } )
      Store(Arg0,Index(PCKG,0))
      return (PCKG)
    }


  // USB 2.0 port (visible)
  Scope (\_SB.PCI0.XHCI.RHUB.HS01) {
    Method(_UPC) { Return (UPC3) }  //[type-c]
    Method(_PLD) { Return (GPLD(1,1)) }
  }

  // Camera Module (none visible)
  Scope (\_SB.PCI0.XHCI.RHUB.HS02) {
    Method(_UPC) { Return (GUPC(1)) }
    Method(_PLD) { Return (GPLD(0,2)) }
    //-----------------------------------------------------
    // Fix WHCK Webcam Location test issue. (Mirror issue)
    //-----------------------------------------------------
    Device(WCAM)
    {
      Name(_ADR, 0x02)
      Name(_PLD, Package(1) {
        Buffer (0x14) {
          0x82, 0x00, 0x00, 0x00,     // Revision 2, Ignore color
          0x00, 0x00, 0x00, 0x00,
          0x25, 0x1D, 0x00, 0x00,     // Front Panel, Vertical Upper, Horz. Center, Shape Unknown
          0x00, 0x00, 0x00, 0x00,
          0xFF, 0xFF, 0xFF, 0xFF
        }
      })
    }

  }

  // Bluetooth (none visible)
  Scope (\_SB.PCI0.XHCI.RHUB.HS03) {
    Method(_UPC) { Return (GUPC(1)) }
    Method(_PLD) { Return (GPLD(0,3)) }
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.HS04) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) }
  }

  // USB 2.0 port (visible)
  Scope (\_SB.PCI0.XHCI.RHUB.HS05) {
    Method(_UPC) { Return (UPC3) }  //[type-c]
    Method(_PLD) { Return (GPLD(1,5)) }
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.HS06) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) }
  }

  // H1 TPM (none visible)
  Scope (\_SB.PCI0.XHCI.RHUB.HS07) {
    Method(_UPC) { Return (GUPC(1)) }
    Method(_PLD) { Return (GPLD(0,7)) }
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.HS08) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.HS09) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.HS10) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.USR1) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) }
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.USR2) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) }
  }

  // USB 3.0 port (visible)
  Scope (\_SB.PCI0.XHCI.RHUB.SS01) {
    Method(_UPC) { Return (UPC3) }  //[type-c]
    Method(_PLD) { Return (GPLD(1,1)) } //paired with HS01
  }

  // USB 3.0 port (visible)
  Scope (\_SB.PCI0.XHCI.RHUB.SS02) {
    Method(_UPC) { Return (UPC3) }  //[type-c]
    Method(_PLD) { Return (GPLD(1,5)) } //paired with HS02
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.SS03) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.SS04) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.SS05) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }

  // none
  Scope (\_SB.PCI0.XHCI.RHUB.SS06) {
    Method(_UPC) { Return (GUPC(0)) }
    Method(_PLD) { Return (GPLD(0,0)) } //not connected
  }
}

