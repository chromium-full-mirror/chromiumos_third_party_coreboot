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
SPD_SOURCES += empty				# 0b0011
SPD_SOURCES += empty				# 0b0100
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
