bootblock-$(CONFIG_UDELAY_TSC) += delay_tsc.c
ramstage-$(CONFIG_UDELAY_TSC) += delay_tsc.c
romstage-$(CONFIG_TSC_CONSTANT_RATE) += delay_tsc.c
ifeq ($(CONFIG_ARCH_VERSTAGE_X86_32)$(CONFIG_ARCH_VERSTAGE_X86_64),y)
verstage-$(CONFIG_TSC_CONSTANT_RATE) += delay_tsc.c
endif
postcar-$(CONFIG_TSC_CONSTANT_RATE) += delay_tsc.c
smm-$(CONFIG_TSC_CONSTANT_RATE) += delay_tsc.c
