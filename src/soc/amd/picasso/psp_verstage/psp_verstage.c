/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <arch/exception.h>
#include <arch/hlt.h>
#include <arch/io.h>
#include <security/vboot/vboot_common.h>
#include <vendorcode/amd/fsp/picasso/bl_syscall_public.h>
#include <vendorcode/amd/fsp/picasso/bl_errorcodes_public.h>
#include <delay.h>
#include <reset.h>
#include <boot_device.h>
#include <console/console.h>
#include <console/streams.h>
#include <security/vboot/vboot_common.h>
#include <amdblocks/acpimmio.h>
#include <bootmode.h>
#include <timer.h>
#include <halt.h>
#include <string.h>
#include <timestamp.h>
#include <assert.h>
#include <stdint.h>
#include <soc/i2c.h>
#include <vb2_api.h>
#include <2recovery_reasons.h>
#include <security/vboot/symbols.h>
#include <security/vboot/misc.h>
#include <soc/espi.h>
#include <soc/i2c.h>
#include <lib.h>
#include "psp_verstage.h"
#include <soc/southbridge.h>

#define RUN_PSP_SVC_TESTS 0

static struct mem_region_device boot_dev =
		MEM_REGION_DEV_RO_INIT(NULL, CONFIG_ROM_SIZE);

void __weak verstage_mainboard_init(void) { }

static void test_svc_calls(void)
{
	struct SPIROM_INFO spi = {0};
	uint32_t *addr = NULL;

	/* Test svc_get_spi_rom_info & svc_map_spi_rom */
	svc_debug_print("\nTest: Getting SPI ROM info.\n");
	if (svc_get_spi_rom_info(&spi))
		svc_debug_print("Error getting SPI ROM info.\n");

	if (spi.SpiBiosSmnBase != 0)
		if (svc_map_spi_rom(spi.SpiBiosSmnBase, CONFIG_ROM_SIZE, (void **)&addr))
			svc_debug_print("Error mapping SPI ROM to address.\n");
	printk(BIOS_DEBUG,"SPI ROM info:\n");
	printk(BIOS_DEBUG, "SpiBiosSysHubBase: %p\n", spi.SpiBiosSysHubBase);
	printk(BIOS_DEBUG, "SpiBiosSmnBase: %p\n", spi.SpiBiosSmnBase);
	printk(BIOS_DEBUG,"SpiBiosSize: 0x%08x\n", spi.SpiBiosSize);
	printk(BIOS_DEBUG, "addr: %p\n", addr);

	/* Test svc_debug_print_ex, svc_read_timer_val, svc_wait_10ns_multiple, and svc_delay_in_usec */
	uint64_t timer1 = 0;
	uint64_t timer2 = 0;
	printk(BIOS_DEBUG,"\nTest: Reading chrono timer twice, delaying 10 uSec, then printing both values\n");
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer1);
	svc_wait_10ns_multiple(10000);
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer2);

	printk(BIOS_DEBUG,"Chrono timer info:\n");
	printk(BIOS_DEBUG,"Initial timer: 0x%08x_%08x\n", (uint32_t)(timer1>>32),(uint32_t)timer1);
	printk(BIOS_DEBUG,"2nd timer: 0x%08x_%08x\n", (uint32_t)(timer2>>32),(uint32_t)timer2);
	printk(BIOS_DEBUG,"Difference: 0x%08x_%08x\n", (uint32_t)((timer2-timer1)>>32),(uint32_t)(timer2-timer1));
	svc_debug_print_ex((uint32_t)(timer1>>32),(uint32_t)timer1,(uint32_t)(timer2>>32),(uint32_t)timer2);

	printk(BIOS_DEBUG,"\nTest: Reading RTC timer twice, delaying 1 Sec, then printing both values\n");
	svc_read_timer_val(PSP_TIMER_TYPE_RTC, &timer1);
	svc_delay_in_usec(1000000);
	svc_read_timer_val(PSP_TIMER_TYPE_RTC, &timer2);

	printk(BIOS_DEBUG,"RTC timer info:\n");
	printk(BIOS_DEBUG,"Initial timer: 0x%08x_%08x\n", (uint32_t)(timer1>>32),(uint32_t)timer1);
	printk(BIOS_DEBUG,"2nd timer: 0x%08x_%08x\n", (uint32_t)(timer2>>32),(uint32_t)timer2);
	printk(BIOS_DEBUG,"Difference: 0x%08x_%08x\n", (uint32_t)((timer2-timer1)>>32),(uint32_t)(timer2-timer1));
	svc_debug_print_ex((uint32_t)(timer1>>32),(uint32_t)timer1,(uint32_t)(timer2>>32),(uint32_t)timer2);

	/* Test svc_get_boot_mode */
	uint32_t bootmode = 0;
	printk(BIOS_DEBUG,"\nTest: Getting boot mode.\n");
	if (svc_get_boot_mode(&bootmode))
		printk(BIOS_DEBUG,"Error getting boot mode.\n");

	if (bootmode == PSP_BOOT_MODE_S0)
		printk(BIOS_DEBUG,"Platform is NOT resuming.  Booting From S0?\n");
	else if (bootmode == PSP_BOOT_MODE_S5_COLD)
		printk(BIOS_DEBUG,"Platform is NOT resuming. Cold boot\n");
	else if (bootmode == PSP_BOOT_MODE_S5_COLD)
		printk(BIOS_DEBUG,"Platform is NOT resuming. Warm boot\n");
	else if (bootmode == PSP_BOOT_MODE_S3_RESUME)
		printk(BIOS_DEBUG,"Platform is resuming from S3.\n");
	else if (bootmode == PSP_BOOT_MODE_S0i3_RESUME)
		printk(BIOS_DEBUG,"Platform is resuming from S0i3.\n");
	else if (bootmode == PSP_BOOT_MODE_S4)
		printk(BIOS_DEBUG,"S4?  Really?\n");

	printk(BIOS_DEBUG,"\nTest: Saving workbuf\n");
	svc_save_uapp_data(UAPP_COPYBUF_CHROME_WORKBUF, _vboot2_work, VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);

}

