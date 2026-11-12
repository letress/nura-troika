#!/usr/bin/env bash
# Připraví zdrojáky ovladače WiFi (Samsung SCSC 11.19) v build/scsc:
# původní soubory z refs/gta4xl-kernel (LineageOS/android_kernel_samsung_gta4xl,
# commit 8d1f7a68b0d0) ověřené proti součtům portu gta4xl, jeho dva patche pro
# Linux 7.3 a naše nastavení z wifi/ (troika.h, Makefile).
#
#   tools/prepare-wifi.sh          připraví build/scsc (smaže starý)
#   tools/prepare-wifi.sh build    navíc přeloží moduly proti build/linux
#
# Na telefon nesahá.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/refs/gta4xl-kernel"
port="$root/refs/gta4xl-linux/patches/upstream-wifi"
dst="$root/build/scsc"

[ -d "$src/drivers/misc/samsung/scsc" ] || { echo "CHYBA: chybí $src" >&2; exit 1; }
rm -rf "$dst"
python3 -I - "$src" "$port" "$dst" <<'PY'
import hashlib, json, shutil, sys
from pathlib import Path
src, port, dst = map(Path, sys.argv[1:])
for name, want in json.loads((port / 'import-sha256.json').read_text()).items():
    if name.startswith('core/'):
        f = src / 'drivers/misc/samsung/scsc' / name[5:]
    elif name.startswith('wlan/'):
        f = src / 'drivers/net/wireless/scsc' / name[5:]
    elif name.startswith('bt/'):
        f = src / 'drivers/misc/samsung/scsc_bt' / name[3:]
    else:
        f = src / name
    if hashlib.sha256(f.read_bytes()).hexdigest() != want:
        sys.exit('CHYBA: jiný obsah ' + str(f))
    t = dst / name
    t.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(f, t)
PY
for p in "$port"/[0-9][0-9][0-9][0-9]-*.patch; do
	patch --batch -s -p1 -d "$dst" < "$p"
done
# Port gta4xl má umístění firmwaru tabletu na dvou místech natvrdo; nahradit
# nastavením z troika.h. Kontrola, že se obě místa opravdu změnila.
sed -i 's|= "samsung/gta4xl";  /\* fixed in defconfig \*/|= CONFIG_SCSC_CORE_FW_LOCATION;|' "$dst/core/mx140_file.c"
sed -i 's|"samsung/gta4xl/slsi_reg_database.bin"|CONFIG_SCSC_CORE_FW_LOCATION "/slsi_reg_database.bin"|' "$dst/wlan/reg_info.c"
if grep -rn "samsung/gta4xl" "$dst/core" "$dst/wlan"; then
	echo "CHYBA: v ovladači zůstala cesta tabletu" >&2
	exit 1
fi
install -m 644 "$root/wifi/troika.h" "$root/wifi/Makefile" "$dst/"
echo "připraveno: $dst"

if [ "${1:-}" = build ]; then
	. "$root/tools/env.sh"
	make -s -C "$root/linux" O="$root/build/linux" LOCALVERSION= \
		M="$dst" KBUILD_MODPOST_WARN=1 -j"$(nproc)" modules
	ls -l "$dst"/*.ko
fi
