#!/usr/bin/env bash
#
# This file is part of the coreboot project.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#
# Generates an SPD with the specified fields injected
#
# Usage:
#	gen_spd.sh --[hex|bin]-template TEMPLATE [OPTION..]
#
# Outputs: 
#	Template SPD with specified fields injected. The filename is
#	generated based on the fields specified. The output encodig will
#	match the encoding (hex or binary) of the template.

set -e

function readbyte () {
	printf "%d" "$(dd if="$1" bs=1 count=1 skip="$2" status=none | \
		       od -t u1 -A none)"
}

function writebyte () {
	local value=$(( $2 & 0xff ))
	local offset="$3"
	# shellcheck disable=SC2059
	printf "\x$(printf %x ${value})" | dd of="$1" conv=notrunc bs=1 \
					   count=1 seek="${offset}" status=none
}

function readbits {
	local file="$1"
	local byte_offset="$2"
	local bit_offset="$3"
	local bit_count="$4"
	local mask
	local value
	mask=$(( ((1<<bit_count)-1)<<bit_offset ))
	value=$(( ($(readbyte "${file}" "${byte_offset}") & mask)>>bit_offset ))
	printf "%d" "${value}"
}

function writebits {
	local file="$1"
	local value="$2"
	local byte_offset="$3"
	local bit_offset="$4"
	local bit_count="$5"
	local mask
	local other_bits
	mask=$(( ((1<<bit_count)-1)<<bit_offset ))
	other_bits=$(( $(readbyte "${file}" "${byte_offset}") & ~mask ))
	value=$(( (value << bit_offset) & mask ))
	writebyte "${file}" $(( value | other_bits )) "${byte_offset}"
}

# Caclulates the CRC16 of a file as defined in
# JEDEC Standard No. 21-C
# Page 4.1.2.12.3 – 44
# Byte 127 (0x07F): Cyclical Redundancy Code (CRC)
function crc16 {
	local file="$1"
	local start="$2"
	local length="$3"
	local crc=0
	for (( offset = start; offset < (start+length); offset++ )); do
		local b
		b=$(readbyte "${file}" "${offset}")
		crc=$(( (crc ^ (b << 8)) & 0xffff ))
		for (( j = 0; j < 8; j++ )); do
			if [ $((crc & 0x8000)) -ne 0 ]; then
				crc=$(((crc << 1) ^ 0x1021))
			else
				crc=$((crc << 1))
			fi
		done
	done
	printf "%d" $((crc & 0xffff))
}

function error {
	echo "ERROR: $1"
	rm -f "${outtmp}"
	exit 1
}

OUTPUT_DIR="."
while (( $# )); do
	case "$1" in
	--help)
		printf "\n"
		printf "Usage:\n"
		printf "\t%s %s\n" "gen_spd.sh --[hex|bin]-template" \
			"TEMPLATE [OPTION..]"
		printf "\n"
		printf "Options:\n"
		printf "\t%s\t\t %s\n" "--hex-template TEMPLATE" \
				"SPD template, hex encoded"
		printf "\t%s\t\t %s\n" "--bin-template TEMPLATE" \
				"SPD template, binary encoded"
		printf "\t%s\t\t %s %s\n" "--output-dir OUTPUT_DIR"\
				"Directory for generated SPD"\
				"(Default: current directory)"
		printf "\t--hex-output\t\t\t Encode output as hex (Default)\n"
		printf "\t--bin-output\t\t\t Encode output as binary\n"
		printf "\t--die-capacity CAPACITY\t\t Capacity per die in Gb\n"
		printf "\t--die-count DIE_COUNT\t\t Die count\n"
		printf "\t--banks BANKS\t\t\t Addressable Banks\n"
		printf "\t--bank-groups BANK_GROUPS\t Addressable Bank Groups\n"
		printf "\t--rows ROWS\t\t\t Addressable Rows\n"
		printf "\t--columns COLUMNS\t\t Addressable Columns\n"
		printf "\t--width WIDTH\t\t\t SDRAM width in bits\n"
		printf "\t--speed SPEED\t\t\t DRAM speed in MT/s\n"
		printf "\n"
		exit
		;;
	--die-capacity)
		DIE_CAPACITY="$2"
		shift 2
		;;
	--die-count)
		DIE_COUNT="$2"
		shift 2
		;;
	--banks)
		BANKS="$2"
		shift 2
		;;
	--bank-groups)
		BANK_GROUPS="$2"
		shift 2
		;;
	--rows)
		ROWS="$2"
		shift 2
		;;
	--columns)
		COLUMNS="$2"
		shift 2
		;;
	--width)
		WIDTH="$2"
		shift 2
		;;
	--speed)
		SPEED="$2"
		shift 2
		;;
	--bin-template)
		BIN_TEMPLATE="$2"
		shift 2
		;;
	--hex-template)
		HEX_TEMPLATE="$2"
		shift 2
		;;
	--output-dir)
		OUTPUT_DIR="$2"
		shift 2
		;;
	--hex-output)
		HEX_OUTPUT=true
		shift 1
		;;
	--bin-output)
		BIN_OUTPUT=true
		shift 1
		;;
	*)
		error "Unkonwn option $1"
		;;
	esac
done

echo "Building SPD"

