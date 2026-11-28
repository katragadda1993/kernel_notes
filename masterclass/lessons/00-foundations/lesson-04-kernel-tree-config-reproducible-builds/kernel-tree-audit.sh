#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'USAGE'
Usage: kernel-tree-audit.sh KERNEL_SOURCE OUTPUT_DIRECTORY [ARCH]

Non-building audit of a configured Linux source/output tree. It does not build,
install, boot, or load anything. Example:
  ./kernel-tree-audit.sh ~/src/linux ~/build/linux-x86 x86
USAGE
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" || $# -lt 2 || $# -gt 3 ]]; then
    usage
    [[ ${1:-} == "-h" || ${1:-} == "--help" ]] && exit 0
    exit 2
fi

SRC=$(cd "$1" && pwd -P)
OUT=$2
ARCH_ARG=${3:-${ARCH:-}}

if [[ -z "$ARCH_ARG" ]]; then
    case "$(uname -m)" in
        x86_64) ARCH_ARG=x86 ;;
        aarch64) ARCH_ARG=arm64 ;;
        armv*) ARCH_ARG=arm ;;
        riscv64) ARCH_ARG=riscv ;;
        *) ARCH_ARG=$(uname -m) ;;
    esac
fi

if [[ ! -f "$SRC/Makefile" || ! -f "$SRC/MAINTAINERS" || ! -d "$SRC/scripts" ]]; then
    echo "error: $SRC does not look like a Linux kernel source tree" >&2
    exit 1
fi

mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd -P)

printf 'source=%s\noutput=%s\narch=%s\n' "$SRC" "$OUT" "$ARCH_ARG"
if git -C "$SRC" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    printf 'commit='
    git -C "$SRC" rev-parse --short=12 HEAD
    printf 'describe='
    git -C "$SRC" describe --tags --always --dirty
    if [[ -n $(git -C "$SRC" status --porcelain) ]]; then
        echo 'source_worktree=modified'
    else
        echo 'source_worktree=clean'
    fi
else
    echo 'git_revision=unavailable (source archive or non-git tree)'
fi

printf 'kernel_version='
make -s -C "$SRC" O="$OUT" ARCH="$ARCH_ARG" kernelversion

if [[ -f "$OUT/.config" ]]; then
    echo 'config=present'
    if [[ -f "$OUT/include/config/auto.conf" ]]; then
        printf 'kernel_release='
        make -s -C "$SRC" O="$OUT" ARCH="$ARCH_ARG" kernelrelease
    else
        echo 'kernel_release=unavailable (configuration not prepared; run olddefconfig)'
    fi
    echo 'selected_config_symbols:'
    grep -E '^(CONFIG_(MODULES|MODULE_UNLOAD|DEBUG_INFO|KALLSYMS|KALLSYMS_ALL|GDB_SCRIPTS|IKCONFIG|IKCONFIG_PROC)=|# CONFIG_(MODULES|MODULE_UNLOAD|DEBUG_INFO|KALLSYMS|KALLSYMS_ALL|GDB_SCRIPTS|IKCONFIG|IKCONFIG_PROC) is not set)' "$OUT/.config" || true
else
    echo 'config=missing (run an architecture defconfig target first)'
fi

if [[ -d "$OUT/include/generated" ]]; then
    echo 'generated_headers=present'
else
    echo 'generated_headers=not-yet-generated'
fi

printf 'compiler_env: CC=%s CROSS_COMPILE=%s LLVM=%s\n' \
    "${CC:-default}" "${CROSS_COMPILE:-default}" "${LLVM:-default}"

echo 'audit=non-building (no kernel build, install, boot or module load performed)'
