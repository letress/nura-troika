#!/usr/bin/env bash
# Zazálohuje z troiky (LineageOS, adb root) všechny zbývající oddíly kromě
# velkých, obnovitelných (userdata, super).
#
# Na telefonu POUZE ČTE. Nic na něj nezapisuje, nepoužívá fastboot.
# Soubory, které v záloze už jsou, přeskakuje a nepřepisuje.
#
#   BACKUP_DIR  cíl (výchozí ~/troika-backup)
#   MAX_MIB     oddíly větší než tolik MiB se přeskočí (výchozí 1024)
set -euo pipefail

BACKUP_DIR="${BACKUP_DIR:-$HOME/troika-backup}"
MAX_MIB="${MAX_MIB:-1024}"
# Uživatelská data a systém: velké, nejsou potřeba k oživení telefonu.
SKIP="userdata super"

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

byname=/dev/block/by-name
adb shell "[ -e $byname/boot_a ]" || die "nenašel jsem $byname/boot_a"

mkdir -p "$BACKUP_DIR"
list="$BACKUP_DIR/partitions.txt"
adb shell "for p in $byname/*; do echo \"\$(blockdev --getsize64 \$p) \$(basename \$p) \$(readlink -f \$p)\"; done" |
	tr -d '\r' | sort -k2 > "$list"
echo "== $(wc -l < "$list") oddílů, seznam v $list"

done_n=0 skip_n=0 warn_n=0
while read -r size name _; do
	out="$BACKUP_DIR/$name.img"
	mib=$((size / 1048576))
	if [ -e "$out" ]; then
		echo "   $name: už v záloze, přeskakuji"
		continue
	fi
	case " $SKIP " in *" $name "*)
		echo "   $name: $mib MiB, vynecháno záměrně"
		skip_n=$((skip_n + 1))
		continue ;;
	esac
	if [ "$mib" -gt "$MAX_MIB" ]; then
		echo "   $name: $mib MiB > $MAX_MIB MiB, přeskočeno (zvyš MAX_MIB)"
		skip_n=$((skip_n + 1))
		continue
	fi

	# Připojený oddíl se může mezi čtením a součtem změnit; zkus to dvakrát.
	ok=0
	for _try in 1 2; do
		adb exec-out "cat $byname/$name" < /dev/null > "$out.part"
		remote=$(adb shell "sha256sum $byname/$name" < /dev/null | cut -d' ' -f1)
		local_sum=$(sha256sum "$out.part" | cut -d' ' -f1)
		if [ "$remote" = "$local_sum" ]; then
			ok=1
			break
		fi
	done
	[ "$(stat -c %s "$out.part")" = "$size" ] ||
		die "$name: načteno $(stat -c %s "$out.part") B, oddíl má $size B"

	mv "$out.part" "$out"
	chmod a-w "$out"
	echo "$local_sum  $name.img" >> "$BACKUP_DIR/SHA256SUMS"
	if [ "$ok" = 1 ]; then
		echo "   $name.img  $size B  ok"
	else
		echo "   $name.img  $size B  POZOR: oddíl se během čtení měnil (je připojený)"
		echo "$name" >> "$BACKUP_DIR/CHANGED-DURING-READ.txt"
		warn_n=$((warn_n + 1))
	fi
	done_n=$((done_n + 1))
done < "$list"

echo "Hotovo: $done_n nových, $skip_n vynecháno, $warn_n s varováním."
du -sh "$BACKUP_DIR" | cut -f1
