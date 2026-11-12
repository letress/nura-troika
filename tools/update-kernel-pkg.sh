#!/usr/bin/env bash
# Přenese patche z větve kernelu (výchozí troika-clk: základ + hodiny + USB,
# bez ladicích commitů z troika-clk-test) do pmaports balíčku
# linux-motorola-troika-mainline, přepíše seznam patchů v source= a přepočítá
# sha512sums. Jiná větev: KBRANCH=troika-mainline tools/update-kernel-pkg.sh
# Volitelný argument: soubor s konfigurací kernelu, kterým se nahradí ta
# v balíčku. Konfigurace v balíčku má odpovídat kompilátoru v chrootu
# pmbootstrapu (jinak se oldconfig při sestavení ptá na nové volby), proto se
# z build/linux už nekopíruje sama.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
pkg="$root/pmaports/device/testing/linux-motorola-troika-mainline"
cfg="config-motorola-troika-mainline.aarch64"

uni_commit=30442c60e34de64f4cc7c4236f878e84b4516d86

rm -f "$pkg"/*.patch "$pkg"/uniloader-*.diff
kbranch="${KBRANCH:-troika-clk}"
git -C "$root/linux" format-patch -q --no-signature -o "$pkg" "v7.3.3..$kbranch"
if [ $# -ge 1 ]; then
	cp "$1" "$pkg/$cfg"
fi

# uniLoader: jen dva patche z větve troika-pkg (board + převzetí initrd/bootargs).
# Přípona .diff, aby je default_prepare neaplikoval na strom kernelu.
tmp="$(mktemp -d)"
git -C "$root/uniLoader-src" format-patch -q --no-signature -o "$tmp" "$uni_commit"..troika-pkg
for p in "$tmp"/*.patch; do
	cp "$p" "$pkg/uniloader-$(basename "${p%.patch}").diff"
done
rm -rf "$tmp"

sums="$(cd "$root/dl" && sha512sum linux-7.3.3.tar.xz "uniLoader-$uni_commit.tar.gz")
$(cd "$pkg" && sha512sum "$cfg" $(ls scsc-wlbt-troika.tar.gz 2>/dev/null) uniloader-*.diff *.patch)"
patches="$(cd "$pkg" && ls *.patch)"
python3 -I - "$pkg/APKBUILD" "$sums" "$patches" <<'PY'
import re, sys
p, sums, patches = sys.argv[1], sys.argv[2], sys.argv[3].split()
t = open(p).read()
# source=: řádky s patchi kernelu nahradit aktuálním seznamem, ostatní nechat
m = re.search(r'source="\n(.*?)\n"', t, flags=re.S)
keep = [l for l in m.group(1).split('\n') if not re.match(r'\t\d{4}-.*\.patch$', l)]
t = t[:m.start(1)] + '\n'.join(keep + ['\t' + x for x in patches]) + t[m.end(1):]
t = re.sub(r'sha512sums="\n.*?\n"', lambda m: 'sha512sums="\n' + sums + '\n"', t, flags=re.S)
open(p, 'w').write(t)
PY
echo "aktualizováno: $(ls "$pkg" | wc -l) souborů v $pkg"
