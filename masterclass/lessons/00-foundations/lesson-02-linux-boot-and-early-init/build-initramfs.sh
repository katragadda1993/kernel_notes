#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "Run this script as root (it uses chroot and attempts to create /dev nodes)." >&2
    exit 1
fi

for tool in busybox cpio gzip ldd chroot; do
    command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 1; }
done

SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
if [ "$#" -gt 0 ]; then
    OUT=$1
else
    OUT="$SCRIPT_DIR/initramfs.cpio.gz"
fi
mkdir -p "$(dirname -- "$OUT")"
OUT="$(cd -- "$(dirname -- "$OUT")" && pwd)/$(basename -- "$OUT")"
ROOTFS=$(mktemp -d)
trap 'rm -rf "$ROOTFS"' EXIT

mkdir -p "$ROOTFS/bin" "$ROOTFS/sbin" "$ROOTFS/etc" "$ROOTFS/proc" "$ROOTFS/sys" "$ROOTFS/dev"
BUSYBOX=$(command -v busybox)
cp -L "$BUSYBOX" "$ROOTFS/bin/busybox"

# Copy the dynamic loader and shared libraries required by this BusyBox build.
while IFS= read -r lib; do
    [ -n "$lib" ] || continue
    mkdir -p "$ROOTFS$(dirname -- "$lib")"
    cp -L "$lib" "$ROOTFS$lib"
done < <(ldd "$BUSYBOX" 2>/dev/null | awk '{ for (i = 1; i <= NF; i++) if (substr($i, 1, 1) == "/") print $i }' | sort -u)

chroot "$ROOTFS" /bin/busybox --install -s /bin
cp "$SCRIPT_DIR/init" "$ROOTFS/init"
chmod 0755 "$ROOTFS/init"

# These nodes let the kernel attach the initial console before /init runs.
# Some containers intentionally deny CAP_MKNOD; in that case the archive
# remains inspectable, but an interactive QEMU boot needs these nodes supplied.
mknod -m 0600 "$ROOTFS/dev/console" c 5 1 2>/dev/null || echo "warning: could not create /dev/console"
mknod -m 0666 "$ROOTFS/dev/null" c 1 3 2>/dev/null || echo "warning: could not create /dev/null"

# Exercise the init executable and its interpreter before packaging.
chroot "$ROOTFS" /init --self-test

# Linux initramfs uses a newc cpio archive; gzip is one supported compression.
(cd "$ROOTFS" && find . -print0 | cpio --null -o -H newc) | gzip -9 > "$OUT"
gzip -t "$OUT"

echo "Created: $OUT"
echo "Archive size: $(stat -c '%s bytes' "$OUT")"
echo "Archive members (first 25):"
gzip -dc "$OUT" | cpio -it 2>/dev/null | sed -n '1,25p'
