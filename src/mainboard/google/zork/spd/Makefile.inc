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

LIB_SPD_DEPS = $(foreach f, $(SPD_SOURCES), src/mainboard/$(MAINBOARDDIR)/spd/$(f).spd.hex)

SPD_SOURCES =  hynix-HMA851S6CJR6N-VK		# 0b0000
SPD_SOURCES += hynix-HMAA1GS6CMR6N-VK		# 0b0001
SPD_SOURCES += empty				# 0b0010
SPD_SOURCES += micron-MT40A1G16KD-062E-E	# 0b0011
SPD_SOURCES += samsung-K4A8G165WC-BCWE		# 0b0100
SPD_SOURCES += empty				# 0b0101
SPD_SOURCES += empty				# 0b0110
SPD_SOURCES += empty				# 0b0111
SPD_SOURCES += empty				# 0b1000
SPD_SOURCES += empty				# 0b1001
SPD_SOURCES += empty				# 0b1010
SPD_SOURCES += empty				# 0b1011
SPD_SOURCES += empty				# 0b1100
SPD_SOURCES += empty				# 0b1101
SPD_SOURCES += empty				# 0b1110
SPD_SOURCES += empty				# 0b1111

# APCB_SOURCES should be the same as SPD_SOURCES, except _x[channels] suffix
# must be included. Name should correlate to a file in:
# picasso-pi/AGESA/AgesaPkg/Addendum/Apcb/RavenFp5Trembyle/ApcbData/
# TODO: Remove channel suffix when b:141434940 is fixed
APCB_SOURCES =  hynix-HMA851S6CJR6N-VK_x1	# 0b0000
APCB_SOURCES += hynix-HMAA1GS6CMR6N-VK_x2	# 0b0001
APCB_SOURCES += empty				# 0b0010
APCB_SOURCES += micron-MT40A1G16KD-062E-E_x2	# 0b0011
APCB_SOURCES += samsung-K4A8G165WC-BCWE_x1	# 0b0100
APCB_SOURCES += empty				# 0b0101
APCB_SOURCES += empty				# 0b0110
APCB_SOURCES += empty				# 0b0111
APCB_SOURCES += empty				# 0b1000
APCB_SOURCES += empty				# 0b1001
APCB_SOURCES += empty				# 0b1010
APCB_SOURCES += empty				# 0b1011
APCB_SOURCES += empty				# 0b1100
APCB_SOURCES += empty				# 0b1101
APCB_SOURCES += empty				# 0b1110
APCB_SOURCES += empty				# 0b1111
