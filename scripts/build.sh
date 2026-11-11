#!/usr/bin/env bash
# Configure and build XOS with CMake, or host unit tests with --tests.
# Add --sanitize to build the tests under ThreadSanitizer + UBSan (runs the
# concurrency stress test under TSan); implies --tests.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${XOS_BUILD_DIR:-build}"
TOOLCHAIN="${ROOT}/cmake/i686-elf-toolchain.cmake"
CROSS_PREFIX="${XOS_CROSS_PREFIX:-i686-elf}"

BUILD_TESTS=0
SANITIZE=0
BUILD_ARGS=()

for arg in "$@"; do
  case "$arg" in
    --tests)    BUILD_TESTS=1 ;;
    --sanitize) SANITIZE=1 ;;
    *)          BUILD_ARGS+=("$arg") ;;
  esac
done

# --sanitize builds the test suite, so it implies --tests.
if [[ "$SANITIZE" -eq 1 ]]; then
  BUILD_TESTS=1
fi

if [[ "$BUILD_TESTS" -eq 1 ]]; then
  CMAKE_TEST_ARGS=()
  if [[ "$SANITIZE" -eq 1 ]]; then
    TEST_BUILD_DIR="${XOS_SAN_BUILD_DIR:-build-san}"
    CMAKE_TEST_ARGS+=(-DXOS_SANITIZE=ON)
  else
    TEST_BUILD_DIR="${XOS_TEST_BUILD_DIR:-build-tests}"
  fi

  cmake -S "${ROOT}/tests" -B "${ROOT}/${TEST_BUILD_DIR}" "${CMAKE_TEST_ARGS[@]}"
  cmake --build "${ROOT}/${TEST_BUILD_DIR}" "${BUILD_ARGS[@]}"

  echo -ne "\n\n"
  echo "Built host tests: ${ROOT}/${TEST_BUILD_DIR}"
  if [[ "$SANITIZE" -eq 1 ]]; then
    echo "  Run: ./scripts/run-test.sh --sanitize"
  else
    echo "  Run: ./scripts/run-test.sh"
  fi
  echo -ne "\n"
  exit 0
fi

cmake -S "$ROOT" -B "${ROOT}/${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}"

cmake --build "${ROOT}/${BUILD_DIR}" "${BUILD_ARGS[@]}"

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
