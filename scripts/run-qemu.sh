#!/usr/bin/env bash
# Run XOS in QEMU with a curses display.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${XOS_BUILD_DIR:-build}"
XOS_ISO="${ROOT}/${BUILD_DIR}/xos.iso"

if [[ ! -f "${XOS_ISO}" ]]; then
  echo "Missing ${XOS_ISO}. Build first:" >&2
  echo "  ./scripts/build.sh" >&2
  echo "  ./scripts/docker-run.sh ./scripts/build.sh" >&2
  exit 1
fi

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
  echo "qemu-system-x86_64 not found in PATH." >&2
  exit 1
fi

exec qemu-system-x86_64 -machine pc -boot order=d \
  -display curses \
  -cdrom "${XOS_ISO}" \
  "$@"
