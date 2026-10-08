#!/usr/bin/env bash
# Run host unit tests (build first: ./scripts/build.sh --tests).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_BUILD_DIR="${XOS_TEST_BUILD_DIR:-build-tests}"

if [[ ! -d "${ROOT}/${TEST_BUILD_DIR}" ]]; then
  echo "Missing ${ROOT}/${TEST_BUILD_DIR}. Build tests first:" >&2
  echo "  ./scripts/build.sh --tests" >&2
  exit 1
fi

exec ctest --test-dir "${ROOT}/${TEST_BUILD_DIR}" --output-on-failure "$@"
