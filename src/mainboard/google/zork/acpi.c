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

#include <arch/acpi.h>
#include <arch/smp/mpspec.h>
#include <soc/acpi.h>

unsigned long acpi_mb_madt_irqoverride(unsigned long current)
{
	/*
	 * IRQ 1 is used by the keyboard. The keyboard is provided by the EC
	 * via eSPI. The EC is configured to send a Active Level High interrupt.
	 * We need to override the default Active Edge Low default.
	 * The eSPI message the IOAPIC receives is actually inverted, so we
	 * set this as an Active Low interrupt.
	 */
	current += acpi_create_madt_irqoverride(
		(acpi_madt_irqoverride_t *)current, 0, 1, 1,
		MP_IRQ_TRIGGER_LEVEL | MP_IRQ_POLARITY_LOW);

	return current;
}
