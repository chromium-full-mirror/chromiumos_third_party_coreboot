ifeq ($(CONFIG_SOC_QUALCOMM_QCS405),y)

################################################################################
bootblock-y += bootblock.c
bootblock-y += timer.c
bootblock-y += spi.c
bootblock-y += mmu.c
bootblock-y += gpio.c
bootblock-y += clock.c
bootblock-y += i2c.c
bootblock-y += qup.c
bootblock-y += blsp.c
bootblock-$(CONFIG_DRIVERS_UART) += uart.c

################################################################################
verstage-y += timer.c
verstage-y += spi.c
verstage-y += gpio.c
verstage-y += clock.c
verstage-y += i2c.c
verstage-y += qup.c
verstage-y += blsp.c
verstage-$(CONFIG_DRIVERS_UART) += uart.c

################################################################################
romstage-y += timer.c
romstage-y += spi.c
romstage-y += cbmem.c
romstage-y += gpio.c
romstage-y += clock.c
romstage-y += usb.c
romstage-$(CONFIG_DRIVERS_UART) += uart.c
romstage-y += i2c.c
romstage-y += qup.c
romstage-y += blsp.c
romstage-y += mmu.c
romstage-y += ../common/mmu.c
romstage-y += ../common/qclib.c
romstage-y += soc_blob_load.c

################################################################################
ramstage-y += soc.c
ramstage-y += timer.c
ramstage-y += spi.c
ramstage-y += cbmem.c
ramstage-y += gpio.c
ramstage-y += clock.c
ramstage-y += i2c.c
ramstage-y += qup.c
ramstage-y += blsp.c
ramstage-y += usb.c
ramstage-$(CONFIG_DRIVERS_UART) += uart.c
ramstage-y += rpm_load_reset.c
ramstage-y += bl31_plat_params.c

################################################################################

CPPFLAGS_common += -Isrc/soc/qualcomm/qcs405/include
CPPFLAGS_common += -Isrc/soc/qualcomm/common/include

QCS405_BLOB := $(top)/3rdparty/blobs/soc/qualcomm/qcs405

$(objcbfs)/bootblock.bin: $(objcbfs)/bootblock.raw.bin
	@printf "Generating: $(subst $(obj)/,,$(@))\n"
	cp $(objcbfs)/bootblock.raw.bin $(objcbfs)/bootblock.bin

################################################################################
ifeq ($(CONFIG_QC_SDI_ENABLE),y)
QCSDI_FILE := $(QCS405_BLOB)/QcSdi.elf
qcsdi_file := $(shell ls $(QCSDI_FILE))
ifneq (,$(findstring $(QCSDI_FILE),$(qcsdi_file)))
	QCSDI_CBFS := $(CONFIG_CBFS_PREFIX)/qcsdi
	$(QCSDI_CBFS)-file := $(QCSDI_FILE)
	$(QCSDI_CBFS)-type := stage
	$(QCSDI_CBFS)-compression := none
	cbfs-files-y += $(QCSDI_CBFS)
endif
endif

################################################################################
DCB_FILE := $(QCS405_BLOB)/cdt.bin
dcb_file := $(shell ls $(DCB_FILE))
ifneq (,$(findstring $(DCB_FILE),$(dcb_file)))
	DCB_CBFS := $(CONFIG_CBFS_PREFIX)/dcb
	$(DCB_CBFS)-file := $(DCB_FILE)
	$(DCB_CBFS)-type := raw
	$(DCB_CBFS)-compression := none
	cbfs-files-y += $(DCB_CBFS)
endif

################################################################################
QCLIB_FILE := $(QCS405_BLOB)/QcLib.elf
qclib_file := $(shell ls $(QCLIB_FILE))
ifneq (,$(findstring $(QCLIB_FILE),$(qclib_file)))
	QCLIB_CBFS := $(CONFIG_CBFS_PREFIX)/qclib
	$(QCLIB_CBFS)-file := $(QCLIB_FILE)
	$(QCLIB_CBFS)-type := stage
	$(QCLIB_CBFS)-compression := none
	cbfs-files-y += $(QCLIB_CBFS)
endif

################################################################################
PMIC_FILE := $(QCS405_BLOB)/Pmic.bin
pmic_file := $(shell ls $(PMIC_FILE))
ifneq (,$(findstring $(PMIC_FILE),$(pmic_file)))
	PMIC_CBFS := $(CONFIG_CBFS_PREFIX)/pmiccfg
	$(PMIC_CBFS)-file := $(PMIC_FILE)
	$(PMIC_CBFS)-type := raw
	$(PMIC_CBFS)-compression := none
	cbfs-files-y += $(PMIC_CBFS)
endif

################################################################################
RPM_FILE := $(QCS405_BLOB)/rpm.mbn
rpm_file := $(shell ls $(RPM_FILE))
ifneq (,$(findstring $(RPM_FILE),$(rpm_file)))
	RPM_CBFS := $(CONFIG_CBFS_PREFIX)/rpm
	$(RPM_CBFS)-file := $(RPM_FILE)
	$(RPM_CBFS)-type := payload
	$(RPM_CBFS)-compression := $(CBFS_COMPRESS_FLAG)
	cbfs-files-y += $(RPM_CBFS)
endif

################################################################################
QC_SEC_FILE := $(QCS405_BLOB)/qc_sec.mbn
qc_sec_file := $(shell ls $(QC_SEC_FILE))
ifneq (,$(findstring $(QC_SEC_FILE),$(qc_sec_file)))
$(objcbfs)/bootblock.bin: $(objcbfs)/bootblock.elf
	python util/qualcomm/createxbl.py --mbn_version 6 -f $(objcbfs)/bootblock.elf \
		-x $(QC_SEC_FILE) -o $(objcbfs)/merged_bb_qcsec.mbn \
		-a 64 -d 32 -c 32
	@printf "\nqgpt.py 4K sector size\n"
	python util/qualcomm/qgpt.py $(objcbfs)/merged_bb_qcsec.mbn \
		$(objcbfs)/bootblock.bin

else
$(objcbfs)/bootblock.bin: $(objcbfs)/bootblock.raw.bin
	@printf "Generating: $(subst $(obj)/,,$(@))\n"
	cp $(objcbfs)/bootblock.raw.bin $(objcbfs)/bootblock.bin
endif
endif
