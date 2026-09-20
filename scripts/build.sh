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

KERNEL_ELF="${ROOT}/${BUILD_DIR}/kernel/kernel.elf"
XOS_ISO="${ROOT}/${BUILD_DIR}/xos.iso"

echo -ne "\n\n"
if [[ ! -f "${KERNEL_ELF}" ]]; then
  echo "Expected kernel image missing: ${KERNEL_ELF}" >&2
  exit 1
fi

if [[ ! -f "${XOS_ISO}" ]]; then
  echo "Expected boot ISO missing: ${XOS_ISO}" >&2
  echo "  Install grub-mkrescue (Debian/Ubuntu: grub-common grub-pc-bin xorriso)" >&2
  exit 1
fi

echo "Built: ${XOS_ISO}"
echo "  Run: ./scripts/run-qemu.sh"

echo -ne "\n"

if command -v "${CROSS_PREFIX}-size" >/dev/null 2>&1; then
  echo "ELF memory (${CROSS_PREFIX}-size):"
  "${CROSS_PREFIX}-size" --format=berkeley "${KERNEL_ELF}"
fi
