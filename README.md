# U-Boot for Xiaomi Poco F5 / Redmi Note 12 Turbo (marble)

Mainline U-Boot port for the **Xiaomi Poco F5 / Redmi Note 12 Turbo** (`xiaomi-marble`, Qualcomm Snapdragon 7+ Gen 2 / SM7475).

The bootloader is chainloaded by the stock Android Bootloader (ABL) on slot B (`boot_b`), providing extlinux boot capability from UFS storage, a countdown boot menu, and an interactive USB serial console (`/dev/ttyACM0`).

---

## Hardware Features

* **Display**: SimpleFB display console on internal AMOLED panel (1080x2400).
* **Storage**: Qualcomm QMP UFS 3.1 controller (`scsi 0`).
* **Input**: Volume Up / Volume Down keys via PM8350 SPMI GPIO for boot menu navigation.
* **USB**: DWC3 peripheral controller with PTN3222 eUSB2 repeater and USB CDC ACM serial console.
* **Memory**: Universal RAM bank parsing from previous bootloader (ABL) via `CONFIG_QCOM_PREFER_PREVBL_MEMORY` (supports 8 GB, 12 GB, and 16 GB models).

---

## Building and Packaging

### Prerequisites

Install an aarch64 cross-compiler and `mkbootimg`:

```bash
# Alpine Linux
apk add gcc-aarch64-none-elf mkbootimg

# Debian / Ubuntu / ALT Linux
apt-get install gcc-aarch64-linux-gnu mkbootimg
```

### Automated Build

Run the included packaging script:

```bash
./scripts/pack-marble-bootimg.sh
```

### Manual Build Steps

```bash
# 1. Configure
scripts/kconfig/merge_config.sh \
    configs/qcom_defconfig \
    board/qualcomm/qcom-phone.config \
    board/qualcomm/xiaomi-marble.config

# 2. Compile
make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 3. Compress payload and append device tree
# Xiaomi ABL requires the DTB to follow the gzipped kernel stream
gzip -9 -c u-boot.bin > u-boot-kernel.gz
cat u-boot-kernel.gz dts/upstream/src/arm64/qcom/sm8475-xiaomi-marble.dtb > u-boot-payload.gz

# 4. Create Android boot image (header v4)
mkbootimg \
    --header_version 4 \
    --kernel u-boot-payload.gz \
    --pagesize 4096 \
    --os_version 16.0.0 \
    --os_patch_level 2026-08 \
    -o boot-uboot.img
```

---

## Installation

Flash the image to slot B to preserve the stock Android system on slot A:

```bash
fastboot flash boot_b boot-uboot.img
fastboot set_active b
fastboot reboot
```

---

## Boot Workflow

1. ABL executes `boot_b` and starts U-Boot.
2. U-Boot initializes display, UFS, and USB serial console.
3. An interactive 5-second countdown menu appears. Holding **Volume Down** interrupts the countdown.
4. By default, U-Boot scans for the partition labeled `esp` on LUN0 and executes `/extlinux/extlinux.conf`.

---

## License

GPL-2.0-or-later.
