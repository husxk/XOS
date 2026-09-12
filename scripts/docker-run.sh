#!/usr/bin/env bash
# Run the XOS build image with the repo mounted at /work.
# Interactive shell: ./scripts/docker-run.sh
# One-shot command:  ./scripts/docker-run.sh ./scripts/build.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${XOS_DOCKER_IMAGE:-xos-build:latest}"

set +e
inspect_out=$(docker image inspect "$IMAGE" 2>&1)
inspect_rc=$?
set -e
if (( inspect_rc != 0 )); then
  echo "$inspect_out" >&2
  exit "$inspect_rc"
fi

docker_args=(--rm -u "$(id -u):$(id -g)" -v "${ROOT}:/work" -w /work)

if [[ $# -eq 0 ]]; then
  docker_args=(-it "${docker_args[@]}")
  set -- bash
fi

exec docker run "${docker_args[@]}" "$IMAGE" "$@"
