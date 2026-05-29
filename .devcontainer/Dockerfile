FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04 AS base

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    gpg \
    wget \
    software-properties-common \
    && wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && rm -rf /var/lib/apt/lists/*

FROM base AS devcontainer

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    gcc-16 \
    g++-16 \
    gdb \
    ninja-build \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-16 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-16 100 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++ 100 \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y --no-install-recommends \
    python-is-python3 \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    && rm -rf /var/lib/apt/lists/*
    
ENV PIP_DISABLE_PIP_VERSION_CHECK=1

WORKDIR /workspaces/CellUI
