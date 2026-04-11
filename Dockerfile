# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* docker/Dockerfile
# NAME
#   Dockerfile
#******

#===============================================================================
# NATIVE BUILDER
#===============================================================================

FROM archlinux:latest AS native-builder

LABEL project="compuguessr"

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
    base-devel \
    bear \
    clang \
    git \
    mold \
    nasm \
    postgresql-libs

WORKDIR /app

#===============================================================================
# ROBODOC BUILDER
#===============================================================================

FROM native-builder AS robodoc-builder

RUN pacman -S --noconfirm autoconf automake

COPY external/robodoc /tmp/robodoc

RUN cd /tmp/robodoc && \
    autoreconf -i && \
    ./configure && \
    make

#===============================================================================
# EMSCRIPTEN BUILDER
#===============================================================================

FROM native-builder AS emcc-builder

RUN pacman -S --noconfirm emscripten

ENV EMSCRIPTEN="/usr/lib/emscripten" \
    PATH="/usr/lib/emscripten:${PATH}" \
    EM_CACHE="/tmp/em_cache"

RUN mkdir -p /tmp/em_cache && chmod 777 /tmp/em_cache

#===============================================================================
# DOCUMENTATION AND TESTING BUILDER
#===============================================================================

FROM robodoc-builder AS docs-builder

RUN pacman -S --noconfirm python python-pip python-pytest python-sphinx

COPY --from=robodoc-builder /tmp/robodoc/Source/robodoc /usr/local/bin/robodoc

#===============================================================================
# FULL BUILDER
#===============================================================================

FROM emcc-builder AS full-builder

RUN pacman -S --noconfirm python python-pip python-pytest python-sphinx gdb

COPY --from=robodoc-builder /tmp/robodoc/Source/robodoc /usr/local/bin/robodoc

#===============================================================================
# RELEASE BUILDER
#===============================================================================

FROM full-builder AS release-builder

COPY . .

RUN make dist-staging

#===============================================================================
# RELEASE IMAGE
#===============================================================================

FROM scratch AS release

COPY --from=release-builder /app/dist/staging/bin/compuguessr /bin/compuguessr

ENTRYPOINT ["/bin/compuguessr"]
