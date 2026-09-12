#!/usr/bin/env bash
# Build i686-elf binutils + GCC (C only) into /opt/cross. Runs during docker build.
set -euo pipefail

PREFIX=/opt/cross
TARGET=i686-elf
GCC_VERSION=14.2.0
BINUTILS_VERSION=2.42
SRC=/tmp/cross-src

unset CXXFLAGS
export CXXFLAGS_FOR_BUILD="-g -O2 -std=c++11"

mkdir -p "$SRC"
cd "$SRC"

curl -LO "https://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VERSION}.tar.xz"
tar xf "binutils-${BINUTILS_VERSION}.tar.xz"
mkdir -p binutils-build && cd binutils-build
../binutils-${BINUTILS_VERSION}/configure --target="$TARGET" --prefix="$PREFIX" \
  --disable-nls --disable-werror --disable-multilib
make -j"$(nproc)"
make install
cd "$SRC"

curl -LO "https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"
tar xf "gcc-${GCC_VERSION}.tar.xz"
mkdir -p gcc-build && cd gcc-build
unset CXXFLAGS
../gcc-${GCC_VERSION}/configure --target="$TARGET" --prefix="$PREFIX" \
  --disable-nls --disable-libssp --without-headers \
  --enable-languages=c --disable-libstdc++-v3
make -j"$(nproc)" all-gcc all-target-libgcc
make install-gcc install-target-libgcc

rm -rf "$SRC"
"${PREFIX}/bin/${TARGET}-gcc" -dumpmachine
