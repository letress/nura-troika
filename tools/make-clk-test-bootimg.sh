#!/usr/bin/env bash
# Sestaví testovací boot image z kernelu v build/linux (větev troika-clk-test:
# hodiny, USB, UFS a ladicí výpisy), uniLoaderu jako v balíčku a testovacího
# initramfs.
#
#   tools/make-clk-test-bootimg.sh [JMÉNO]     výchozí JMÉNO je USB1
#
# Výstup: out/troika-test-JMÉNO.img, v cmdline `pmos_test=jméno` malými písmeny
# (podle něj testovací init volí, co vypíše). Ostatní image v out/ nechává být.
# CLK1 vznikl dřívější verzí tohoto skriptu z kernelu bez USB; znovu ho
# nesestavuj, přepsal by se novějším kernelem.
# Nic neflashuje, na telefon nesahá.
set -euo pipefail

name="${1:-USB1}"
root="$(cd "$(dirname "$0")/.." && pwd)"
. "$root/tools/env.sh"

image="$root/build/linux/arch/arm64/boot/Image"
dtb="$root/build/linux/arch/arm64/boot/dts/exynos/exynos9610-troika.dtb"
stock_dtb="$root/device-info/bootimg_a/second"
busybox="$root/dl/busybox-static/bin/busybox.static"
uni="$root/uniLoader-src"
out="$root/out"

for f in "$image" "$dtb" "$stock_dtb" "$busybox" "$root/test-initramfs/init"; do
	[ -f "$f" ] || { echo "CHYBA: chybí $f" >&2; exit 1; }
done
grep -q CLKDBG "$image" || { echo "CHYBA: $image není z větve troika-clk-test" >&2; exit 1; }
grep -q samsung,exynos9610-cmu-top "$dtb" || { echo "CHYBA: $dtb nemá uzly CMU" >&2; exit 1; }

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT
chmod 755 "$tmp"
mkdir -p "$tmp/bin" "$tmp/proc" "$tmp/sys" "$tmp/dev"
install -m 755 "$busybox" "$tmp/bin/busybox"
install -m 755 "$root/test-initramfs/init" "$tmp/init"
# Restart s argumentem (test RB): busybox ho předat neumí.
aarch64-linux-gcc -nostdlib -static -o "$tmp/bin/reboot-arg" "$root/test-initramfs/reboot-arg.S"
# Čtení a zápis registru I2C (test BL): busybox nástroje i2c* nemá.
aarch64-linux-gcc -nostdlib -static -O2 -ffreestanding -fno-stack-protector \
	-o "$tmp/bin/i2c-reg" "$root/test-initramfs/i2c-reg.c"
# Čtení registrů přes /dev/mem (test TMU): busybox devmem nemá. Jen čte.
aarch64-linux-gcc -nostdlib -static -O2 -ffreestanding -fno-stack-protector \
	-o "$tmp/bin/mem-rd" "$root/test-initramfs/mem-rd.c"
# Zápis do vybraných registrů zvukové jednotky (test ABOX): jen adresy ze
# seznamu v abox-wr.c. Sám se nespouští.
aarch64-linux-gcc -nostdlib -static -O2 -ffreestanding -fno-stack-protector \
	-o "$tmp/bin/abox-wr" "$root/test-initramfs/abox-wr.c"
# Modul firmwaru ACPM pro změnu frekvencí (test ACPM3): soubor ze zdrojů
# původního kernelu, který si kernel načte přes request_firmware().
fvp="$root/refs/downstream-kernel/firmware/exynos9610_acpm_fvp.fw"
if [ -f "$fvp" ]; then
	mkdir -p "$tmp/lib/firmware"
	install -m 644 "$fvp" "$tmp/lib/firmware/"
