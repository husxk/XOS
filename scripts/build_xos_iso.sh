#!/usr/bin/env bash
# Stage boot/grub/grub.cfg + kernel.elf and run grub-mkrescue.

set -euo pipefail

usage() {
    cat >&2 <<'EOF'
Usage: build_xos_iso.sh --kernel PATH --grub-cfg PATH --output PATH --staging DIR
                      [--grub-mkrescue PATH]

  --kernel         kernel.elf to install as /boot/kernel.elf on the ISO
  --grub-cfg       grub.cfg source (copied to /boot/grub/grub.cfg)
  --output         xos.iso path to write
  --staging        temporary ISO root (removed and recreated)
  --grub-mkrescue  grub-mkrescue executable (default: grub-mkrescue in PATH)
EOF
    exit 2
}

KERNEL_ELF=""
GRUB_CFG=""
ISO_OUT=""
STAGING=""
GRUB_MKRESCUE="${GRUB_MKRESCUE:-grub-mkrescue}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --kernel) KERNEL_ELF=$2; shift 2 ;;
        --grub-cfg) GRUB_CFG=$2; shift 2 ;;
        --output) ISO_OUT=$2; shift 2 ;;
        --staging) STAGING=$2; shift 2 ;;
        --grub-mkrescue) GRUB_MKRESCUE=$2; shift 2 ;;
        -h|--help) usage ;;
        *)
            echo "build_xos_iso.sh: unknown argument: $1" >&2
            usage
            ;;
    esac
done

if [[ -z "$KERNEL_ELF" || -z "$GRUB_CFG" || -z "$ISO_OUT" || -z "$STAGING" ]]; then
    usage
fi

if [[ ! -f "$KERNEL_ELF" ]]; then
    echo "build_xos_iso.sh: kernel not found: ${KERNEL_ELF}" >&2
    exit 1
fi

if [[ ! -f "$GRUB_CFG" ]]; then
    echo "build_xos_iso.sh: grub.cfg not found: ${GRUB_CFG}" >&2
    exit 1
fi

if ! command -v "$GRUB_MKRESCUE" >/dev/null 2>&1; then
    echo "build_xos_iso.sh: grub-mkrescue not found: ${GRUB_MKRESCUE}" >&2
    exit 1
fi

rm -rf "$STAGING"
mkdir -p "${STAGING}/boot/grub"
cp -- "$GRUB_CFG" "${STAGING}/boot/grub/grub.cfg"
cp -- "$KERNEL_ELF" "${STAGING}/boot/kernel.elf"

mkdir -p "$(dirname "$ISO_OUT")"
exec "$GRUB_MKRESCUE" -o "$ISO_OUT" "$STAGING"
