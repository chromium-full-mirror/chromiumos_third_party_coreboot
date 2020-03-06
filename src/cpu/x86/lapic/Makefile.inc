ramstage-y += lapic.c
ramstage-y += lapic_cpu_init.c
ramstage-$(CONFIG_SMP) += secondary.S
romstage-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
ramstage-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
postcar-$(CONFIG_UDELAY_LAPIC) += apic_timer.c
bootblock-y += boot_cpu.c
ifeq ($(CONFIG_ARCH_VERSTAGE_X86_32)$(CONFIG_ARCH_VERSTAGE_X86_64),y)
verstage-y += boot_cpu.c
endif
romstage-y += boot_cpu.c
ramstage-y += boot_cpu.c
postcar-y += boot_cpu.c
