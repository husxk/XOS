#!/usr/bin/env bash
# Run host unit tests (build first: ./scripts/build.sh --tests).
# Add --sanitize to use the ThreadSanitizer + UBSan build (./scripts/build.sh --sanitize).
# Add --soak to run the long-running SPSC soak binary directly instead of ctest;
# pass its duration in seconds as an argument (0 = forever), e.g.
#   ./scripts/run-test.sh --soak 0 --sanitize
# With ctest, extra arguments pass through to ctest instead.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SANITIZE=0
SOAK=0
REST_ARGS=()
for arg in "$@"; do
  case "$arg" in
    --sanitize) SANITIZE=1 ;;
    --soak)     SOAK=1 ;;
    *)          REST_ARGS+=("$arg") ;;
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

if [[ "$SOAK" -eq 1 ]]; then
  SOAK_BIN="${ROOT}/${TEST_BUILD_DIR}/xos_soak_test"
  if [[ ! -x "${SOAK_BIN}" ]]; then
    echo "Missing ${SOAK_BIN}. Build tests first:" >&2
    echo "  ${BUILD_HINT}" >&2
    exit 1
  fi
  echo "Soaking: ${SOAK_BIN} (seconds: ${REST_ARGS[0]:-10})" >&2
  exec "${SOAK_BIN}" "${REST_ARGS[@]}"
fi

exec ctest --test-dir "${ROOT}/${TEST_BUILD_DIR}" --output-on-failure "${REST_ARGS[@]}"
