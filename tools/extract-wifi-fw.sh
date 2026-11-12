#!/usr/bin/env bash
# Vytáhne firmware rádia a jeho konfiguraci pro troiku ze zálohy oddílu vendor
# do dl/wifi-fw/motorola/troika (dl/ se necommituje). Zálohu jen čte: debugfs
# otevírá obraz pouze pro čtení, nic se nepřipojuje.
#
#   tools/extract-wifi-fw.sh [OBRAZ]     výchozí ~/troika-backup/vendor_a.img
#
# Rozložení odpovídá /vendor/etc/wifi původního systému, kde soubory hledá
# ovladač (mx140.bin, mx140/conf/<platforma>/wlan/*.hcf). Adresu MAC nečte.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
img="${1:-$HOME/troika-backup/vendor_a.img}"
plat=leman_s620_troika_dualfem
out="$root/dl/wifi-fw/motorola/troika"

[ -r "$img" ] || { echo "CHYBA: nejde číst $img" >&2; exit 1; }
rm -rf "$root/dl/wifi-fw"
mkdir -p "$out/mx140/conf/$plat/wlan"
get() {
	debugfs -R "dump $1 $2" "$img" 2>/dev/null
	[ -s "$2" ] || { echo "CHYBA: v obrazu chybí $1" >&2; exit 1; }
}
get /etc/wifi/mx140.bin "$out/mx140.bin"
for f in wlan.hcf wlan_sw.hcf wlan_t.hcf wlan_t_sw.hcf platform.txt; do
	get "/etc/wifi/mx140/conf/$plat/wlan/$f" "$out/mx140/conf/$plat/wlan/$f"
done
# Přenesený ovladač hledá konfiguraci „naplocho“ vedle firmwaru
# (mx140_wlan.hcf …, volba force_flat), proto ještě odkazy s těmi jmény.
for f in wlan.hcf wlan_sw.hcf wlan_t.hcf wlan_t_sw.hcf; do
	ln -s "mx140/conf/$plat/wlan/$f" "$out/mx140_$f"
done
(cd "$root/dl/wifi-fw" && find . -type f | sort | xargs sha256sum > SHA256SUMS)
find "$out" -type f | sort | xargs ls -l | awk '{print $5, $9}'
