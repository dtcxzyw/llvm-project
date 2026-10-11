FROM ubuntu:latest

RUN apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    linux-tools-common \
    linux-tools-generic \
    cmake \
    sudo \
    ccache \
    libkrb5-3 \
    zlib1g-dev \
    libssl-dev \
    libicu-dev \
    gawk \
    bison \
    wget \
    flex \
    curl \
    jq \
    git \
    ca-certificates \
    python3 \
    python3-venv \
    software-properties-common

RUN useradd -u 1001 -m llvm
USER llvm
WORKDIR /home/llvm

RUN curl -LsSf https://astral.sh/uv/install.sh | sh