static uintptr_t *map_spi_rom(void)
{
	uintptr_t *addr = NULL;
	struct SPIROM_INFO spi = {0};

	printk(BIOS_DEBUG,"Getting SPI ROM info.\n");
	if (svc_get_spi_rom_info(&spi))
		printk(BIOS_DEBUG,"Error getting SPI ROM info.\n");

	if (spi.SpiBiosSmnBase != 0)
		if (svc_map_spi_rom(spi.SpiBiosSmnBase, CONFIG_ROM_SIZE, (void **)&addr))
			printk(BIOS_DEBUG,"Error mapping SPI ROM to address.\n");

	return addr;
}

static void i2c3_set_bar(void *bar)
{
	i2c_set_bar(3, (uintptr_t)bar);
}

static void i2c2_set_bar(void *bar)
{
	i2c_set_bar(2, (uintptr_t)bar);
}

static struct {
	const char *name;
	struct {
		FCH_IO_DEVICE device;
		uint32_t arg0;
	} args;
	void (*set_bar)(void *bar);
	void *_bar;
} bar_map[] = {
	{"IOMUX", {FCH_IO_DEVICE_IOMUX}, iomux_set_bar},
	{"MISC", {FCH_IO_DEVICE_MISC}, misc_set_bar},
	{"GPIO", {FCH_IO_DEVICE_GPIO}, gpio_set_bar},
	{"IO", {FCH_IO_DEVICE_IOPORT}, io_set_bar},
	{"eSPI", {FCH_IO_DEVICE_ESPI}, espi_set_bar},
	{"I2C2", {FCH_IO_DEVICE_I2C, 2}, i2c2_set_bar},
	{"I2C3", {FCH_IO_DEVICE_I2C, 3}, i2c3_set_bar},
};

static uint32_t unmap_fch_devices(void)
{
	void *bar;
	uint32_t err, rtn = BL_UAPP_OK;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bar_map); ++i) {
		bar = bar_map[i]._bar;
		if (!bar)
			continue;

		err = svc_unmap_fch_dev(bar_map[i].args.device, bar);
		if (err) {
			printk(BIOS_ERR, "Failed to unmap %s: %u\n", bar_map[i].name, err);
			rtn = err;
		} else {
			printk(BIOS_DEBUG, "%s unmapped\n", bar_map[i].name);
			bar_map[i]._bar = NULL;
			bar_map[i].set_bar(NULL);
		}
	}

	return rtn;
}

static void reboot_into_recovery(struct vb2_context *ctx, uint32_t subcode)
{
	subcode += PSP_VBOOT_ERROR_SUBCODE;
	post_code(subcode);

	vb2api_fail(ctx, VB2_RECOVERY_RO_UNSPECIFIED, (int)subcode);
	vboot_save_data(ctx);

	printk(BIOS_ERR, "Rebooting into recovery: %#x\n", (unsigned int)subcode);
	vboot_reboot();
}

