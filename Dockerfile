# --- STAGE 1: Compiler ---
FROM expense-bot-builder:latest AS compiler

WORKDIR /app
COPY vcpkg.json .
COPY . .

# Build the application
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
RUN cmake --build build --config Release -j $(nproc)

# --- STAGE 2: Runner ---
FROM ubuntu:24.04 AS runner

# Install runtime dependencies (SSL and Postgres client)
RUN apt-get update && apt-get install -y \
    libssl3t64 libpq5 ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 1. Copy the executable
COPY --from=compiler /app/build/expense-bot .

# 2. Copy the shared libraries from vcpkg (the .so files)
# This is what was missing!
COPY --from=compiler /app/vcpkg_installed/x64-linux/lib/*.so* /usr/local/lib/

# 3. Refresh the linker cache so the system finds the new libraries
RUN ldconfig

# 4. Create non-root user
RUN useradd -m botuser && chown -R botuser:botuser /app
USER botuser

# 5. Corrected Entrypoint
ENTRYPOINT ["./expense-bot"]
