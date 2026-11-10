#!/usr/bin/env bash
# Sestaví testovací boot image pro `fastboot boot` (nic neflashuje, na telefon nesahá).
#
#   out/troika-test-A.img  uniLoader (kernel + mainline DTB + initramfs uvnitř),
#                          ve "second" je PŮVODNÍ DTB z ~/troika-backup -> overlay bootloaderu projde
#   out/troika-test-B.img  uniLoader, ve "second" mainline DTB, ramdisk boot image původní
#   out/troika-test-C.img  bez uniLoaderu: holý Image, mainline DTB ve "second"
#   out/troika-test-E{1,2,3}.img  bisekce odmítnutí image bootloaderem (viz níže)
#   out/troika-test-D.img  diagnostika displeje (uniLoader-troika-diag, kernel nespouští)
#
# Předpoklady: sestavený kernel v build/linux (Image + exynos9610-troika.dtb),
# device-info/bootimg_a/second (rozbalený DTB ze zálohy).
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
. "$root/tools/env.sh"

image="$root/build/linux/arch/arm64/boot/Image"
dtb="$root/build/linux/arch/arm64/boot/dts/exynos/exynos9610-troika.dtb"
stock_dtb="$root/device-info/bootimg_a/second"
stock_kernel="$root/device-info/bootimg_a/kernel"
stock_ramdisk="$root/device-info/bootimg_a/ramdisk"
busybox="$root/dl/busybox-static/bin/busybox.static"
uni="$root/uniLoader-src"
out="$root/out"

for f in "$image" "$dtb" "$stock_dtb" "$stock_kernel" "$stock_ramdisk" "$busybox" "$root/test-initramfs/init"; do
	[ -f "$f" ] || { echo "CHYBA: chybí $f" >&2; exit 1; }
done
mkdir -p "$out"

echo "== initramfs"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
chmod 755 "$tmp"
mkdir -p "$tmp/bin" "$tmp/proc" "$tmp/sys" "$tmp/dev"
install -m 755 "$busybox" "$tmp/bin/busybox"
install -m 755 "$root/test-initramfs/init" "$tmp/init"
(cd "$tmp" && find . | LC_ALL=C sort | cpio --quiet -o -H newc -R 0:0) | gzip -9n > "$out/test-initramfs.cpio.gz"

echo "== uniLoader"
install -m 644 "$image" "$uni/blob/Image"
install -m 644 "$dtb" "$uni/blob/dtb"
install -m 644 "$out/test-initramfs.cpio.gz" "$uni/blob/ramdisk"
# Nejdřív diagnostická varianta, potom běžná (strom zůstane v běžné konfiguraci).
for variant in troika_diag troika_diagboot troika_step troika_steppad troika_embedded troika_stepinherit troika; do
	if [ "$variant" = troika ] || [ "$variant" = troika_stepinherit ]; then
		# Jako v balíčku: ramdisk se nevkládá, soubor je jen zástupný. Kdyby
		# převzetí initrd z DTB bootloaderu nefungovalo, kernel initramfs nenajde.
		head -c 4096 /dev/zero > "$uni/blob/ramdisk"
	else
		install -m 644 "$out/test-initramfs.cpio.gz" "$uni/blob/ramdisk"
	fi
	make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- "${variant}_defconfig"
	make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- -j"$(nproc)"
	install -m 644 "$uni/uniLoader" "$out/uniLoader-${variant/_/-}"
done

# Parametry podle `pmbootstrap bootimg_analyze` na ~/troika-backup/boot_a.img
# (FINDINGS.md kap. 4); os_version/patch level opsané ze stejného image.
mk() { # <výstup> <kernel> <second> [ramdisk] [cmdline]
	local rd="${4:-$out/test-initramfs.cpio.gz}"
	local cmdline="${5:-loop.max_part=7 androidboot.boot_devices=13520000.ufs}"
	mkbootimg --header_version 1 \
		--base 0x10000000 --kernel_offset 0x00008000 \
		--ramdisk_offset 0x00000000 --second_offset 0x00000000 \
		--tags_offset 0x00000000 --pagesize 2048 \
		--os_version 15.0.0 --os_patch_level 2025-04 \
		--cmdline "$cmdline" \
		--kernel "$2" --ramdisk "$rd" --second "$3" \
		-o "$1"
}

echo "== boot image"
mk "$out/troika-test-A.img" "$out/uniLoader-troika-embedded" "$stock_dtb"
mk "$out/troika-test-B.img" "$out/uniLoader-troika-embedded" "$dtb" "$stock_ramdisk"
mk "$out/troika-test-C.img" "$image" "$dtb"

# Bisekce: co z image A bootloader odmítá. Všechny mají původní DTB ve "second".
#   E1 = původní kernel + původní ramdisk (jen přebalené, bez AVB patičky)
#   E2 = původní kernel + testovací initramfs
#   E3 = uniLoader      + původní ramdisk
#   (A = uniLoader      + testovací initramfs)
mk "$out/troika-test-E1.img" "$stock_kernel" "$stock_dtb" "$stock_ramdisk"
mk "$out/troika-test-E2.img" "$stock_kernel" "$stock_dtb"
mk "$out/troika-test-E3.img" "$out/uniLoader-troika-embedded" "$stock_dtb" "$stock_ramdisk"

# D = diagnostika displeje (nebootuje kernel), původní ramdisk i DTB.
mk "$out/troika-test-D.img" "$out/uniLoader-troika-diag" "$stock_dtb" "$stock_ramdisk"

# F = jako E3, ale uniLoader před spuštěním kernelu 20 s čeká s odpočtem.
mk "$out/troika-test-F.img" "$out/uniLoader-troika-step" "$stock_dtb" "$stock_ramdisk"

# H = F se stejným rozložením paměti jako diagnostika (jen výplň v BSS).
# G = diagnostika, která po odpočtu spustí kernel.
mk "$out/troika-test-H.img" "$out/uniLoader-troika-steppad" "$stock_dtb" "$stock_ramdisk"
mk "$out/troika-test-G.img" "$out/uniLoader-troika-diagboot" "$stock_dtb" "$stock_ramdisk"

# K = aktuální hlavní test: běžný uniLoader (framebuffer 0xed000000) + kernel
#     s opraveným DTB; v boot image původní ramdisk a DTB kvůli bootloaderu.
mk "$out/troika-test-K.img" "$out/uniLoader-troika-embedded" "$stock_dtb" "$stock_ramdisk"

# P = jak to má dělat balíček: uniLoader bez vloženého ramdisku, který si
#     polohu initrd a příkazovou řádku převezme z DTB bootloaderu. Ramdisk je
#     v boot image (tady testovací initramfs), cmdline taky.
mk "$out/troika-test-P.img" "$out/uniLoader-troika" "$stock_dtb" \
	"$out/test-initramfs.cpio.gz" "console=tty0 fbcon=font:TER16x32 pmos_test=1"

# P2 = P s krokovací variantou: před spuštěním kernelu 20 s ukazuje, co
#      bootloader předal v x0 (ukazatel na jeho DTB).
mk "$out/troika-test-P2.img" "$out/uniLoader-troika-stepinherit" "$stock_dtb" \
	"$out/test-initramfs.cpio.gz" "console=tty0 fbcon=font:TER16x32 pmos_test=1"

(cd "$out" && sha256sum troika-test-*.img > SHA256SUMS)
ls -l "$out"
