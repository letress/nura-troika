#!/usr/bin/env bash
# Přenese konfiguraci z build/linux a patche z větve troika-mainline do pmaports
# balíčku linux-motorola-troika-mainline a přepočítá sha512sums.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
pkg="$root/pmaports/device/testing/linux-motorola-troika-mainline"
cfg="config-motorola-troika-mainline.aarch64"

uni_commit=30442c60e34de64f4cc7c4236f878e84b4516d86

rm -f "$pkg"/*.patch "$pkg"/uniloader-*.diff
git -C "$root/linux" format-patch -q --no-signature -o "$pkg" v7.3.3..troika-mainline
cp "$root/build/linux/.config" "$pkg/$cfg"

# uniLoader: jen dva patche z větve troika-pkg (board + převzetí initrd/bootargs).
# Přípona .diff, aby je default_prepare neaplikoval na strom kernelu.
tmp="$(mktemp -d)"
git -C "$root/uniLoader-src" format-patch -q --no-signature -o "$tmp" "$uni_commit"..troika-pkg
for p in "$tmp"/*.patch; do
	cp "$p" "$pkg/uniloader-$(basename "${p%.patch}").diff"
done
rm -rf "$tmp"

sums="$(cd "$root/dl" && sha512sum linux-7.3.3.tar.xz "uniLoader-$uni_commit.tar.gz")
$(cd "$pkg" && sha512sum "$cfg" uniloader-*.diff *.patch)"
python3 -I - "$pkg/APKBUILD" "$sums" <<'PY'
import re, sys
p, sums = sys.argv[1], sys.argv[2]
t = open(p).read()
t = re.sub(r'sha512sums="\n.*?\n"', lambda m: 'sha512sums="\n' + sums + '\n"', t, flags=re.S)
open(p, 'w').write(t)
PY
echo "aktualizováno: $(ls "$pkg" | wc -l) souborů v $pkg"
