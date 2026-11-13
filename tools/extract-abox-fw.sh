#!/usr/bin/env bash
# Vytáhne firmware zvukové jednotky (Calliope) ze zálohy oddílu vendor do
# dl/abox-fw (dl/ se necommituje). Zálohu jen čte (debugfs, bez připojení).
#
#   tools/extract-abox-fw.sh [OBRAZ]     výchozí ~/troika-backup/vendor_a.img
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
img="${1:-$HOME/troika-backup/vendor_a.img}"
out="$root/dl/abox-fw"

[ -r "$img" ] || { echo "CHYBA: nejde číst $img" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out"
for f in calliope_sram.bin calliope_dram.bin; do
	debugfs -R "dump /firmware/$f $out/$f" "$img" 2>/dev/null
	[ -s "$out/$f" ] || { echo "CHYBA: v obrazu chybí /firmware/$f" >&2; exit 1; }
done
(cd "$out" && sha256sum calliope_*.bin | tee SHA256SUMS)
ls -l "$out" | awk 'NR>1 {print $5, $9}'
