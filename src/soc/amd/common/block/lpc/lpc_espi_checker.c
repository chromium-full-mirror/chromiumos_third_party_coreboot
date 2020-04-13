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

#include <device/device.h>
#include <device/pci.h>
#include <device/pnp.h>
#include <device/pci_ids.h>
#include <soc/pci_devs.h>
#include <device/pci_ops.h>
#include <soc/espi.h>

#include <amdblocks/lpc.h>
#include <amdblocks/lpc_espi_checker.h>

#include <assert.h>
#include <stdint.h>

typedef struct {
	unsigned char enabled;
	u32 bit_mask;
	u16 start;
	u16 end;
} lpc_range_t;

lpc_range_t io_port_ranges[] = {
	/* These are from the first register: LPC_IO_PORT_DECODE_ENABLE */
	{0, DECODE_ENABLE_ADLIB_PORT,     0x388, 0x389},
	{0, DECODE_ENABLE_ACPIUC_PORT,    0x062, 0x062}, /* double map for 62/66 */
	{0, DECODE_ENABLE_ACPIUC_PORT,    0x066, 0x066}, /* double map for 62/66 */
	{0, DECODE_ENABLE_KBC_PORT,       0x060, 0x060}, /* double map for 60/64 */
	{0, DECODE_ENABLE_KBC_PORT,       0x064, 0x064}, /* double map for 60/64 */
	{0, DECODE_ENABLE_GAME_PORT,      0x200, 0x20F},
	{0, DECODE_ENABLE_FDC_PORT1,      0x370, 0x377},
	{0, DECODE_ENABLE_FDC_PORT0,      0x3F0, 0x3F7},
	{0, DECODE_ENABLE_MSS_PORT3,      0xF40, 0xF47},
	{0, DECODE_ENABLE_MSS_PORT2,      0xE80, 0xE87},
	{0, DECODE_ENABLE_MSS_PORT1,      0x604, 0x60b},
	{0, DECODE_ENABLE_MSS_PORT0,      0x530, 0x537},
	{0, DECODE_ENABLE_MIDI_PORT3,     0x330, 0x331},
	{0, DECODE_ENABLE_MIDI_PORT2,     0x320, 0x321},
	{0, DECODE_ENABLE_MIDI_PORT1,     0x310, 0x311},
	{0, DECODE_ENABLE_MIDI_PORT0,     0x300, 0x301},
	{0, DECODE_ENABLE_AUDIO_PORT3,    0x280, 0x293},
	{0, DECODE_ENABLE_AUDIO_PORT2,    0x260, 0x273},
	{0, DECODE_ENABLE_AUDIO_PORT1,    0x240, 0x253},
	{0, DECODE_ENABLE_AUDIO_PORT0,    0x230, 0x233},
	{0, DECODE_ENABLE_SERIAL_PORT7,   0x3E8, 0x3EF},
	{0, DECODE_ENABLE_SERIAL_PORT6,   0x338, 0x33F},
	{0, DECODE_ENABLE_SERIAL_PORT5,   0x2E8, 0x2EF},
	{0, DECODE_ENABLE_SERIAL_PORT4,   0x238, 0x23F},
	{0, DECODE_ENABLE_SERIAL_PORT3,   0x228, 0x22F},
	{0, DECODE_ENABLE_SERIAL_PORT2,   0x220, 0x227},
	{0, DECODE_ENABLE_SERIAL_PORT1,   0x2F8, 0x2FF},
	{0, DECODE_ENABLE_SERIAL_PORT0,   0x3F8, 0x3FF},
	{0, DECODE_ENABLE_PARALLEL_PORT5, 0x7BC, 0x7BF},
	{0, DECODE_ENABLE_PARALLEL_PORT4, 0x3BC, 0x3BF},
	{0, DECODE_ENABLE_PARALLEL_PORT3, 0x678, 0x67F},
	{0, DECODE_ENABLE_PARALLEL_PORT2, 0x278, 0x27F},
	{0, DECODE_ENABLE_PARALLEL_PORT1, 0x778, 0x77F},
	{0, DECODE_ENABLE_PARALLEL_PORT0, 0x378, 0x37F}
	/* end first register */
};

