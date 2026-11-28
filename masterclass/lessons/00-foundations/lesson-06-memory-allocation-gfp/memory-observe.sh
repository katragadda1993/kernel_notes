#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
set -eu

show_file() {
    file=$1
    title=$2
    if [ -r "$file" ]; then
        printf '\n== %s (%s) ==\n' "$title" "$file"
        cat "$file"
    else
        printf '\n== %s unavailable (%s) ==\n' "$title" "$file"
    fi
}

printf 'Kernel: '
uname -r
printf 'Architecture: '
uname -m
show_file /proc/meminfo 'Memory summary'
show_file /proc/buddyinfo 'Buddy allocator free blocks by order and zone'
show_file /proc/pagetypeinfo 'Page allocation/migratetype details'
if [ -r /proc/slabinfo ]; then
    printf '\n== First slabinfo entries ==\n'
    sed -n '1,18p' /proc/slabinfo
else
    printf '\n== /proc/slabinfo unavailable ==\n'
fi
if [ -d /sys/kernel/slab ]; then
    printf '\n== Sample sysfs slab cache names ==\n'
    find /sys/kernel/slab -mindepth 1 -maxdepth 1 -type d -printf '%f\n' 2>/dev/null | sort | head -20 || true
else
    printf '\n== /sys/kernel/slab unavailable ==\n'
fi
printf '\nNote: these are snapshots. They do not prove that a particular GFP/order allocation will succeed.\n'
