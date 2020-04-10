/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stdint.h>
#include <arch/io.h>
#include <device/mmio.h>
#include <amdblocks/acpimmio.h>

static uintptr_t iomux_bar;

void iomux_set_bar(void *bar)
{
	iomux_bar = (uintptr_t)bar;
}

u8 iomux_read8(u8 reg)
{
	return read8((void *)(iomux_bar + reg));
}

u16 iomux_read16(u8 reg)
{
	return read16((void *)(iomux_bar + reg));
}

u32 iomux_read32(u8 reg)
{
	return read32((void *)(iomux_bar + reg));
}

void iomux_write8(u8 reg, u8 value)
{
	write8((void *)(iomux_bar + reg), value);
}

void iomux_write16(u8 reg, u16 value)
{
	write16((void *)(iomux_bar + reg), value);
}

void iomux_write32(u8 reg, u32 value)
{
	write32((void *)(iomux_bar + reg), value);
}

static uintptr_t misc_bar;

void misc_set_bar(void *bar)
{
	misc_bar = (uintptr_t)bar;
}

u8 misc_read8(u8 reg)
{
	return read8((void *)(misc_bar + reg));
}

u16 misc_read16(u8 reg)
{
	return read16((void *)(misc_bar + reg));
}

u32 misc_read32(u8 reg)
{
	return read32((void *)(misc_bar + reg));
}

void misc_write8(u8 reg, u8 value)
{
	write8((void *)(misc_bar + reg), value);
}

void misc_write16(u8 reg, u16 value)
{
	write16((void *)(misc_bar + reg), value);
}

void misc_write32(u8 reg, u32 value)
{
	write32((void *)(misc_bar + reg), value);
}

static uintptr_t gpio_bar;

void gpio_set_bar(void *bar)
{
	gpio_bar = (uintptr_t)bar;
}

void *gpio_get_bar(void)
{
	return (void *)gpio_bar;
}

static uintptr_t aoac_bar;

void aoac_set_bar(void *bar)
{
	aoac_bar = (uintptr_t)bar;
}

u8 aoac_read8(u8 reg)
{
	return read8((void *)(aoac_bar + reg));
}

void aoac_write8(u8 reg, u8 value)
{
	write8((void *)(aoac_bar + reg), value);
}

static uintptr_t io_bar;

void io_set_bar(void *bar)
{
	io_bar = (uintptr_t)bar;
}

u8 io_read8(u16 reg)
{
	return read8((void *)(io_bar + reg));
}

void io_write8(u16 reg, u8 value)
{
	write8((void *)(io_bar + reg), value);
}
