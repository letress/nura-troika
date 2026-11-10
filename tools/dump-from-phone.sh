#!/usr/bin/env bash
# Vyčte z troiky (LineageOS, adb root) oddíly boot/dtbo/vbmeta obou slotů
# a informace o hardwaru potřebné pro mainline DTS.
#
# Na telefonu POUZE ČTE. Nic na něj nezapisuje, nepoužívá fastboot.
# Existující soubory v záloze nepřepisuje.
#
#   Záloha oddílů  -> $BACKUP_DIR (výchozí ~/troika-backup)
#   Info o HW      -> $INFO_DIR   (výchozí <projekt>/device-info)
set -euo pipefail

BACKUP_DIR="${BACKUP_DIR:-$HOME/troika-backup}"
INFO_DIR="${INFO_DIR:-$(cd "$(dirname "$0")/.." && pwd)/device-info}"
PARTS="boot dtbo vbmeta"
SLOTS="a b"

die() { echo "CHYBA: $*" >&2; exit 1; }

command -v adb >/dev/null || die "adb není nainstalované"
[ "$(adb get-state 2>/dev/null)" = "device" ] ||
	die "telefon není připojený nebo není povolené USB ladění"

dev=$(adb shell getprop ro.product.device | tr -d '\r')
[ "$dev" = "troika" ] || die "připojené zařízení je '$dev', ne troika"

adb root >/dev/null
adb wait-for-device
[ "$(adb shell id -u | tr -d '\r')" = "0" ] ||
	die "adb neběží jako root – zapni v Možnostech pro vývojáře 'Rooted debugging'"

byname=""
for d in /dev/block/by-name /dev/block/bootdevice/by-name; do
	if adb shell "[ -e $d/boot_a ]"; then
		byname=$d
		break
	fi
done
[ -n "$byname" ] || die "nenašel jsem oddíl boot_a"

mkdir -p "$BACKUP_DIR" "$INFO_DIR"

echo "== Oddíly ($byname) -> $BACKUP_DIR"
for p in $PARTS; do
	for s in $SLOTS; do
		name="${p}_${s}"
		out="$BACKUP_DIR/$name.img"
		if [ -e "$out" ]; then
			die "$out už existuje, nepřepisuji"
		fi
		adb exec-out "cat $byname/$name" > "$out.part"
		remote=$(adb shell "sha256sum $byname/$name" | cut -d' ' -f1)
		local_sum=$(sha256sum "$out.part" | cut -d' ' -f1)
		[ "$remote" = "$local_sum" ] ||
			die "kontrolní součet $name nesouhlasí (telefon $remote, PC $local_sum)"
		mv "$out.part" "$out"
		chmod a-w "$out"
		echo "$local_sum  $name.img" >> "$BACKUP_DIR/SHA256SUMS"
		echo "   $name.img  $(stat -c %s "$out") B  ok"
	done
done

echo "== Info o hardwaru -> $INFO_DIR"
# Sériová čísla, MAC adresy a UID do props.txt neukládáme;
# cmdline.txt, dmesg.txt a live.dtb je obsahovat mohou.
adb shell getprop |
	grep -E '^\[ro\.(boot\.|bootloader|product\.device|build\.fingerprint|hardware|board\.platform)' |
	grep -v -i -E 'serial|imei|macaddr|\.uid\]' > "$INFO_DIR/props.txt" || true

# DTB tak, jak ho kernelu předal bootloader (po overlayi a doplnění paměti).
adb exec-out "cat /sys/firmware/fdt" > "$INFO_DIR/live.dtb"
if [ "$(head -c 4 "$INFO_DIR/live.dtb" | od -An -tx1 | tr -d ' \n')" != "d00dfeed" ]; then
	echo "   POZOR: live.dtb nemá hlavičku FDT" >&2
fi

adb shell cat /proc/iomem > "$INFO_DIR/iomem.txt"
adb shell cat /proc/cmdline > "$INFO_DIR/cmdline.txt"
adb shell cat /proc/meminfo > "$INFO_DIR/meminfo.txt"
adb shell dmesg > "$INFO_DIR/dmesg.txt" || true
adb shell 'for f in /sys/devices/soc0/*; do [ -f "$f" ] && echo "$f: $(cat "$f" 2>/dev/null)"; done' \
	> "$INFO_DIR/soc0.txt" || true
# Klidový stav GPIO tlačítek (polarita); debugfs nemusí být připojený.
adb shell 'cat /sys/kernel/debug/gpio 2>/dev/null' > "$INFO_DIR/gpio.txt" || true

ls -l "$INFO_DIR"
echo "Hotovo."
