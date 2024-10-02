# Copyright 2024 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _ap_deps_impl(module_ctx):
    def _coreboot_sdk_subtool(arch, version, sha256):
        http_archive(
            name = "ap-coreboot-sdk-%s" % arch,
            build_file = "//platform/rules_cros_firmware/cros_firmware:BUILD.gcs_subtool",
            sha256 = sha256,
            url = "https://storage.googleapis.com/chromiumos-sdk/toolchains/coreboot-sdk-%s/%s.tar.zst" % (arch, version),
        )

    _coreboot_sdk_subtool(
        "aarch64-elf",
        "11.3.0-r2/7a7d0263efa0f77b3335dbd8398970906e3007cb",
        "292ec6e37223a5d2258d5cd5a2383c13cd85eef4b167ae98d308bd383fd6a1cb",
    )
    _coreboot_sdk_subtool(
        "i386-elf",
        "11.3.0-r2/5ba88fb0227c76584851bd9cbb24d785e31a717b",
        "72f0b55516120e0919f10ddf28c53a429ccc8132685b6dbd6a8dcefeba92fcc5",
    )
    _coreboot_sdk_subtool(
        "arm-eabi",
        "11.3.0-r2/8adade1392d87565482ea57bfafaf74223cebbe5",
        "312557355983bf732b20dcf7b7553a5b2a13247fc3ad6f21226ff260db1783cd",
    )
    _coreboot_sdk_subtool(
        "x86_64-elf",
        "11.3.0-r2/df64df9a5312fcc86809b63bf4f64ca3175af334",
        "006350d5d7bff416c95f4d2978b55a5dbe34a54d2e60fdde924ca87a20f3b97f",
    )
    _coreboot_sdk_subtool(
        "iasl",
        "11.3.0-r2/a59cd53c87d0d4e57386c070e9f9a2f1d1cf9b6f",
        "ad1f61b82f13c229e37ac19f43aba835363fed98ff69ff431abf3d11b311bb53",
    )

    return module_ctx.extension_metadata(
        root_module_direct_deps = [
            "ap-coreboot-sdk-aarch64-elf",
            "ap-coreboot-sdk-arm-eabi",
            "ap-coreboot-sdk-i386-elf",
            "ap-coreboot-sdk-x86_64-elf",
            "ap-coreboot-sdk-iasl",
        ],
        root_module_direct_dev_deps = [],
        reproducible = True,
    )

ap_deps = module_extension(
    implementation = _ap_deps_impl,
)
