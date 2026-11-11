#!/usr/bin/env bash
# Run host unit tests (build first: ./scripts/build.sh --tests).
# Add --sanitize to run the ThreadSanitizer + UBSan suite (build it with
# ./scripts/build.sh --sanitize). Any other arguments pass through to ctest.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SANITIZE=0
CTEST_ARGS=()
for arg in "$@"; do
  case "$arg" in
    --sanitize) SANITIZE=1 ;;
    *)          CTEST_ARGS+=("$arg") ;;
  esac
done

if [[ "$SANITIZE" -eq 1 ]]; then
  TEST_BUILD_DIR="${XOS_SAN_BUILD_DIR:-build-san}"
  BUILD_HINT="./scripts/build.sh --sanitize"
else
  TEST_BUILD_DIR="${XOS_TEST_BUILD_DIR:-build-tests}"
  BUILD_HINT="./scripts/build.sh --tests"
fi

if [[ ! -d "${ROOT}/${TEST_BUILD_DIR}" ]]; then
  echo "Missing ${ROOT}/${TEST_BUILD_DIR}. Build tests first:" >&2
  echo "  ${BUILD_HINT}" >&2
  exit 1
fi

exec ctest --test-dir "${ROOT}/${TEST_BUILD_DIR}" --output-on-failure "${CTEST_ARGS[@]}"
