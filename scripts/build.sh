#!/usr/bin/env bash
# Configure and build XOS with CMake.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${XOS_BUILD_DIR:-build}"
TOOLCHAIN="${ROOT}/cmake/i686-elf-toolchain.cmake"
CROSS_PREFIX="${XOS_CROSS_PREFIX:-i686-elf}"

cmake -S "$ROOT" -B "${ROOT}/${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}"

cmake --build "${ROOT}/${BUILD_DIR}" "$@"

OS_IMAGE="${ROOT}/${BUILD_DIR}/os.img"
KERNEL_ELF="${ROOT}/${BUILD_DIR}/kernel/kernel.elf"
KERNEL_BIN="${ROOT}/${BUILD_DIR}/kernel/kernel.bin"
BOOT_READ_SECTORS=32

echo -ne "\n\nBuilt: ${OS_IMAGE}\n"

if [[ -f "${KERNEL_BIN}" ]]; then
  bytes=$(stat -c%s "${KERNEL_BIN}")
  sectors=$(( (bytes + 511) / 512 ))
  boot_bytes=$((BOOT_READ_SECTORS * 512))
  echo "Kernel image: ${bytes} bytes, ${sectors} disk sector(s) (512 B each)"
  echo "  load address: 0x1000 (kernel starts at disk sector 2)"
  echo "  boot loader reads: ${BOOT_READ_SECTORS} sectors (${boot_bytes} bytes) from sector 2"
  if (( bytes > boot_bytes )); then
    echo "  WARNING: kernel.bin exceeds boot.asm read count — increase AL in boot.asm" >&2
  fi
fi

echo -ne "\n"

if [[ -f "${KERNEL_ELF}" ]] && command -v "${CROSS_PREFIX}-size" >/dev/null 2>&1; then
  echo "ELF memory (${CROSS_PREFIX}-size):"
  "${CROSS_PREFIX}-size" --format=berkeley "${KERNEL_ELF}"
fi
