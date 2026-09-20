# Cross-compile XOS in this image
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH=/opt/cross/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    bison flex \
    libgmp-dev libmpfr-dev libmpc-dev \
    texinfo \
    curl ca-certificates \
    nasm \
    make \
    cmake \
    file \
    grub-common \
    grub-pc-bin \
    xorriso \
    && rm -rf /var/lib/apt/lists/*

COPY cross-toolchain.sh /tmp/cross-toolchain.sh
RUN chmod +x /tmp/cross-toolchain.sh && /tmp/cross-toolchain.sh && rm /tmp/cross-toolchain.sh

WORKDIR /work
