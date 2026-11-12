#!/usr/bin/env bash
# Obnoví uloženou kopii upraveného ovladače WiFi (wifi/scsc) a z ní archiv pro
# balíček kernelu v pmaports (scsc-wlbt-troika.tar.gz), potom přepočítá
# součty balíčku (tools/update-kernel-pkg.sh).
#
# wifi/scsc = původní soubory Samsungu z kernelu tabletu gta4xl (LineageOS,
# commit 8d1f7a68b0d0) + přenos na Linux 7.3 z portu gta4xl + naše úpravy
# (cesty k firmwaru, wifi/troika.h, wifi/Makefile). Viz wifi/README.md.
# Na telefon nesahá.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
pkg="$root/pmaports/device/testing/linux-motorola-troika-mainline"

"$root/tools/prepare-wifi.sh"
rm -rf "$root/wifi/scsc"
cp -r "$root/build/scsc" "$root/wifi/scsc"
rm -f "$root/wifi/scsc/gta4xl.h"
# Bez Bluetooth: zatím se nesestavuje (wifi/Makefile).
tar --sort=name --mtime=@0 --owner=0 --group=0 --numeric-owner \
	--transform 's,^scsc,scsc-wlbt-troika,' -C "$root/wifi" -cf - scsc |
	gzip -9n > "$pkg/scsc-wlbt-troika.tar.gz"
ls -l "$pkg/scsc-wlbt-troika.tar.gz"
"$root/tools/update-kernel-pkg.sh"
