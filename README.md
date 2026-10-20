# U-Boot for Xiaomi Poco F5 / Redmi Note 12 Turbo (marble)

Mainline U-Boot port for the **Xiaomi Poco F5 / Redmi Note 12 Turbo** (`xiaomi-marble`, Qualcomm Snapdragon 7+ Gen 2 / SM7475).

The bootloader is chainloaded by the stock Android Bootloader (ABL) on slot B (`boot_b`), providing extlinux boot capability from UFS storage, a countdown boot menu, USB mass storage and fastboot modes, and an interactive USB serial console (`/dev/ttyACM0`) started from the menu.

---

## Hardware Features

* **Display**: SimpleFB display console on internal AMOLED panel (1080x2400).
* **Storage**: Qualcomm QMP UFS 3.1 controller (`scsi 0`).
* **Input**: Volume Up / Volume Down keys via PM8350 SPMI GPIO for boot menu navigation.
* **USB**: DWC3 peripheral controller with PTN3222 eUSB2 repeater and USB CDC ACM serial console, USB mass storage (UMS) and fastboot gadgets. Only one gadget can be active per boot: reset the phone before switching between the serial console and mass storage or fastboot.
* **Memory**: Universal RAM bank parsing from previous bootloader (ABL) via `CONFIG_QCOM_PREFER_PREVBL_MEMORY` (supports 8 GB, 12 GB, and 16 GB models).

---

## Building and Packaging

### Prerequisites

Install an aarch64 cross-compiler and `mkbootimg`:

```bash
# Alpine Linux (the cross compiler prefix is aarch64-none-elf-)
apk add gcc-aarch64-none-elf mkbootimg

# Debian / Ubuntu / ALT Linux
apt-get install gcc-aarch64-linux-gnu mkbootimg
```

The usual U-Boot build dependencies are needed as well: make, bison, flex, swig, Python 3 with setuptools and pyelftools, and the OpenSSL, GnuTLS and libuuid development files. The script uses the `aarch64-linux-gnu-` prefix unless `CROSS_COMPILE` is set.

### Automated Build

Run the included packaging script from the top of the source tree:

