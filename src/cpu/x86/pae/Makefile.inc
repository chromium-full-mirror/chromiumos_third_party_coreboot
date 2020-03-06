bootblock-y += pgtbl.c
ifeq ($(CONFIG_ARCH_VERSTAGE_X86_32)$(CONFIG_ARCH_VERSTAGE_X86_64),y)
verstage-y += pgtbl.c
endif
romstage-y += pgtbl.c
postcar-y += pgtbl.c
ramstage-y += pgtbl.c
smm-y += pgtbl.c
