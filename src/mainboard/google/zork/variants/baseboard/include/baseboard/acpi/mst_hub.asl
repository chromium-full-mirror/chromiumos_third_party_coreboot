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
	/* TODO: Generate this via Device Tree and remove from here */
	Device (MSTH)
	{
		Name (_HID, "GOOG0012")
		Name (_UID, 1)
		Name (_DDN, "CROS I2C TUNNEL Device")

		/* Device-Specific Data */
		Name (_DSD, Package ()
		{
			ToUUID ("daffd814-6eba-4d8c-8a91-bc9bbf4aa301"),
			Package ()
			{
				Package () { "google,remote-bus", 9 },
			}
		})
	}
}
