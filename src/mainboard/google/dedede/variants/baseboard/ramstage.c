/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <acpi/acpi_device.h>
#include <baseboard/variants.h>
#include <console/console.h>
#include <drivers/usb/acpi/chip.h>
#include <fw_config.h>
#include <gpio.h>
#include <soc/pci_devs.h>
#include <ec/google/chromeec/ec.h>
#include <device/pci_ops.h>
#include <intelblocks/power_limit.h>
#include <chip.h>
#include <drivers/intel/dptf/chip.h>
#include <soc/pci_devs.h>

#define LTE_USB_PORT_ID 3
#define LTE_USB_PORT_TYPE 2

void update_lte_device(struct acpi_gpio *lte_reset_gpio, struct acpi_gpio *lte_enable_gpio)
{
	struct device *xhci, *hub = NULL, *port = NULL;
	struct drivers_usb_acpi_config *config;

	xhci = pcidev_path_on_root(PCH_DEVFN_XHCI);
	if (!xhci) {
		printk(BIOS_ERR, "%s: Could not locate XHCI device in DT\n", __func__);
		return;
	}

	while ((hub = dev_bus_each_child(xhci->link_list, hub)) != NULL) {
		while ((port = dev_bus_each_child(hub->link_list, port)) != NULL) {
			if (!port->chip_info || port->path.usb.port_id != LTE_USB_PORT_ID)
				continue;

			if (fw_config_probe(FW_CONFIG(DB_PORTS, DB_PORTS_LTE_HDMI)) ||
				fw_config_probe(FW_CONFIG(DB_PORTS, DB_PORTS_1C_LTE)) ||
				fw_config_probe(FW_CONFIG(DB_PORTS, DB_PORTS_1A_HDMI_LTE))) {

				config = port->chip_info;
				config->type = UPC_TYPE_INTERNAL;
				if (port->path.usb.port_type == LTE_USB_PORT_TYPE) {
					config->has_power_resource = 1;
					memcpy(&config->reset_gpio, lte_reset_gpio,
								sizeof(config->reset_gpio));
					config->reset_off_delay_ms = 20;
					memcpy(&config->enable_gpio, lte_enable_gpio,
								sizeof(config->enable_gpio));
					config->enable_delay_ms = 20;
				}
			}
		}
	}
}

#define SET_PSYSPL2(e, w) ((e) * (w) / 100)
#define MICROWATTS_TO_WATTS 1000000

static bool get_sku_index(size_t *intel_idx)
{
	uint16_t mch_id = pci_s_read_config16(PCI_DEV(0, 0, 0), PCI_DEVICE_ID);
	uint8_t tdp = get_cpu_tdp();
	size_t i = 0;

	if (mch_id != 0xFFFF) {
		for (i = 0; i < ARRAY_SIZE(cpuid_to_jsl); i++) {
			if (mch_id == cpuid_to_jsl[i].pci_did &&
					tdp == cpuid_to_jsl[i].cpu_tdp) {
				*intel_idx = cpuid_to_jsl[i].limits;
				break;
			}
		}
	}

	if (i == ARRAY_SIZE(cpuid_to_jsl) || mch_id == 0xFFFF) {
		printk(BIOS_ERR, "Cannot find correct intel sku index.\n");
		return false;
	}

	return true;
}

void variant_update_psys_power_limits(const struct psys_config *config_psys)
{
	struct soc_power_limits_config *soc_config;
	size_t intel_idx = 0;
	u16 volts_mv, current_ma;
	enum usb_chg_type type;
	u32 psys_pl2;
	config_t *conf;
	u32 watts;
	int rv;

	if (!get_sku_index(&intel_idx))
		return;

	conf = config_of_soc();
	soc_config = &conf->power_limits_config[intel_idx];

	rv = google_chromeec_get_usb_pd_power_info(&type, &current_ma, &volts_mv);

	if (rv == 0 && type == USB_CHG_TYPE_PD) {
		/* Detected USB-PD. Base on max value of adapter */
		watts = ((u32)current_ma * volts_mv) / MICROWATTS_TO_WATTS;
	} else {
		/* Input type is barrel jack */
		volts_mv = config_psys->bj_volts_mv;
		watts = config_psys->bj_power_w;
	}
	/* Set psyspl2 to 97% of adapter rating */
	psys_pl2 = SET_PSYSPL2(config_psys->efficiency, watts);

	/* voltage unit is milliVolts and current is in milliAmps */
	soc_config->psys_pmax = (u16)(((u32)config_psys->psys_imax_ma * volts_mv) / MICROWATTS_TO_WATTS);
	conf->PsysPmax = soc_config->psys_pmax;

	soc_config->tdp_psyspl2 = psys_pl2;

	printk(BIOS_INFO, "Overriding PsysPL2 (%uW) Psys_Pmax (%uW)\n",
			soc_config->tdp_psyspl2,
			soc_config->psys_pmax);
}
