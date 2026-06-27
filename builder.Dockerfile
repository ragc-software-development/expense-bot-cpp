FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# 1. Install modern build tools (Ubuntu 24.04 brings GCC 13+)
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip tar pkg-config libssl-dev python3 bison flex \
    && rm -rf /var/lib/apt/lists/*

# 2. Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

# 3. Pre-compile dependencies
# Now that we have GCC 13, libpqxx will find <format> and compile perfectly.
WORKDIR /app
COPY vcpkg.json .
RUN VCPKG_MAX_CONCURRENCY=8 vcpkg install

# Now this image has everything ready to compile ExpenseBot