#define WIDE2_IDX 0
#define WIDE1_IDX 1
#define WIDE0_IDX 10
lpc_range_t io_mm_port_ranges[] = {
	/* second register, including variable sized ranges - which we will populate later
	 * sourced from LPC_IO_OR_MEM_DECODE_ENABLE */
	{0, LPC_WIDEIO2_ENABLE,          0x0000, 0x0000}, /* dynamic */
	{0, LPC_WIDEIO1_ENABLE,          0x0000, 0x0000}, /* dynamic */
	{0, DECODE_IO_PORT_ENABLE6,      0xFD60, 0xFD6F},
	{0, DECODE_IO_PORT_ENABLE5,      0x4700, 0x470B},
	{0, DECODE_IO_PORT_ENABLE4,      0x0080, 0x0080}, /* post codes !*/
	/* mem port enable is not used for io remap */
	{0, DECODE_IO_PORT_ENABLE3,      0x0580, 0x05BF},
	{0, DECODE_IO_PORT_ENABLE2,      0x0500, 0x053F},
	{0, DECODE_IO_PORT_ENABLE1,      0x0480, 0x04BF},
	{0, DECODE_IO_PORT_ENABLE0,      0x0400, 0x043F},
	/* time count not used for io remap */
	/* timeout counter disable not used for io remap */
	{0, LPC_DECODE_RTC_IO_ENABLE,    0x0070, 0x0073},
	/* memory range not used for io remap */
	/* reserved slot */
	/* reserved slot */
	/* dynamic - doesn't technically live here but lets us access them as an array */
	{0, LPC_WIDEIO0_ENABLE,          0x0000, 0x0000},
	{0, LPC_DECODE_RTC_IO_ENABLE,    0x0070, 0x0073},
	{0, DECODE_ALTERNATE_SIO_ENABLE, 0x004E, 0x004F},
	{0, DECODE_SIO_ENABLE,           0x002E, 0x002F}
	/* end second register */
};

static u8 mem_enabled[2] = {0,0};
static u32 mem_start[2], mem_end[2];

/* populates the enable bit from the control register supplied */
static void populate_enables(u32 reg, lpc_range_t array[], u32 size)
{
	int i;
	for (i=0; i<size; i++) {
		if (array[i].bit_mask & reg) {
			array[i].enabled = 1;
		}
	}
}

/*
 * @brief Populate wide IO ranges based on offset + size chooser
 *
 * Fill_out_dynamic_range populates the wide IO ranges based on
 * their base offset + choice to use 512B vs. 16B for the end
 *
 * @param dynamic_ptr pointer into the array of structs
 * @param base_reg_offset Register offset to get the base address
 * @param use_512_range   Boolean to choose between (1): use 512B, (0): use 16B
 */
static void fill_out_dynamic_range(
			    lpc_range_t *dynamic_ptr,
			    u32 base_reg_offset,
			    u8 use_512_range)
{

	u32 base_reg;

	if (dynamic_ptr->enabled) {
		base_reg = pci_read_config32(SOC_LPC_DEV, base_reg_offset);
		/* only use the bottom 16 bits. 31-16 reserved. */
		dynamic_ptr->start = base_reg & 0xFFFF;

		if (use_512_range) {
			/* one nice thing about this mask is I cannot overflow */
			dynamic_ptr->end = dynamic_ptr->start + 512;
		} else {
			dynamic_ptr->end = dynamic_ptr->start + 16;
		}
	}
}



/* overlap comes from the two ranges being disjoint, and then inverting
 * |espi|  |LPC|    OR  | LPC |   |espi|  => implies there is not overlap
 * ((E[1] < L[0]) || (L[1] < E[0])) => there is no overlap
 * or....   (L[0] <= E[1]) && L[1] >= E[0] => there is overlap
 */
