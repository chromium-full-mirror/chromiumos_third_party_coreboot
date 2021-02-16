ifeq ($(CONFIG_SOC_AMD_COMMON_BLOCK_SMM),y)

ramstage-y += finalize.c
ramstage-y += smm_relocate.c

endif # CONFIG_SOC_AMD_COMMON_BLOCK_SMM
