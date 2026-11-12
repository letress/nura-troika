#!/usr/bin/env bash
# Sestaví boot image, který spustí systém pmOS (initramfs, cmdline a původní
# DTB z už zkontrolovaného image z `pmbootstrap export`) s místně sestaveným
# kernelem z build/linux. Slouží ke zkoušení změn kernelu v plném systému
# (netboot) bez sestavování balíčku.
#
#   tools/make-local-pmos-bootimg.sh JMÉNO ZDROJ
#
#   JMÉNO  výstup bude out/troika-test-JMÉNO.img
#   ZDROJ  boot image z exportu, např. out/troika-test-GUI2.img
#
# Moduly v systému jsou z balíčku kernelu, proto musí mít místní kernel stejné
# označení verze (7.3.3-troika, bez přípony z gitu) a konfiguraci balíčku:
#   cp pmaports/device/testing/linux-motorola-troika-mainline/config-*.aarch64 build/linux/.config
#   scripts/config --file ../build/linux/.config -d LOCALVERSION_AUTO
#   make O=../build/linux olddefconfig && make O=../build/linux LOCALVERSION= Image dtbs
# Nic neflashuje, na telefon nesahá.
set -euo pipefail

name="${1:?chybí JMÉNO}"
src="${2:?chybí ZDROJ}"
root="$(cd "$(dirname "$0")/.." && pwd)"
. "$root/tools/env.sh"

image="$root/build/linux/arch/arm64/boot/Image"
dtb="$root/build/linux/arch/arm64/boot/dts/exynos/exynos9610-troika.dtb"
release="$root/build/linux/include/config/kernel.release"
uni="$root/uniLoader-src"
out="$root/out"

for f in "$image" "$dtb" "$release" "$src"; do
	[ -f "$f" ] || { echo "CHYBA: chybí $f" >&2; exit 1; }
done
[ "$(cat "$release")" = "7.3.3-troika" ] || {
	echo "CHYBA: kernel se jmenuje $(cat "$release"), moduly z balíčku čekají 7.3.3-troika" >&2
	exit 1
}

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
unpack_bootimg --boot_img "$src" --out "$tmp" > "$tmp/info"
cmdline="$(sed -n 's/^command line args: //p' "$tmp/info")"
[ -s "$tmp/ramdisk" ] && [ -s "$tmp/second" ] && [ -n "$cmdline" ] || {
	echo "CHYBA: $src nemá ramdisk, second nebo cmdline" >&2
	exit 1
}

# Modul firmwaru ACPM pro změnu frekvencí: ovladač exynos-acpm je zabudovaný
# a soubor chce dřív, než je připojený kořenový systém, takže musí být
# v initramfs. Initramfs z balíčku ho nemá; připojí se za něj jako druhý
# archiv cpio (kernel jich umí rozbalit víc za sebou).
fvp="$root/refs/downstream-kernel/firmware/exynos9610_acpm_fvp.fw"
if [ -f "$fvp" ]; then
	mkdir -p "$tmp/fw/usr/lib/firmware"
	install -m 644 "$fvp" "$tmp/fw/usr/lib/firmware/"
	(cd "$tmp/fw" && echo usr/lib/firmware/exynos9610_acpm_fvp.fw |
		cpio -o -H newc -R 0:0 --quiet) > "$tmp/fw.cpio"
	# Kernel pozná další archiv cpio jen na adrese dělitelné čtyřmi
	# (init/initramfs.c), nuly mezi archivy přeskakuje.
	pad=$(( (4 - $(stat -c %s "$tmp/ramdisk") % 4) % 4 ))
	head -c "$pad" /dev/zero >> "$tmp/ramdisk"
	cat "$tmp/fw.cpio" >> "$tmp/ramdisk"
fi

# uniLoader jako v balíčku (troika_defconfig): initrd a cmdline z DTB bootloaderu.
install -m 644 "$image" "$uni/blob/Image"
install -m 644 "$dtb" "$uni/blob/dtb"
head -c 4096 /dev/zero > "$uni/blob/ramdisk"
make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- troika_defconfig
make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- -j"$(nproc)"
install -m 644 "$uni/uniLoader" "$out/uniLoader-troika-local"

# Stejná hlavička, jakou dělá boot-deploy pro balíček (os_version 0).
mkbootimg --header_version 1 \
	--base 0x10000000 --kernel_offset 0x00008000 \
	--ramdisk_offset 0x00000000 --second_offset 0x00000000 \
	--tags_offset 0x00000000 --pagesize 2048 \
	--cmdline "$cmdline" \
	--kernel "$out/uniLoader-troika-local" --ramdisk "$tmp/ramdisk" \
	--second "$tmp/second" \
	-o "$out/troika-test-$name.img"
(cd "$out" && sha256sum troika-test-*.img > SHA256SUMS)
ls -l "$out/troika-test-$name.img"