static int check_overlap(lpc_range_t ranges[], int ranges_size, u16 espi_start, u16 espi_end)
{
	int i;

	/* go through all of the ranges in LPC space */
	for (i=0; i<ranges_size; i++) {
		/* if this range is enabled, and it overlaps, we're done. Problem. */
		if ( (ranges[i].enabled) &&
		     (ranges[i].start <= espi_end) &&
		     (ranges[i].end >= espi_start) ) {
			printk(BIOS_DEBUG,
			       "ESPI %04x:%04x overlapped LPC Mask[%08x] => %04x:%04x\n",
			       espi_start, espi_end,
			       ranges[i].bit_mask,
			       ranges[i].start, ranges[i].end);

			return ranges[i].bit_mask;
		}
	}

	return 0;
}

/*
 * @brief Checks whether lpc and a target espi range would overlap for IO
 * @param espi_start 16b start IO address
 * @param espi_size  16b size/range for IO
 * @return 0 if there is no overlap. You're good to go. Program that ESPI. \
 *         1 if there is overlap. Don't touch that cfg register!
 */
static int check_lpc_espi_io_overlap (u16 espi_start, u16 espi_size)
{

	ASSERT_MSG(espi_start < (UINT16_MAX - espi_size),
		   "Starting IO address for ESPI would overflow u16");
	u32 espi_end = espi_start + espi_size;
	int overlap;

	overlap = check_overlap(io_port_ranges, ARRAY_SIZE(io_port_ranges), espi_start, espi_end);
	if (overlap) {
		/* we assert here because in dev, we want to stop right away. In the real world,
		 * we should continue the best we can. */
		ASSERT_MSG(0, "IO port range overlap between LPC and eSPI - io_port_range reg");
		return 1;
	}

	/* the io_mm_port ranges reg also has IO in it */
	overlap = check_overlap(io_mm_port_ranges, ARRAY_SIZE(io_mm_port_ranges),
				espi_start, espi_end);
	if (overlap) {
		ASSERT_MSG(0, "IO port range overlap between LPC and eSPI - io_mm_port_range_reg");
		return 1;
	}

	return 0;

}

static int check_lpc_espi_mm_overlap(u32 espi_start, u16 espi_size)
{
	ASSERT_MSG(espi_start < (UINT32_MAX - espi_size), "Starting memory mapped addr for ESPI would overflow u32");
	u32 espi_end = espi_start + espi_size;

	int i;
	for (i=0; i<2; i++) {
		if (mem_enabled[i] &&
		    (mem_start[i] <= espi_end) &&
		    (mem_end[i] >= espi_start) )
		{
			printk(BIOS_DEBUG, "eSPI MMIO %08x:%08x overlaps LPC %08x:%08x",
			       espi_start, espi_end, mem_start[i], mem_end[i]);
			ASSERT_MSG(0, "MMIO range overlap between LPC and eSPI");
			return 1;
		}
	}
	return 0;
}


/*
 * This sets up IO and memory structures to eventually compare against eSPI by sampling the
 * LPC registers:
 *   LPC_IO_PORT_DECODE_ENABLE - io ranges
 *   LPC_IO_OR_MEM_DECODE_ENABLE - io ranges, wide IO, & some indirection to MMIO
 *   LPC_ALT_WIDEIO_RANGE_ENABLE - sizing register for wide IO
 *   LPC_WIDEIO 0,1,2 - WIDE IO Ranges
 *   LPC_MEM_PORT 0,1 - MMIO Ranges controlled by  IO_OR_MEM above.
 *
 * By pre-sampling, we can submit a bunch of comparisons from espi against these pre-populated
 * structs and only have to do 1 set of reg reads for eSPI, and 1 set of reg reads for LPC.
 */