```bash
./scripts/pack-marble-bootimg.sh
# Alpine: CROSS_COMPILE=aarch64-none-elf- ./scripts/pack-marble-bootimg.sh
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

postmarketOS is installed in four steps: U-Boot is flashed to slot B, the root filesystem is built on the PC, the phone exports its internal storage as a USB disk from the U-Boot menu, and `scripts/marble-install` writes the images to it. No custom recovery is needed.

### Layouts

| Layout | Android | Internal storage |
|---|---|---|
| `single` | Not usable. The `userdata` partition is replaced. | `esp` (1 GiB) and `pmos` (the rest of the former `userdata`). |
| `dual` | Kept on slot A. `userdata` is shrunk and its data is lost once. | `esp` (1 GiB) and `pmos` (size chosen with `--pmos-size`) at the end of the disk. |

Both layouts erase the Android user data. Back it up first. Partitions other than `userdata` are not modified, and the boot chain (xbl, abl, modem and others, which sit on LUN 1 to 5) cannot be reached from the PC at all, because U-Boot exports only LUN 0.

The IMEI and radio calibration (`persist`, `modemst1`, `modemst2`, `fsg`) are on LUN 5. A backup from Android is still advised before any change to the device.

### Requirements

* Unlocked bootloader.
* Linux PC with `fastboot`, Python 3, `pmbootstrap` 3.11 or newer, e2fsprogs, util-linux and, for an encrypted root, `cryptsetup`.
* `boot-uboot.img`, built as described above or taken from the release page.

### 1. Flash U-Boot to slot B

Slot A stays untouched. In fastboot mode (hold Volume Down + Power while the phone is off):

```bash
fastboot getvar current-slot
fastboot getvar current-slot
fastboot flash boot_b boot-uboot.img
fastboot set_active b
fastboot reboot
```

The slot is queried twice because the first answer after a reboot can be wrong.

### 2. Build the images

The device packages are in a pmaports fork. pmbootstrap reads `channels.cfg` from the upstream branch, so the upstream remote has to exist:

```bash
git clone -b marble https://github.com/iLtanaka/pmaports.git
git -C pmaports remote add upstream https://gitlab.postmarketos.org/postmarketOS/pmaports.git
git -C pmaports fetch --depth=1 upstream main
pmbootstrap --aports=./pmaports init
```

Select vendor `xiaomi`, codename `marble` and a user interface (for example `plasma-mobile` or `phosh`). pmbootstrap does not build the firmware package on its own, and the install fails with `no such package` for the `firmware-xiaomi-marble-*` subpackages unless it is built first:

```bash
pmbootstrap build firmware-xiaomi-marble
pmbootstrap install --split --fde --add postmarketos-buffybox-unl0kr
```

Without full disk encryption, use `pmbootstrap install --split`. The result is two filesystem images in `~/.local/var/pmbootstrap/chroot_native/home/pmos/rootfs/`: `xiaomi-marble-boot.img` and `xiaomi-marble-root.img`. The build takes a while, because the kernel is compiled locally.

### 3. Export the storage

Reboot the phone and hold Volume Down while U-Boot starts, or press a key during the 5 second countdown. Choose "Enable USB mass storage" and connect the phone to the PC. U-Boot exports UFS LUN 0, which appears as a new USB disk.

```bash
scripts/marble-install list
scripts/marble-install show
```

`list` selects only USB disks whose partition table contains `super`, `metadata`, `devinfo` and `vbmeta_system_a`. The script also refuses disks with mounted partitions or a sector size other than 4096 bytes.

### 4. Write the images

Check the plan first. Nothing is written with `--dry-run`:

```bash
scripts/marble-install install --layout single --dry-run
scripts/marble-install install --layout single
```

For a dual boot setup, keep 64 GiB for postmarketOS (default) or choose another size. The remaining `userdata` has to stay above 32 GiB:

```bash
scripts/marble-install install --layout dual --pmos-size 64G
```

The script saves the old partition table to `marble-lun0-gpt-<time>.bak` in the current directory, creates `esp` and `pmos`, writes the images, compares them with the source, and grows the filesystems to fill the partitions. For an encrypted root, `cryptsetup` asks for the passphrase chosen during `pmbootstrap install`. If the images are not in the pmbootstrap work directory, pass `--boot-img` and `--root-img`.

`--layout reinstall` writes new images into the existing `esp` and `pmos` partitions without changing the partition table.

### 5. Boot

Press a key on the phone to leave mass storage mode and choose "Boot postmarketOS", or reset the phone and wait for the countdown. With an encrypted root, the passphrase is entered on the touchscreen.

With the `dual` layout, Android needs a data wipe before its next start, because its encrypted `userdata` was resized. Make slot A active, boot the recovery (Volume Up + Power) and wipe the data. The format function of a custom recovery was used for this; the wipe of the stock recovery has not been tested.

```bash
fastboot set_active a
```

### Recovery

* **Phone boots Android after crashes of the Linux system.** ABL counts failed boots of slot B and switches to slot A when the counter runs out. Enter fastboot mode and run `fastboot set_active b`. With the `single` layout, Android cannot start, so slot B has to be made active again the same way.
* **Return to the old partition table.** Export the storage again and run `scripts/marble-install restore marble-lun0-gpt-<time>.bak`. This writes back the saved table only. The data in `pmos` is gone, and `userdata` has to be wiped from the Android recovery.

### Manual partitioning

The installer is a convenience. U-Boot only requires a GPT partition labeled `esp` on LUN 0 with `/extlinux/extlinux.conf` on it (ext2, ext4 or FAT). The root partition is found by the initramfs from the UUID on the kernel command line, so any partition layout works as long as the UUIDs of the written filesystems are kept. The GPT of the phone uses 4096-byte sectors; many tools assume 512 and must not be used on it. Filesystems on this disk need a block size of at least 4096 bytes.

---

## Boot Workflow

1. ABL executes `boot_b` and starts U-Boot.
2. U-Boot initializes display and UFS. The USB serial console starts only when its menu entry is chosen.
3. An interactive 5-second countdown menu appears. Holding **Volume Down** interrupts the countdown.
4. By default, U-Boot scans for the partition labeled `esp` on LUN0 and executes `/extlinux/extlinux.conf`.

---

## License

GPL-2.0-or-later.
