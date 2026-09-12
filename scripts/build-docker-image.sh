#!/usr/bin/env bash
# Build the XOS cross-compile Docker image
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="${XOS_DOCKER_IMAGE:-xos-build:latest}"
DOCKERFILE="${ROOT}/docker/Dockerfile"

docker build -t "$IMAGE" -f "$DOCKERFILE" "${ROOT}/docker"
echo "Built image: ${IMAGE}"
