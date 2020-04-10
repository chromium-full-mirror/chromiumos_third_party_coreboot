#!/bin/bash

set -eu

declare FIRMWARE_ROOT=/build/zork/firmware
declare KEYDIR=/usr/share/vboot/devkeys
: "${CACHE_DIR:=/tmp/coreboot}"
: "${VBOOT_SOURCE:=/mnt/host/source//src/platform/vboot_reference}"
: "${HOSTCC:=x86_64-pc-linux-gnu-clang}"
: "${HOSTPKGCONFIG:=x86_64-pc-linux-gnu-pkg-config}"

if [[ "$#" -eq 0 ]]; then
	echo "Usage: $0 <board|config file>..."
	echo "e.g., $0 trembyle mandolin-hardcore"
	exit 1
fi

declare -a CONFIGS
declare -a CONFIG_PATHS=(
	"$HOME/trunk/src/overlays/overlay-zork/sys-boot/coreboot-zork/files/configs"
	"$HOME/trunk/src/overlays/overlay-zork/sys-boot/coreboot/files/configs"
	"./configs"
)

em100=
psp_verstage=
while [[ "$#" -gt 0 ]]; do
	arg="$1"
	shift

	if [[ "$arg" = "--" ]]; then
		break;
	fi
	if [[ "$arg" = "-e" ]]; then
		em100=y
		echo "Using em100 mode"
		continue
	fi
	if [[ "$arg" = "--psp-verstage" ]]; then
		psp_verstage=y
		echo "Building PSP verstage"
		continue
	fi
	if [[ -e "$arg" ]]; then
		CONFIGS+=("$arg")
		continue
	fi
	found=0
	for config_path in "${CONFIG_PATHS[@]}"; do
		if [[ -e "$config_path/config.$arg" ]]; then
			CONFIGS+=("$config_path/config.$arg")
			found=1
			break
		fi
	done
	if [[ "$found" -eq 0 ]]; then
		echo "Failed to find config for '$arg'"
		exit 1
	fi
done

declare -a MAKE_OPTS=("$@")

declare -a CLEANUP

function finish {
	local dir
	for dir in "${CLEANUP[@]+"${CLEANUP[@]}"}"; do
		if [[ -e "$dir" ]]; then
			rm -r "$dir"
		fi
	done
}

trap finish EXIT

