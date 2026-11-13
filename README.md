# nura-troika – mainline Linux on the Motorola One Action

A work-in-progress port of Nura (formerly postmarketOS) to the **Motorola One
Action** (`troika`, Exynos 9609 – an underclocked Exynos 9610, A/B partitions)
running a **mainline Linux 7.3.3** kernel with out-of-tree patches. The result
is started with `fastboot boot` (nothing is ever flashed) and runs the Phosh
desktop with accelerated graphics (Panfrost) and Wi-Fi.

**Status: work in progress, experimental.** The phone is usable as a test
machine over USB and Wi-Fi, not as a daily phone (see [What works](#what-works)).

The project documents (`PROGRESS.md`, `FINDINGS.md`, `TESTING.md`) are written
in Czech. Tools, patches and kernel commit messages are in English.

## Safety – read this before trying anything

- **Nothing is written to the phone.** All tests go through `fastboot boot`
  (the image is loaded into RAM; after a reboot the phone is back in its
  original state). `fastboot flash`, `fastboot erase` and `pmbootstrap flasher`
  are **not used** in this project and are not recommended.
- **The phone's storage (UFS) is read-only in the system** – the initramfs
  guards it with a rule and a start-up check, and the system runs over USB
  (netboot). Reason: the mainline UFS driver has no verified write safety yet.
- Before anything else, make a **full backup of all partitions**
  (`tools/backup-all-partitions.sh`) and verify it. Do not start without one.
- Power rails, charging and regulators are intentionally not described in the
  device tree (risk of hardware damage). The only exception is the careful,
  step-by-step audio investigation (see below).
- Everything comes without warranty and has been verified on a single unit
  with an unlocked bootloader.

## What works

Verified on the phone (details, values and sources in [`FINDINGS.md`](FINDINGS.md)):

| Area | Status |
|---|---|
| Booting a mainline kernel through the stock bootloader | works (chain [uniLoader](https://github.com/ivoszbg/uniLoader) → kernel + DTB) |
| UFS storage | reading works, writing is disabled on purpose |
| USB (device mode) | works, networking over USB and SSH |
| Touchscreen (Himax HX83112A) | works |
| Buttons, reboot, power off, reboot to bootloader | works |
| Display | bootloader framebuffer (simpledrm); no native panel driver yet |
| Backlight (SGM37603A) | works, brightness from Phosh, off with the screen |
| Phosh desktop | works, Mali-G72 GPU through Panfrost |
| GPU frequency | 260–764 MHz controlled through the ACPM firmware, thermal throttling from 88 °C |
| Temperatures (TMU) | three sensors readable, zones with a protective shutdown at 105 °C |
| Wi-Fi | works (Samsung SCSC driver, out of tree), WPA2 on 5 GHz, scanning, connecting, internet |

## What does not work or is not finished

- **Audio** – all sound goes through the SoC's audio unit (ABOX), for which
  mainline has nothing. Analysed, documented and partly brought up (address
  translation done); without the unit's firmware playback is not possible.
  See chapters 31 and 33 of [`FINDINGS.md`](FINDINGS.md).
- **Bluetooth**, **cameras**, **modem/SIM**, **SD card**, **suspend**,
  **CPU frequency scaling** (fixed bootloader frequencies for now),
  **charging and battery status**.
- **Wi-Fi:** the MAC address is passed on by the bootloader and applied by a
  udev rule (not yet verified), the regulatory database is missing; WPA3 and
  long-term operation are unverified.
- **Installation to storage** – the system runs from a PC over USB (netboot).

## Layout

The project consists of two repositories:

| Repository | Contents |
|---|---|
| **this one** (`nura-troika`) | documentation, kernel patches, tools, test initramfs, Wi-Fi driver |
| [`pmaports-troika`](https://github.com/letress/pmaports-troika) | postmarketOS/Nura packages: `device-motorola-troika`, `linux-motorola-troika-mainline`, `firmware-motorola-troika` |

Contents of this repository:

| Path | What is there |
|---|---|
| [`PROGRESS.md`](PROGRESS.md) | **start here** – current state, rules, work log |
| [`FINDINGS.md`](FINDINGS.md) | results of research and tests with sources of values (chapters by topic) |
| [`TESTING.md`](TESTING.md) | how each test is done and how to run it |
| `kernel-patches/` | clean patches on top of Linux 7.3.3 (clocks, USB, UFS, touch, GPU, backlight, ACPM, TMU, Wi-Fi nodes, …) |
| `kernel-patches-debug/` | debug patches (register dumps and experimental drivers); not part of the package |
| `patches-src/` | original patches from lore.kernel.org that the work started from |
| `wifi/` | Samsung SCSC 11.19 driver ported to Linux 7.3 plus troika setup ([`wifi/README.md`](wifi/README.md)) |
| `tools/` | environment, test-image builds, phone backup and dump, firmware analysis |
| `test-initramfs/` | test `init` and small tools (register reads, I²C, reboot to bootloader) |

## How to try it

Full details are in [`TESTING.md`](TESTING.md). Roughly:

1. A phone with an **unlocked bootloader** and a **full partition backup**.
2. Linux 7.3.3 plus the patches from `kernel-patches/`
   (`tools/update-kernel-pkg.sh` writes them into the package), toolchain as
   in `tools/env.sh`.
3. The packages from [`pmaports-troika`](https://github.com/letress/pmaports-troika)
   and `pmbootstrap` (`build`, `install`, `export`).
4. Boot the image with `fastboot boot` and serve the system with
   `pmbootstrap netboot serve`.

Quick tests without packages: `tools/make-clk-test-bootimg.sh` (a small test
image with telnet) and `tools/make-local-pmos-bootimg.sh` (the full system
with a locally built kernel).

## How the project is run

- No hardware value (address, memory size, GPIO, voltage) is made up: each one
  has a documented source, otherwise it is marked TODO.
  Sources: the original LineageOS kernel and device tree, dumps from the phone
  itself, mailing lists, the Galaxy Tab S6 Lite reference port.
- Hardware changes are tried in small steps, and each one is written into
  [`FINDINGS.md`](FINDINGS.md) together with what did not work.
- Data from the phone (`device-info/`, `photos/`) is kept out of the
  repository: it contains the serial number, MAC addresses and other
  identifiers.

## Credits and sources

- The [postmarketOS](https://postmarketos.org) team and the community around Nura.
- [LineageOS](https://lineageos.org) – the Motorola One Action kernel and device
  tree sources, without which the values would have had no source.
- The author of the Exynos 9611 port for the Galaxy Tab S6 Lite
  ([`gta4xl-linux`](https://github.com/arhammahajan/gta4xl-linux)) – guidance
  for ACPM, GPU and the Wi-Fi driver port.
- [uniLoader](https://github.com/ivoszbg/uniLoader) (Ivaylo Ivanov).
- The authors of the mainline Exynos drivers, mainly `exynos-acpm`
  (Linaro/Google), and of Exynos 9610 support on [lore.kernel.org](https://lore.kernel.org).

## License

The project's own scripts, tools, test initramfs and documentation are licensed
under the **GNU GPL version 2** (`GPL-2.0-only`, see [`LICENSE`](LICENSE)), like
Linux. Exceptions:

- Kernel patches carry the license of the files they modify (Linux:
  GPL-2.0-only; device-tree bindings are often `GPL-2.0-only OR BSD-2-Clause`).
- The driver in `wifi/scsc/` is Samsung code from the Galaxy Tab S6 Lite kernel
  (GPL-2.0, see `wifi/scsc/COPYING` and [`wifi/README.md`](wifi/README.md)) and
  keeps its original headers.
- The files in `patches-src/` are third-party patches from mailing lists and
  belong to their authors.

Vendor firmware (Wi-Fi radio, ACPM module) is **not** in the repository; the
`firmware-motorola-troika` package downloads it from public sources and marks
it as proprietary.
