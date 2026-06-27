# --- STAGE 1: Builder ---
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during apt-get
ENV DEBIAN_FRONTEND=noninteractive

# Install core build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# Prepare project directory
WORKDIR /app

# Copy vcpkg manifest
COPY vcpkg.json ./

# Pre-install dependencies (this layer will be cached unless vcpkg.json changes)
RUN vcpkg install

# Copy project source
COPY . .

# Build the project
# We use the Toolchain file provided by vcpkg to resolve dependencies
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
RUN cmake --build build --config Release -j $(nproc)

# --- STAGE 2: Runner ---
FROM ubuntu:22.04 AS runner

# Install minimal runtime dependencies (SSL and Postgres client runtime)
RUN apt-get update && apt-get install -y \
    libssl3 \
    libpq5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary from the builder stage
COPY --from=builder /app/build/expense_bot .

# Security: Run as a non-root user
RUN useradd -m botuser
USER botuser

# Credentials will be passed as ENV variables via Docker Compose / CI-CD
# Do NOT COPY .env files into the image

ENTRYPOINT ["./expense_bot"]
