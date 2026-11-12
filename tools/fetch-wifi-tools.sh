#!/usr/bin/env bash
# Zkopíruje wpa_supplicant, wpa_cli a jejich knihovny z chrootu systému
# pmbootstrapu (jen čtení) do dl/wifi-tools, odkud je bere testovací image
# (test WLAN2: hledání sítí a připojení bez plného systému).
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
. "$root/tools/env.sh"
chroot="${1:-$HOME/.local/var/pmbootstrap/chroot_rootfs_motorola-troika}"
out="$root/dl/wifi-tools"

[ -x "$chroot/usr/sbin/wpa_supplicant" ] || { echo "CHYBA: v $chroot není wpa_supplicant" >&2; exit 1; }
rm -rf "$out"
mkdir -p "$out/bin" "$out/lib"
install -m 755 "$chroot/usr/sbin/wpa_supplicant" "$chroot/usr/bin/wpa_cli" "$out/bin/"
python3 -I - "$chroot" "$out" <<'PY'
import os, shutil, subprocess, sys
chroot, out = sys.argv[1:]
def needed(path):
    txt = subprocess.run(['aarch64-linux-readelf', '-d', path],
                         capture_output=True, text=True, check=True).stdout
    return [l.split('[')[1].split(']')[0] for l in txt.splitlines() if 'NEEDED' in l]
libs, todo = {}, [os.path.join(out, 'bin', b) for b in ('wpa_supplicant', 'wpa_cli')]
while todo:
    for name in needed(todo.pop()):
        if name in libs or name.startswith('libc.musl'):
            continue
        for d in ('usr/lib', 'lib'):
            f = os.path.join(chroot, d, name)
            if os.path.exists(f):
                libs[name] = os.path.realpath(f)
                todo.append(libs[name])
                break
        else:
            sys.exit('CHYBA: chybí knihovna ' + name)
for name, f in libs.items():
    shutil.copyfile(f, os.path.join(out, 'lib', name))
PY
(cd "$out" && find . -type f | sort | xargs ls -l | awk '{print $5, $9}')
