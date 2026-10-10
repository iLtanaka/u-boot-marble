#!/bin/bash
# Script to build U-Boot and pack an Android boot image (v4) for Xiaomi Poco F5 (marble)
set -euo pipefail

OS_VERSION="${OS_VERSION:-16.0.0}"
OS_PATCH_LEVEL="${OS_PATCH_LEVEL:-2026-08}"
CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"

echo "[1/4] Configuring U-Boot for Xiaomi Poco F5 (marble)..."
scripts/kconfig/merge_config.sh \
    configs/qcom_defconfig \
    board/qualcomm/qcom-phone.config \
    board/qualcomm/xiaomi-marble.config

echo "[2/4] Compiling U-Boot..."
make CROSS_COMPILE="$CROSS_COMPILE" -j"$(nproc)"

echo "[3/4] Compressing payload and appending device tree..."
gzip -9 -c u-boot.bin > u-boot-kernel.gz
cat u-boot-kernel.gz dts/upstream/src/arm64/qcom/sm8475-xiaomi-marble.dtb > u-boot-payload.gz

echo "[4/4] Packing boot-uboot.img via mkbootimg..."
mkbootimg \
    --header_version 4 \
    --kernel u-boot-payload.gz \
    --pagesize 4096 \
    --os_version "$OS_VERSION" \
    --os_patch_level "$OS_PATCH_LEVEL" \
    -o boot-uboot.img

echo "Done: boot-uboot.img generated successfully."