fi
# WiFi (test WLAN): moduly ovladače SCSC z build/scsc (tools/prepare-wifi.sh
# build) a firmware rádia ze zálohy (tools/extract-wifi-fw.sh). Init je sám
# nenahrává; rádio se spouští až ručním insmod po souhlasu uživatele.
if ls "$root"/build/scsc/*.ko > /dev/null 2>&1 && [ -d "$root/dl/wifi-fw/motorola" ]; then
	mkdir -p "$tmp/lib/wifi" "$tmp/lib/firmware"
	for m in "$root"/build/scsc/*.ko; do
		aarch64-linux-strip --strip-debug -o "$tmp/lib/wifi/$(basename "$m")" "$m"
	done
	cp -r "$root/dl/wifi-fw/motorola" "$tmp/lib/firmware/"
fi
# DHCP server a telnetd z initramfs pmOS (dynamicky linkované proti musl).
# Bez nich test poběží taky, jen bez DHCP a telnetu.
bits="$root/dl/pmos-bits"
if [ -f "$bits/busybox-extras" ] && [ -f "$bits/unudhcpd" ] && [ -f "$bits/ld-musl-aarch64.so.1" ]; then
	mkdir -p "$tmp/lib" "$tmp/usr/bin"
	install -m 755 "$bits/ld-musl-aarch64.so.1" "$tmp/lib/ld-musl-aarch64.so.1"
	ln -s ld-musl-aarch64.so.1 "$tmp/lib/libc.musl-aarch64.so.1"
	install -m 755 "$bits/busybox-extras" "$bits/unudhcpd" "$tmp/usr/bin/"
fi
# Nástroje pro hledání sítí a připojení (test WLAN2), z tools/fetch-wifi-tools.sh.
# Nic se nespouští samo.
wtools="$root/dl/wifi-tools"
if [ -x "$wtools/bin/wpa_supplicant" ] && [ -f "$tmp/lib/ld-musl-aarch64.so.1" ]; then
	mkdir -p "$tmp/usr/sbin" "$tmp/usr/lib" "$tmp/run" "$tmp/tmp" "$tmp/etc"
	install -m 755 "$wtools/bin/wpa_supplicant" "$wtools/bin/wpa_cli" "$tmp/usr/sbin/"
	install -m 755 "$wtools"/lib/* "$tmp/usr/lib/"
fi
(cd "$tmp" && find . | LC_ALL=C sort | cpio --quiet -o -H newc -R 0:0) | gzip -9n > "$out/test-initramfs-$name.cpio.gz"

# uniLoader jako v balíčku (troika_defconfig): initrd a cmdline z DTB bootloaderu.
install -m 644 "$image" "$uni/blob/Image"
install -m 644 "$dtb" "$uni/blob/dtb"
head -c 4096 /dev/zero > "$uni/blob/ramdisk"
make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- troika_defconfig
make -s -C "$uni" ARCH=aarch64 CROSS_COMPILE=aarch64-linux- -j"$(nproc)"
install -m 644 "$uni/uniLoader" "$out/uniLoader-troika-clk"

# Stejné parametry jako tools/make-test-bootimg.sh (test P). clk_ignore_unused
# přidává i bootloader z původního DTB; tady je pro jistotu ještě jednou.
mkbootimg --header_version 1 \
	--base 0x10000000 --kernel_offset 0x00008000 \
	--ramdisk_offset 0x00000000 --second_offset 0x00000000 \
	--tags_offset 0x00000000 --pagesize 2048 \
	--os_version 15.0.0 --os_patch_level 2025-04 \
	--cmdline "console=tty0 fbcon=font:TER16x32 clk_ignore_unused scsi_mod.scan=sync pmos_test=${name,,}" \
	--kernel "$out/uniLoader-troika-clk" --ramdisk "$out/test-initramfs-$name.cpio.gz" \
	--second "$stock_dtb" \
	-o "$out/troika-test-$name.img"
(cd "$out" && sha256sum troika-test-*.img > SHA256SUMS)
ls -l "$out/troika-test-$name.img"
