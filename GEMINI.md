# ExpenseBot — Architectural Specification

This living document contains the system specifications, structural constraints, and architectural patterns of **ExpenseBot**, a high-performance Discord expense tracking bot implemented in modern C++.

## ⚙️ Core Tooling & Ecosystem
* **Language Standard:** C++17 / C++20
* **Package Management:** vcpkg (Manifest Mode via `vcpkg.json`)
* **Build Automation:** CMake (Configured via `CMakePresets.json`)
* **Primary Dependencies:**
  * `dpp` (Discord++ WebSocket Gateway & HTTP client)
  * `libpqxx` (PostgreSQL C++ client bindings)
  * `PostgreSQL 15+` (Target Database Engine)
  * `OpenSSL` (System-level crypto library required for Linux/macOS linking stages)

## 🏗️ Pipeline Architecture Layout
To maximize throughput and prevent Gateway blocking constraints, tasks are strictly decoupled into independent operational states:

### 1. Receive Task (`src/receiver.hpp` / `src/receiver.cpp`)
* Listens non-blockingly to incoming Discord chat messages and slash commands.
* Extracts the payload wrapper and offloads it immediately to the processing worker pool.

### 2. Process Task (`src/processor.hpp` / `src/processor.cpp`)
* Utilizes asynchronous threads to handle multi-second blocking operations:
  * Employs Gemini LLM API (`gemini-2.5-flash-lite`) requests to extract semantic details from informal sentences.
  * Writes normalized structural schemas into a persistent PostgreSQL repository.

### 3. Reply Task (`src/replier.hpp` / `src/replier.cpp`)
* Generates visually structured Discord Rich Embed cards showing transaction breakdowns, currency mappings, and budgeting limits.

## 🗄️ Database Schema & Object Mappings

### C++ Object Mapping
```cpp
struct Expense {
    double amount;
    std::string currency;
    std::string category;
    std::string description;
    int64_t user_id;
};