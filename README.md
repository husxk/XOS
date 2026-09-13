# XOS - eXtendable OS

It is not really extendable.

## Building

XOS is built with **CMake** and a freestanding **i686-elf** toolchain (`cmake/i686-elf-toolchain.cmake` sets the cross compiler, **nasm**, and **objcopy**). The host `gcc` is not used for the kernel.

| Stage | Output |
|-------|--------|
| `boot/boot.asm` | `boot.bin` — 512-byte boot sector |
| `kernel/` (entry asm + C) | `kernel.bin` — flat binary loaded at 0x1000 |
| concatenate | `os.img` — boot sector + kernel (what QEMU runs) |

Default build directory: **`build/`** (override with `XOS_BUILD_DIR`).

```bash
./scripts/build.sh
```

Run the image on the host (requires QEMU with curses):

```bash
./scripts/run-qemu.sh
```

## Docker

Cross-build environment: Ubuntu 24.04 with **i686-elf GCC** (built during image build), **nasm**, and **cmake**. The repo is bind-mounted at **`/work`** when you use `docker-run.sh`.

### Environment

| Variable | Default | Used by |
|----------|---------|---------|
| `XOS_DOCKER_IMAGE` | `xos-build:latest` | `build-docker-image.sh`, `docker-run.sh` |
| `XOS_BUILD_DIR` | `build` | `build.sh`, `run-qemu.sh` |

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

**`scripts/build.sh`** — configure and build; produces **`build/os.img`**.

**`scripts/run-qemu.sh`** — run **`build/os.img`** in QEMU (`-display curses`).
