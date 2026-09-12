# XOS - eXtendable OS

It is not really extendable.

## Docker

Cross-build environment: Ubuntu 24.04 with **i686-elf GCC** (built during image build), **nasm**, and **cmake**. The repo is bind-mounted at **`/work`** when you use `docker-run.sh`.

### Environment

| Variable | Default | Used by |
|----------|---------|---------|
| `XOS_DOCKER_IMAGE` | `xos-build:latest` | `build-docker-image.sh`, `docker-run.sh` |
| `XOS_BUILD_DIR` | `build` | `build.sh` (output directory name) |

### Scripts

**`scripts/build-docker-image.sh`** — build (or rebuild) the image from `docker/Dockerfile`:

```bash
./scripts/build-docker-image.sh
```

**`scripts/docker-run.sh`** — run the image with the repository mounted at `/work`, as your host uid/gid (so files in `build/` are not owned by root):

```bash
# interactive shell
./scripts/docker-run.sh
```

Without arguments it starts **`bash`** in the container (`-it`). With arguments it runs that command and exits (no TTY).