static uint32_t map_fch_devices(void)
{
	void *bar;
	uint32_t err;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bar_map); ++i) {
		printk(BIOS_DEBUG, "Mapping %s\n", bar_map[i].name);
		err = svc_map_fch_dev(bar_map[i].args.device, bar_map[i].args.arg0, 0, &bar);
		if (err) {
			printk(BIOS_ERR, "Failed to map %s: %u\n", bar_map[i].name, err);
			return err;
		}

		printk(BIOS_DEBUG, "%s mapped to 0x%p\n", bar_map[i].name, bar);
		bar_map[i]._bar = bar;
		bar_map[i].set_bar(bar);
	}

	return BL_UAPP_OK;
}

/*
 * Tell the PSP where to load the rest of the firmware from
 */
static uint32_t update_boot_region(struct vb2_context *ctx)
{
	struct psp_ef_table *ef_table;
	uint32_t psp_dir_addr, bios_dir_addr;
	uint32_t *psp_dir_in_spi, *bios_dir_in_spi;

	/* Continue booting from RO */
	if (ctx->flags & VB2_CONTEXT_RECOVERY_MODE) {
		printk(BIOS_ERR, "In recovery mode. Staying in RO.\n");
		return 0;
	}

	if (vboot_is_firmware_slot_a(ctx)) {
		printk(BIOS_SPEW, "Using FMAP RW_A region.\n");
		ef_table = (struct psp_ef_table *)((CONFIG_PICASSO_FW_A_POSITION &
						SPI_ADDR_MASK) + (uint32_t)boot_dev.base);
	} else {
		printk(BIOS_SPEW, "Using FMAP RW_B region.\n");
		ef_table = (struct psp_ef_table *)((CONFIG_PICASSO_FW_B_POSITION &
						SPI_ADDR_MASK) + (uint32_t)boot_dev.base);
	}

	if (ef_table->signature != EMBEDDED_FW_SIGNATURE) {
		printk(BIOS_ERR, "Error: ROMSIG address is not correct.\n");
		return POSTCODE_ROMSIG_MISMATCH_ERROR;
	}

	psp_dir_addr = ef_table->psp_table;
	bios_dir_addr = ef_table->bios1_entry;
	psp_dir_in_spi = (uint32_t *)((psp_dir_addr & SPI_ADDR_MASK) + (uint32_t)boot_dev.base);
	bios_dir_in_spi = (uint32_t *)((bios_dir_addr & SPI_ADDR_MASK) + (uint32_t)boot_dev.base);
	if (*psp_dir_in_spi != PSP_COOKIE) {
		printk(BIOS_ERR, "Error: PSP Directory address is not correct.\n");
		return POSTCODE_PSP_COOKIE_MISMATCH_ERROR;
	}
	if (*bios_dir_in_spi != BDT1_COOKIE) {
		printk(BIOS_ERR, "Error: BIOS Directory address is not correct.\n");
		return POSTCODE_BDT1_COOKIE_MISMATCH_ERROR;
	}

	if (svc_update_psp_bios_dir((void *)&psp_dir_addr,
			(void *)&bios_dir_addr, DIR_OFFSET_SET)) {
		printk(BIOS_ERR, "Error: Updated BIOS Directory could not be set.\n");
		return POSTCODE_UPDATE_PSP_BIOS_DIR_ERROR;
	}

	return 0;
}

/*
 * Save workbuf (and soon memory console and timestamps) to the bootloader to pass
 * back to coreboot.
 */
static uint32_t save_buffers(struct vb2_context **ctx)
{
	uint32_t retval;
	uint32_t buffer_size = DEFAULT_WORKBUF_TRANSFER_SIZE;
	uint32_t max_buffer_size;

	/*
	 * This should never fail, but if it does, we should still try to
	 * save the buffer. If that fails, then we should go to recovery mode.
	 */
	if (svc_get_max_workbuf_size(&max_buffer_size)) {
		post_code(POSTCODE_DEFAULT_BUFFER_SIZE_NOTICE);
		printk(BIOS_NOTICE,"Notice: using default transfer buffer size.\n");
		max_buffer_size = DEFAULT_WORKBUF_TRANSFER_SIZE;
	}
	printk(BIOS_DEBUG,"\nMaximum buffer size: %d bytes\n", max_buffer_size);

	retval = vb2api_relocate(_vboot2_work, _vboot2_work, buffer_size, ctx);
	if (retval != VB2_SUCCESS) {
		printk(BIOS_ERR, "Error shrinking workbuf. Error code %#x\n", retval);
		buffer_size = VB2_FIRMWARE_WORKBUF_RECOMMENDED_SIZE;
		post_code(POSTCODE_WORKBUF_RESIZE_WARNING);
	}

	if (buffer_size > max_buffer_size) {
		printk(BIOS_ERR, "Error: Workbuf is larger than max buffer size.\n");
		post_code(POSTCODE_WORKBUF_BUFFER_SIZE_ERROR);
		return(POSTCODE_WORKBUF_BUFFER_SIZE_ERROR);
	}

	retval = svc_save_uapp_data(UAPP_COPYBUF_CHROME_WORKBUF, (void *)_vboot2_work,
			buffer_size);
	if (retval) {
		printk(BIOS_ERR, "Error: Could not save workbuf. Error code 0x%08x\n",
				retval);
		return(POSTCODE_WORKBUF_SAVE_ERROR);
	}

	return 0;
}

