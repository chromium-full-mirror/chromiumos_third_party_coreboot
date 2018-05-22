/*
 * This file is part of the coreboot project.
 *
 * Copyright 2018 Google LLC
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

Scope (\_SB.PCI0.XHCI.RHUB.HS02)
{
	/* USB-A Rear Right Port USB 2.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		Zero,	/* USB Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* Visible */
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One))
	}
	/* D0 state - VBUS is on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (2)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (2)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.HS03)
{
	/* USB-A Front Right Port USB 2.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		Zero,	/* USB Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* Visible */
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One))
	}
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (3)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (3)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.HS04)
{
	/* USB-A Front Left Port USB 2.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		Zero,	/* USB Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* Visible */
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One))
	}
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (4)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (4)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.HS05)
{
	/* USB-A Rear Left Port USB 2.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		Zero,	/* USB Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* Visible */
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One))
	}
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (0)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (0)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.HS06)
{
	/* USB-A Rear Middle Port USB 2.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		Zero,	/* USB Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* Visible */
	Method (_PLD, 0, NotSerialized)
	{
		Return (GPLD (One))
	}
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (1)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (1)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.SS01)
{
	/* USB-C Rear Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
}
Scope (\_SB.PCI0.XHCI.RHUB.SS02)
{
	/* USB-A Rear Right Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (2)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (2)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.SS03)
{
	/* USB-A Front Right Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (3)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (3)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.SS04)
{
	/* USB-A Front Left Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (4)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (4)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.SS05)
{
	/* USB-A Rear Left Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (0)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (0)
	}
}
Scope (\_SB.PCI0.XHCI.RHUB.SS06)
{
	/* USB-A Rear Middle Port USB 3.0 */
	Name (_UPC, Package (0x04)
	{
		0xFF,	/* Connectable */
		0x03,	/* USB 3.0 Port */
		Zero,	/* Reserved */
		Zero	/* Reserved */
	})
	/* D0 state - VBUS on */
	Method (_PS0)
	{
		\_SB.PCI0.LPCB.EC0.UPPS (1)
	}
	/* D3 state - VBUS off */
	Method (_PS3)
	{
		\_SB.PCI0.LPCB.EC0.UPPC (1)
	}
}