outtmp=$(mktemp)
file_name="${OUTPUT_DIR}/DDR4"

if [[ -n "${BIN_TEMPLATE}" ]]; then
	if [[ -n "${HEX_TEMPLATE}" ]]; then
		error "Specify --bin-template or --hex-template, not both"
	fi
	cp "${BIN_TEMPLATE}" "${outtmp}"
elif [[ -n "${HEX_TEMPLATE}" ]]; then
	grep -v '^#' "${HEX_TEMPLATE}" | xxd -r -p - "${outtmp}"
else
	error "Either --bin-template or --hex-template must be specified."
fi

if [[ -n "${SPEED}" ]]; then
	case "${SPEED}" in
		1600) val=10;;
		1866) val=9;;
		2133) val=8;;
		2400) val=7;;
		2666) val=6;;
		3200) val=5;;
		*) error "Invalid speed ${SPEED}"
	esac
	writebyte "${outtmp}" "${val}" 18
	file_name="${file_name}-${SPEED}"
fi

if [[ -n "${DIE_CAPACITY}" ]]; then
	case "${DIE_CAPACITY}" in
		1) val=2;;
		2) val=3;;
		4) val=4;;
		8) val=5;;
		16)val=6;;
		32)val=7;;
		*) error "Invalid die capacity ${DIE_CAPACITY}";;
	esac
	writebits "${outtmp}" "${val}" 4 0 3
	file_name="${file_name}-${DIE_CAPACITY}Gb"
fi

if [[ -n "${DIE_COUNT}" ]]; then
	if [ -z "${DIE_CAPACITY}" ]; then
		error "--die-count requires --die-capacity to be specified"
	fi
	writebits "${outtmp}" $((DIE_COUNT-1)) 6 4 3
	file_name="${file_name}x${DIE_COUNT}"
fi

if [[ -n "${BANKS}" ]]; then
	if [[ -z "${BANK_GROUPS}" ]]; then
		error "--bank-count requires --bank-groups to be specified"
	fi
	case "${BANKS}" in
		4) val=0;;
		8) val=1;;
		*) error "Invalid bank count ${BANKS}"
	esac
	writebits "${outtmp}" "${val}" 4 2
	file_name="${file_name}-${BANKS}Bk"
fi

if [[ -n "${BANK_GROUPS}" ]]; then
	if [[ -z "${BANKS}" ]]; then
		error "--bank-count requires --bank-groups to be specified"
	fi
	case "${BANK_GROUPS}" in
		0) val=0;;
		2) val=1;;
		4) val=2;;
		*) error "Invalid bank groups ${BANK_GROUPS}"
	esac
	writebits "${outtmp}" "${val}" 4 6 2
	file_name="${file_name}-${BANK_GROUPS}BkG"
fi

if [[ -n "${ROWS}" ]]; then
	if [[ -z "${COLUMNS}" ]]; then
		error "--rows requires --columns to be specified"
	fi
	case "${ROWS}" in
		12) val=0;;
		13) val=1;;
		14) val=2;;
		15) val=3;;
		16) val=4;;
		17) val=5;;
		18) val=6;;
		*) error "Invalid rows ${ROWS}"
	esac
	writebits "${outtmp}" "${val}" 5 3 3
	file_name="${file_name}-${ROWS}"
fi

if [[ -n "${COLUMNS}" ]]; then
	if [[ -z "${ROWS}" ]]; then
		error "--columns requires --rows to be specified"
	fi
	case "${COLUMNS}" in
		9)  val=0;;
		10) val=1;;
		11) val=2;;
		12) val=3;;
		*) error "Invalid columns ${COLUMNS}"
	esac
	writebits "${outtmp}" "${val}" 5 0 2
	file_name="${file_name}x${COLUMNS}"
fi

if [[ -n "${WIDTH}" ]]; then
	case "${WIDTH}" in
		4)  val=0;;
		8)  val=1;;
		16) val=2;;
		32) val=3;;
		*) error "Invalid sdram device width ${WIDTH}"
	esac
	writebits "${outtmp}" "${val}" 12 0 3
	file_name="${file_name}-${WIDTH}bit"
fi

# Update CRC 0~126
crc126=$(crc16 "${outtmp}" 0 126)
writebyte "${outtmp}" $(( crc126 )) 126
writebyte "${outtmp}" $(( crc126 >> 8 )) 127

# Update CRC 128~254
crc254=$(crc16 "${outtmp}" 128 254)
writebyte "${outtmp}" $(( crc254 )) 254
writebyte "${outtmp}" $(( crc254 >> 8 )) 255

if [[ "${HEX_OUTPUT}" && "${BIN_OUTPUT}" ]]; then
	error "Either --hex-output or --bin-output may be specified, not both."
elif [[ "${BIN_OUTPUT}" ]]; then
	file_name="${file_name}.spd.bin"
	echo "Writing binary SPD to ${file_name}"
	cp "${outtmp}" "${file_name}"
else
	file_name="${file_name}.spd.hex"
	echo "Writing hex SPD to ${file_name}"
	hexdump -v -e '8/1 "%02X " "  " 8/1 "%02X " "\n"' "${outtmp}" \
		> "${file_name}"
fi

rm -f "${outtmp}"

echo "Success"