#
# Replace local paths with paths to $FIRMWARE_ROOT/coreboot-private
#
# $1: full config
#
function replace-paths() {
	local full_config="$1"
	local private="$FIRMWARE_ROOT/coreboot-private"
	local full_path
	local -a replace
	while read -r key val; do
		full_path="$private/$val"
		if [[ -e $full_path ]]; then
			replace+=(-e "s|^${key}=.*|${key}=\"${full_path}\"|")
		elif [[ "$key" = "CONFIG_PSP_BOOTLOADER_NAME" ]]; then
			# We need this because the binary assumes it's relative
			# to all the other AMD blobs

			local relative="3rdparty/blobs/soc/amd/picasso/PSP/$val"
			if [[ -e "$private/$relative" ]]; then
				full_path="$(realpath --relative-to="$relative" "$private/$relative")"
				replace+=(-e "s|^${key}=.*|${key}=\"${full_path}\"|")
			fi
		fi
	done <<<"$(sed -En \
		-e 's/^([A-Z0-9_]+)="(3rdparty\/blobs\/.*)"$/\1 \2/p' \
		-e 's/^(CONFIG_PSP_BOOTLOADER_NAME)="(.*)"$/\1 \2/p' \
		"$full_config")"
	if [[ "${replace[@]+"${#replace[@]}"}" -gt 0 ]]; then
		sed -iE "${replace[@]}" "$full_config"
	fi
}

# $1: cache_dir
# $2: work_dir
# $3: config_path
function build-coreboot-rom() {
	local cache_dir="$1" work_dir="$2" config_path="$3"

	cp "$config_path" "$work_dir/.config"
	{
		declare serial_config="$HOME/trunk/src/overlays/overlay-zork/sys-boot/coreboot-zork/files/configs/fwserial.zork"

		cat "${serial_config}"

		echo CONFIG_APCB_BLOB_DIR=\""$FIRMWARE_ROOT/coreboot-private/3rdparty/blobs/mainboard"\"
		if [[ -n "${em100}" ]]; then
			echo CONFIG_EM100=y
		fi
		if [[ -n "${psp_verstage}" ]]; then
			echo CONFIG_VBOOT_STARTS_BEFORE_BOOTBLOCK=y
			echo CONFIG_PSP_BOOTLOADER_NAME=\"test_PspBootLoader_prod_RV_dbg.sbin\"
		fi
	} >> "$work_dir/.config"
	make \
		obj="$work_dir" \
		DOTCONFIG="$work_dir/.config" \
		HOSTCC="$HOSTCC"\
		HOSTPKGCONFIG="$HOSTPKGCONFIG" \
		VBOOT_SOURCE="$VBOOT_SOURCE" \
		olddefconfig

	replace-paths "$work_dir/.config"

	if [[ ! -e "$cache_dir/.config" ]] || ! cmp "$work_dir/.config" "$cache_dir/.config"; then
		echo "Cache busted: $cache_dir"
		# Bust the cache anytime the .config is updated
		rm -rf "$cache_dir"
		mkdir -p "$cache_dir"

		cp "$work_dir/.config" "$cache_dir/.config"
	else
		echo "Reusing cache at $cache_dir"
	fi

	make -j \
		obj="$cache_dir" \
		DOTCONFIG="$cache_dir/.config" \
		HOSTCC="$HOSTCC"\
		HOSTPKGCONFIG="$HOSTPKGCONFIG" \
		VBOOT_SOURCE="$VBOOT_SOURCE" \
		"${MAKE_OPTS[@]+${MAKE_OPTS[@]}}"

	# Kconfig generates a dummy file that doesn't get removed.
	rm -f ..config.tmp.*

	echo "Successfully built $cache_dir"
}

# Use make compress assets in parallel
# $1: src
# $2: dst
# $3: image.bin
function compress-assets() {
	local src="$1" dest="$2"

	make -j -f - src="$src" dest="$dest" << 'EOF'
.DELETE_ON_ERROR:
inputs:=$(wildcard $(src)/*)
outputs:=$(inputs:$(src)/%=$(dest)/%)

.PHONY: all

all: $(outputs)

$(dest):
	mkdir -p "$@"

$(dest)/%: $(src)/% | $(dest)
	cbfs-compression-tool compress "$<" "$@" LZMA
EOF
}

# $1: work_dir
# $2: image.bin
# $3: slot
function sign-region() {
	local work_dir="$1" image="$2" slot="$3"
	local main="FW_MAIN_${slot}" vblock="VBLOCK_${slot}"
	local -i size

	size="$(cbfstool "${image}" truncate -r "$main")"
	cbfstool "${image}" read -r "$main" -f "$work_dir/$main"
	truncate -s "$size" "$work_dir/$main"
	cbfstool "${image}" write --force -u -i 255 \
		-r "$main" -f "$work_dir/$main"

	futility vbutil_firmware \
		--keyblock "${KEYDIR}/firmware.keyblock" \
		--signprivate "${KEYDIR}/firmware_data_key.vbprivk" \
		--kernelkey "${KEYDIR}/kernel_subkey.vbpubk" \
		--fv "$work_dir/$main" \
		--vblock "$work_dir/$vblock"

	cbfstool "${image}" write -u -i 255 -r "${vblock}" \
		-f "$work_dir/$vblock"
}

# $1: cache_dir
# $2: work_dir
# $3: image.bin
function add-depthcharge() {
	local cache_dir="$1" work_dir="$2" image="$3"
	local payload="$FIRMWARE_ROOT/zork/depthcharge/depthcharge.elf"

	compress-assets "$FIRMWARE_ROOT/cbfs-ro-compress" \
		"$cache_dir/compressed-assets-ro"

	local file
	for file in "$cache_dir/compressed-assets-ro"/*; do
		cbfstool "${image}" add -r COREBOOT -f "${file}" \
			-n "$(basename "${file}")" -t raw -c precompression
	done

	cbfstool "${image}" expand -r FW_MAIN_A,FW_MAIN_B

	cbfstool "${image}" add-payload -r COREBOOT,FW_MAIN_A,FW_MAIN_B \
			-f "${payload}" -n fallback/payload -c lzma

	sign-region "$work_dir" "$image" "A"
	sign-region "$work_dir" "$image" "B"
}

# $1: config_path
function build-boot-image() {
	local config_path="$1"
	local work_dir build_name cache_dir image_name
	work_dir="$(mktemp -d)" && CLEANUP+=("$work_dir")
	build_name="${config_path##*.}"
	cache_dir="$CACHE_DIR/$build_name"
	image_name="image-$build_name.serial.bin"

	echo "Building $config_path"

	build-coreboot-rom "$cache_dir"	"$work_dir" "$config_path"

	if grep -q CONFIG_VBOOT=y "$cache_dir/.config"; then
		cp "$cache_dir/coreboot.rom" "$work_dir/$image_name"
		add-depthcharge "$cache_dir" "$work_dir" "$work_dir/$image_name"
		cp "$work_dir/$image_name" "$cache_dir/$image_name"
	else
		cp "$cache_dir/coreboot.rom" "$cache_dir/$image_name"
	fi
	FINAL_IMAGES+=("$cache_dir/$image_name")
}

# Ugh, still in the src tree
rm -f .xcompile
util/xcompile/xcompile /opt/coreboot-sdk/bin/ > .xcompile

declare -a FINAL_IMAGES

for config in "${CONFIGS[@]}"; do
	build-boot-image "$config"
done

echo

for image in "${FINAL_IMAGES[@]}"; do
	echo "*** Built $image ***"
done
