##
## This file is part of the coreboot project.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; version 2 of the License.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##

# Ordered List of APCB entries, upto 16.
# Entries should match this pattern {NAME}_x{1,2}
# There should be a matching SPD hex file in SPD_SOURCES_DIR
# matching the pattern {NAME}.spd.hex
# The _x{1,2} suffix denotes single or dual channel
# TODO: Remove channel suffix when b:141434940 is fixed
# Alternatively, generated APCBs stored at
# CONFIG_APCB_BLOB_DIR/APCB_{NAME}.bin can be included.
APCB_SOURCES =  samsung-K4AAG165WA-BCWE_x2	# 0b0000
APCB_SOURCES += empty				# 0b0001
# b/149596178: We can't use dual channel channel until the PSP supports missing
# channels.
APCB_SOURCES += micron-MT40A512M16TB-062E-J_x1	# 0b0010
APCB_SOURCES += micron-MT40A1G16KD-062E-E_x2	# 0b0011
APCB_SOURCES += samsung-K4A8G165WC-BCWE_x1	# 0b0100
APCB_SOURCES += hynix-H5AN8G6NDJR-XNC_x1	# 0b0101
APCB_SOURCES += micron-MT40A512M16TB-062E-J_x1	# 0b0110
APCB_SOURCES += samsung-K4AAG165WA-BCWE_x2	# 0b0111
APCB_SOURCES += hynix-H5ANAG6NCMR-XNC_x2	# 0b1000
APCB_SOURCES += empty				# 0b1001
APCB_SOURCES += empty				# 0b1010
APCB_SOURCES += empty				# 0b1011
APCB_SOURCES += empty				# 0b1100
APCB_SOURCES += empty				# 0b1101
APCB_SOURCES += empty				# 0b1110
