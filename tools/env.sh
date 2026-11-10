# Prostředí pro build kernelu a uniLoaderu: . tools/env.sh
_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$_root/toolchain/gcc-16.2.0-nolibc/aarch64-linux/bin:$_root/toolchain/dtschema-venv/bin:$PATH"
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-
unset _root
