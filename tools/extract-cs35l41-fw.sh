#!/usr/bin/env bash
# Vytáhne firmware ochrany reproduktoru (CS35L41) ze zálohy oddílu vendor do
# dl/cs35l41-fw (dl/ se necommituje). Zálohu jen čte (debugfs).
# Reproduktor tohoto telefonu je Qisheng (FINDINGS 33.20), ladění „music“.
#
#   tools/extract-cs35l41-fw.sh [OBRAZ]     výchozí ~/troika-backup/vendor_a.img
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
img="${1:-$HOME/troika-backup/vendor_a.img}"
out="$root/dl/cs35l41-fw"

[ -r "$img" ] || { echo "CHYBA: nejde číst $img" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out"
for f in cs35l41-dsp1-prot.wmfw cs35l41-dsp1-spk-prot-music-qisheng.bin; do
	debugfs -R "dump /firmware/$f $out/$f" "$img" 2>/dev/null
	[ -s "$out/$f" ] || { echo "CHYBA: v obrazu chybí /firmware/$f" >&2; exit 1; }
done
(cd "$out" && sha256sum cs35l41-*.* | tee SHA256SUMS)
