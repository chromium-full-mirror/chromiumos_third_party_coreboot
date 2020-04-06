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
#include <drivers/i2c/designware/dw_i2c.h>
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
#include <security/vboot/symbols.h>
#include <soc/espi.h>

#define RUN_PSP_SVC_TESTS 0

static struct mem_region_device boot_dev =
		MEM_REGION_DEV_RO_INIT(NULL, CONFIG_ROM_SIZE);
static void *i2c_bus_addr[I2C_DEVICE_COUNT];

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

/* Unmap the I2C ports so that they can be used in coreboot */
static void unmap_i2c(void)
{
	for (int i = 0; i < I2C_DEVICE_COUNT; i++) {
		if (i2c_bus_addr[i] != NULL) {
			if (svc_unmap_fch_dev(FCH_IO_DEVICE_I2C, i2c_bus_addr[i])) {
				printk(BIOS_ERR, "Error unmapping I2c %d.\n", i);
			} else {
				i2c_bus_addr[i] = NULL;
			}
		}
	}
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

static struct {
	const char *name;
	FCH_IO_DEVICE device;
	void (*set_bar)(void *bar);
	void *_bar;
} bar_map[] = {
	{"IOMUX", FCH_IO_DEVICE_IOMUX, iomux_set_bar},
	{"MISC", FCH_IO_DEVICE_MISC, misc_set_bar},
	{"GPIO", FCH_IO_DEVICE_GPIO, gpio_set_bar},
	{"IO", FCH_IO_DEVICE_IOPORT, io_set_bar},
	{"eSPI", FCH_IO_DEVICE_ESPI, espi_set_bar},
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

		err = svc_unmap_fch_dev(bar_map[i].device, bar);
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

static uint32_t map_fch_devices(void)
{
	void *bar;
	uint32_t err;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(bar_map); ++i) {
		printk(BIOS_DEBUG, "Mapping %s\n", bar_map[i].name);
		err = svc_map_fch_dev(bar_map[i].device, 0, 0, &bar);
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

extern char _bss_start, _bss_end;

void Main(void)
{
	uint32_t retval = 0;

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

	svc_write_postcode(0x01);

	verstage_mainboard_init();

	svc_write_postcode(0x02);

	verstage_main();

	svc_write_postcode(0x03);

	#if 0 //TODO
	uint32_t *psp_dir_offset = NULL;
	uint32_t *bios_dir_offset = NULL;
	uint32_t *workbuf_addr = NULL;

	if (svcc_save_uapp_data(UAPP_COPYBUF_CHROME_WORKBUF, workbuf_addr,
			VB2_FIRMWARE_WORKBUF_RECOMMENDED_SIZE))
		printk(BIOS_ERR,"Error: could not save workbuf\n");

	svc_update_psp_bios_dir(psp_dir_offset, bios_dir_offset, DIR_OFFSET_SET);
	#endif

	if (RUN_PSP_SVC_TESTS)
		test_svc_calls();

err:
	if (boot_dev.base){
		printk(BIOS_DEBUG,"Unmapping SPI rom\n");
		if (svc_unmap_spi_rom((void *)boot_dev.base))
			printk(BIOS_ERR,"Error unmapping SPI rom\n");
	}

	svc_write_postcode(0xF1);
	unmap_fch_devices();

	svc_write_postcode(0xF2);
	unmap_i2c();

	svc_write_postcode(0xF3);

	printk(BIOS_DEBUG, "Leaving verstage on PSP\n");
	svc_exit(retval);
	return;
}

void udelay(uint32_t usecs)
{
	svc_delay_in_usec(usecs);
}


int vboot_platform_is_resuming(void)
{
	uint32_t bootmode = 0;
	if (svc_get_boot_mode(&bootmode))
		printk(BIOS_ERR,"Error getting boot mode.\n");

	if ((bootmode == PSP_BOOT_MODE_S3_RESUME) || (bootmode == PSP_BOOT_MODE_S0i3_RESUME))
		bootmode=1;

	return bootmode;
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

uintptr_t dw_i2c_base_address(uint32_t bus)
{
	if (bus < 2 || bus > 3)
		return 0;

	if (i2c_bus_addr[bus] == NULL)
		if (svc_map_fch_dev(FCH_IO_DEVICE_I2C, bus, 0, &i2c_bus_addr[bus]))
			printk(BIOS_ERR, "Error: Could not map I2c bus %d.", bus);

	return (uintptr_t)i2c_bus_addr[bus];
}

void do_board_reset(void)
{
	svc_reset_system(RESET_TYPE_WARM);
}

void post_code(u8 value)
{
	if (CONFIG(CONSOLE_POST))
		printk(BIOS_WARNING, "Post code: 0x%02x", value);
	if (CONFIG(POST_IO) && CONFIG_POST_IO_PORT == 0x80)
		svc_write_postcode(value);
}

/* Stubs that still need to be implemented */

int get_recovery_mode_switch(void)
{
	return 0;
}
int get_lid_switch(void)
{
	return 0;
}


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
