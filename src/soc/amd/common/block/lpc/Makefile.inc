ramstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc.c

romstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_espi_checker.c
ramstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_espi_checker.c

bootblock-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
ifneq ($(CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK),y)
verstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
endif
romstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
postcar-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
ramstage-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
smm-$(CONFIG_SOC_AMD_COMMON_BLOCK_LPC) += lpc_util.c
