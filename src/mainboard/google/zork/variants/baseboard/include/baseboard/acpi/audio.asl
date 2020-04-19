/*
 * This file is part of the coreboot project.
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

Scope (EC0.CREC) {
	#include <ec/google/chromeec/acpi/codec.asl>
}

/* machine driver */
Device (I2SM)
{
	Name (_HID, "AMDI5682")
	Name (_UID, 1)
	Name (_DDN, "I2S machine Driver")

	Name (_CRS, ResourceTemplate ()
	{
#if CONFIG(BOARD_GOOGLE_BASEBOARD_DALBOZ)
		/* DMIC select GPIO */
		GpioIo (Exclusive, PullDefault, 0x0000, 0x0000,
			IoRestrictionNone, "\\_SB.GPIO", 0x00,
			ResourceConsumer,,) { 6 }
#else
		/* DMIC select GPIO */
		GpioIo (Exclusive, PullDefault, 0x0000, 0x0000,
			IoRestrictionNone, "\\_SB.GPIO", 0x00,
			ResourceConsumer,,) { 13 }
#endif
	})
	/* Device-Specific Data */
	Name (_DSD, Package ()
	{
		ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
		Package ()
		{
			Package ()
			{
				"dmic-gpio", Package () { ^I2SM, 0, 0, 0 }
			}
		}

	})
	Method (_STA)
	{
		Return (0xF)
	}
}