static void gather_lpc_regs(void)
{
	/* read and populate my LPC information */

	u32 lpc_io_reg;
	u32 lpc_mm_io_reg;

	lpc_io_reg = pci_read_config32(SOC_LPC_DEV, LPC_IO_PORT_DECODE_ENABLE);
	populate_enables(lpc_io_reg, io_port_ranges, ARRAY_SIZE(io_port_ranges));

	lpc_mm_io_reg = pci_read_config32(SOC_LPC_DEV, LPC_IO_OR_MEM_DECODE_ENABLE);
	/* if you want to test the checker, you can modify the return values here like so:
	 * lpc_mm_io_reg |= 0x200000; //port 80 - cause a collision
	 */
	populate_enables(lpc_mm_io_reg, io_mm_port_ranges, ARRAY_SIZE(io_mm_port_ranges));


	u32 wide_size_reg = pci_read_config32(SOC_LPC_DEV, LPC_ALT_WIDEIO_RANGE_ENABLE);

	/* this does not follow a 0,1,2 pattern. it's 0,1,3
	 * the struct locations and the ports also don't follow a regular pattern
	 */
	fill_out_dynamic_range(&io_mm_port_ranges[WIDE2_IDX],
			       LPC_WIDEIO2_GENERIC_PORT,
			       wide_size_reg & LPC_ALT_WIDEIO2_ENABLE);

	fill_out_dynamic_range(&io_mm_port_ranges[WIDE1_IDX],
			       LPC_WIDEIO1_GENERIC_PORT,
			       wide_size_reg & LPC_ALT_WIDEIO1_ENABLE);

	fill_out_dynamic_range(&io_mm_port_ranges[WIDE0_IDX],
			       LPC_WIDEIO_GENERIC_PORT,
			       wide_size_reg & LPC_ALT_WIDEIO0_ENABLE);

	/* high order memory range - 4k range */
	if (lpc_mm_io_reg & DECODE_MEM_PORT_ENABLE1) {
		mem_enabled[1] = true;
		mem_start[1] = pci_read_config32(SOC_LPC_DEV, LPC_MEM_PORT1);
		mem_start[1] = mem_start[1] & 0xFFFFF000; /* mask off 11:0 */
		mem_end[1] = mem_start[1] | 0xFFF;        /* add the 4k back on */
	}


	/* low memory range. More dynamically sized */
	if (lpc_mm_io_reg & DECODE_MEM_PORT_ENABLE0) {
		mem_enabled[0] = true;
		u32 port0_reg = pci_read_config32(SOC_LPC_DEV, LPC_MEM_PORT0);
		mem_start[0] = (port0_reg & 0xFFFF) << 16;
		mem_end[0] = (port0_reg | 0xFFFF);
	}
}

/*
 * @brief Checks that LPC vs. ESPI are not overlapping, neither in IO, nor in MMIO ranges
 * @return Can assert/die in early development if there is a fail. (#define controlled) \
 *         Prints error messages otherwise.
 */
void check_lpc_espi_overlap(void)
{
	gather_lpc_regs();

	/* read and populate my eSPI information */
	uint8_t *espi_base = espi_get_bar();
	uint32_t slave0_decode_en = read32(espi_base + ESPI_DECODE);

	/* handle the special bits / ranges in the eSPI register */
	if (slave0_decode_en & ESPI_DECODE_IO_0x80_EN) {
		check_lpc_espi_io_overlap(0x80, 0x0);
	}
	if (slave0_decode_en & ESPI_DECODE_IO_0X60_0X64_EN) {
		check_lpc_espi_io_overlap(0x60, 0x0);
		check_lpc_espi_io_overlap(0x64, 0x0);
	}
	if (slave0_decode_en & ESPI_DECODE_IO_0X2E_0X2F_EN) {
		check_lpc_espi_io_overlap(0x2e, 0x1);
	}


	int i;
	for (i=0; i<4; i++) {
		/* check the io ranges */
		if (slave0_decode_en & ESPI_DECODE_IO_RANGE_EN(i)) {
			uint32_t espi_start = read32(espi_base + ESPI_IO_RANGE_BASE(i));
			u8 espi_size = read8(espi_base + ESPI_IO_RANGE_SIZE(i));
			check_lpc_espi_io_overlap(espi_start, espi_size);
		}

		/* check the mmio ranges */
		if (slave0_decode_en & ESPI_DECODE_MMIO_RANGE_EN(i)) {
			uint32_t espi_start = read32(espi_base + ESPI_MMIO_RANGE_BASE(i));
			u16 espi_size = read16(espi_base + ESPI_MMIO_RANGE_SIZE(i));
			check_lpc_espi_mm_overlap(espi_start, espi_size);

		}
	}
}