static void sb_enable_legacy_io(void)
{
	pm_io_write32(PM_DECODE_EN, pm_io_read32(PM_DECODE_EN) | LEGACY_IO_EN);
}

extern char _bss_start, _bss_end;

void Main(void)
{
	uint32_t retval;
	struct vb2_context *ctx = NULL;

	/* Do not use printk() before verstage_mainboard_init() is called */
	svc_debug_print("Entering verstage on PSP\n");
	memset(&_bss_start, '\0', &_bss_end - &_bss_start);

	console_init();

	printk(BIOS_DEBUG, "Mapping devices\n");

	retval = map_fch_devices();
	if (retval) {
		printk(BIOS_DEBUG, "Failed to map FCH devices: %u\n", retval);
		goto err;
	}

	sb_enable_legacy_io();

	svc_write_postcode(0x01);

	verstage_mainboard_init();

	svc_write_postcode(0x02);

	verstage_main();

	svc_write_postcode(0x03);

	if (RUN_PSP_SVC_TESTS)
		test_svc_calls();

	post_code(POSTCODE_SAVE_BUFFERS);
	retval = save_buffers(&ctx);
	if (retval)
		goto err;

	post_code(POSTCODE_UPDATE_BOOT_REGION);
	retval = update_boot_region(ctx);
	if (retval)
		goto err;

	if (boot_dev.base){
		printk(BIOS_DEBUG,"Unmapping SPI rom\n");
		if (svc_unmap_spi_rom((void *)boot_dev.base))
			printk(BIOS_ERR,"Error unmapping SPI rom\n");
	}

	svc_write_postcode(0xF1);
	unmap_fch_devices();

	svc_write_postcode(0xF2);

	printk(BIOS_DEBUG, "Leaving verstage on PSP\n");
	svc_exit(retval);

err:
	reboot_into_recovery(ctx, retval);
	return;
}

void udelay(uint32_t usecs)
{
	svc_delay_in_usec(usecs);
}

int vboot_platform_is_resuming(void)
{
	uint32_t bootmode = 0;
	if (svc_get_boot_mode(&bootmode)) {
		printk(BIOS_ERR,"Error getting boot mode. Assuming no resume.\n");
		return 0;
	}

	if (bootmode == PSP_BOOT_MODE_S3_RESUME || bootmode == PSP_BOOT_MODE_S0i3_RESUME)
		return 1;

	return 0;
}

const struct region_device *boot_device_ro(void)
{
	uintptr_t *addr;

	addr = map_spi_rom();
	mem_region_device_ro_init(&boot_dev, (void *)addr, CONFIG_ROM_SIZE);

	return &boot_dev.rdev;
}

int do_printk(int msg_level, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = do_vprintk(msg_level, fmt, args);
	va_end(args);

	return i;
}

int do_vprintk(int msg_level, const char *fmt, va_list args)
{
	int i, log_this;
	char buf[256];

	log_this = console_log_level(msg_level);
	if (log_this < CONSOLE_LOG_FAST)
		return 0;

	i = vsnprintf(buf, sizeof(buf), fmt, args);
	svc_debug_print(buf);
	return i;
}

void console_hw_init(void)
{
	// Nothing to init for svc_debug_print
}

void timer_monotonic_get(struct mono_time *mt)
{
	/* Chrono timer is based on a 100MHz clock, so 1 tick is 10ns */
	uint64_t clk;

	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &clk);

	// TODO: Look at better ways to calculate this
	mt->microseconds = clk / 100;
}

void do_board_reset(void)
{
	printk(BIOS_ERR,"Resetting the board now.\n");
	svc_reset_system(RESET_TYPE_COLD);
}

void post_code(u8 value)
{
	if (CONFIG(CONSOLE_POST))
		printk(BIOS_WARNING, "Post code: 0x%02x\n", value);
	if (CONFIG(POST_IO) && CONFIG_POST_IO_PORT == 0x80)
		svc_write_postcode(value);
}

/* Stubs that still need to be implemented */

void timestamp_add_now(enum timestamp_id id)
{
	return;
}

void timestamp_add(enum timestamp_id id, uint64_t ts)
{
	return;
}

uint64_t timestamp_get(void)
{
	return 0;
}
