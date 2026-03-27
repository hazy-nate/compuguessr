
#****h* docker/Dockerfile
# NAME
#   Dockerfile
# FUNCTION
#   Prepares two Docker images:
#   * Development/build environment based on Arch Linux
#   * Release image based on scratch
#******

FROM archlinux:latest AS builder

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
    act \
    autoconf \
    automake \
    base-devel \
    bear \
    clang \
    gdb \
    git \
    mold \
    nasm \
    python \
    python-pip \
    python-pytest \
    python-sphinx

RUN git clone https://github.com/gumpu/ROBODoc.git /tmp/robodoc && \
    cd /tmp/robodoc && \
    autoreconf -i && \
    ./configure && \
    make && \
    make install && \
    rm -rf /tmp/robodoc

WORKDIR /build

COPY . .

RUN make dist-staging

RUN mkdir -p /rootfs/var/run/compuguessr /rootfs/var/run/valkey /rootfs/bin

FROM scratch

COPY --from=builder /build/dist/staging/bin/compuguessr /bin/compuguessr

COPY --from=builder /rootfs /

ENTRYPOINT ["/bin/compuguessr"]

