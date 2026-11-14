#!/usr/bin/env bash
# Automatický test zvuku (ABOX): restart do bootloaderu -> čekání na fastboot ->
# fastboot boot obrazu z out/ -> čekání na USB síť -> příkazy na telefonu ->
# stažení logů.
#
#   tools/auto-test.sh [-n POKUSY] [-c 'PŘÍKAZ NA TELEFONU'] out/troika-test-NAME.img
#
# Příkaz na telefonu (výchozí: výpis dmesg k zvuku) běží v shellu testovacího
# initramfs přes tools/phone-sh.py. Skript sám nic nepíše do napájení ani do
# registrů; co se na telefonu provede, určuje jen -c.
# Z fastbootu povoluje jen: devices, getvar, boot, reboot. Všechno ostatní
# (flash, erase, oem, ...) odmítá napevno. Nepoužívá sudo, žádná hesla.
# Logy: logs/auto-test/NAME-ČAS-pokusN.log (adresář je v .gitignore).
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
tries=2
cmd="dmesg | grep -E 'cs35l41|DEBUG|abox|ACPMDBG' | tail -60"
FB_WAIT=60	# s čekání na fastboot
NET_WAIT=120	# s čekání na USB síť po startu
LOGDIR="$root/logs/auto-test"

while getopts "n:c:" o; do
	case "$o" in
	n) tries="$OPTARG" ;;
	c) cmd="$OPTARG" ;;
	*) echo "použití: $0 [-n pokusy] [-c příkaz] out/troika-test-NAME.img" >&2; exit 2 ;;
	esac
done
shift $((OPTIND - 1))
img="${1:-}"
[[ "$tries" =~ ^[1-9]$ ]] || { echo "CHYBA: počet pokusů 1 až 9" >&2; exit 2; }

# Obraz jen z out/ a jen troika-test-*.img
[ -n "$img" ] || { echo "CHYBA: chybí obraz" >&2; exit 2; }
img="$(realpath -e "$img")" || { echo "CHYBA: obraz neexistuje" >&2; exit 2; }
case "$img" in
"$root"/out/troika-test-*.img) ;;
*) echo "CHYBA: obraz musí být $root/out/troika-test-*.img" >&2; exit 2 ;;
esac

# Jediná cesta k fastbootu: povolené podpříkazy, nic jiného.
fb() {
	case "${1:-}" in
	devices | getvar | boot | reboot) ;;
	*) echo "ODMÍTNUTO: fastboot ${1:-} není povolen" >&2; return 99 ;;
	esac
	timeout 60 fastboot "$@"
}

phone() { timeout 40 python3 "$root/tools/phone-sh.py" "$1"; }

wait_for() {	# wait_for SEKUNDY PŘÍKAZ...
	local end=$((SECONDS + $1)); shift
	until "$@" >/dev/null 2>&1; do
		[ "$SECONDS" -lt "$end" ] || return 1
		sleep 2
	done
}
in_fastboot() { [ -n "$(fb devices 2>/dev/null)" ]; }

mkdir -p "$LOGDIR"
name="$(basename "$img" .img)"
ts="$(date +%Y%m%d-%H%M%S)"

for ((t = 1; t <= tries; t++)); do
	log="$LOGDIR/$name-$ts-pokus$t.log"
	echo "== pokus $t/$tries, $name" | tee "$log"

	if ! in_fastboot; then
		if phone true >/dev/null 2>&1; then
			echo "-- restart do bootloaderu" | tee -a "$log"
			phone "/bin/reboot-arg bootloader" >/dev/null 2>&1 || true
		else
			echo "-- telefon není vidět ani ve fastbootu, ani po síti" | tee -a "$log"
		fi
		if ! wait_for "$FB_WAIT" in_fastboot; then
			echo "ZASEKNUTO: fastboot se neobjevil do ${FB_WAIT} s; je třeba zásah uživatele (podržet tlačítka)" | tee -a "$log"
			exit 3
		fi
	fi

	echo "-- fastboot boot $name" | tee -a "$log"
	fb boot "$img" >>"$log" 2>&1 || { echo "CHYBA: fastboot boot selhal" | tee -a "$log"; continue; }

	if ! wait_for "$NET_WAIT" phone true; then
		echo "SELHÁNÍ: USB síť nenaběhla do ${NET_WAIT} s" | tee -a "$log"
		continue
	fi
	echo "-- telefon běží, spouštím příkaz" | tee -a "$log"
	phone "$cmd" 2>&1 | tee -a "$log"
	echo "-- log: $log"
	exit 0
done
echo "VŠECHNY POKUSY SELHALY" >&2
exit 1
