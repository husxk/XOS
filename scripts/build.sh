#!/usr/bin/env bash
# Configure and build XOS with CMake.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${XOS_BUILD_DIR:-build}"
TOOLCHAIN="${ROOT}/cmake/i686-elf-toolchain.cmake"

cmake -S "$ROOT" -B "${ROOT}/${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN}"

cmake --build "${ROOT}/${BUILD_DIR}" "$@"

echo "Built: ${ROOT}/${BUILD_DIR}/os.img"
