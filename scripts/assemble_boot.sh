#!/usr/bin/env bash
# Assemble boot.asm with KERNEL_SECTORS derived from kernel.bin size.

set -euo pipefail

usage() {
    cat >&2 <<'EOF'
Usage: assemble_boot.sh --kernel PATH --boot-asm PATH --output PATH [--nasm PATH]

  --kernel    Raw kernel image (kernel.bin); size determines sectors to load
  --boot-asm  boot.asm source
  --output    boot.bin path to write
  --nasm      NASM executable (default: nasm, or NASM env var)
EOF
    exit 2
}

KERNEL_BIN=""
BOOT_ASM=""
BOOT_OUT=""
NASM="${NASM:-nasm}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --kernel) KERNEL_BIN=$2; shift 2 ;;
        --boot-asm) BOOT_ASM=$2; shift 2 ;;
        --output) BOOT_OUT=$2; shift 2 ;;
        --nasm) NASM=$2; shift 2 ;;
        -h|--help) usage ;;
        *)
            echo "assemble_boot.sh: unknown argument: $1" >&2
            usage
            ;;
    esac
done

if [[ -z "$KERNEL_BIN" || -z "$BOOT_ASM" || -z "$BOOT_OUT" ]]; then
    usage
fi

if [[ ! -f "$KERNEL_BIN" ]]; then
    echo "assemble_boot.sh: kernel not found: ${KERNEL_BIN}" >&2
    exit 1
fi

if [[ ! -f "$BOOT_ASM" ]]; then
    echo "assemble_boot.sh: boot.asm not found: ${BOOT_ASM}" >&2
    exit 1
fi

if ! command -v "$NASM" >/dev/null 2>&1; then
    echo "assemble_boot.sh: nasm not found: ${NASM}" >&2
    exit 1
fi

bytes=$(stat -c%s "$KERNEL_BIN")
sectors=$(( (bytes + 511) / 512 ))

if (( sectors < 1 )); then
    echo "assemble_boot.sh: kernel size is 0 bytes" >&2
    exit 1
fi

if (( sectors > 127 )); then
    echo "assemble_boot.sh: ${sectors} sectors exceeds INT 13h AL limit (127); add a read loop in boot.asm" >&2
    exit 1
fi

exec "$NASM" -f bin -dKERNEL_SECTORS="$sectors" -o "$BOOT_OUT" "$BOOT_ASM"
