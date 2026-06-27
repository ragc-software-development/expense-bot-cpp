cmake_minimum_required(VERSION 3.20)
project(expense-bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies via vcpkg toolchain
find_package(dpp REQUIRED)
find_package(libpqxx REQUIRED)

# Include directories
include_directories(include)
include_directories(include/models)

# Define source files
set(SOURCES
    src/main.cpp
    src/connection_pool.cpp
    src/worker_pool.cpp
    src/db_processor.cpp
    src/gemini_client.cpp
    src/processor.cpp
    src/replier.cpp
)

# Define your bot executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE dpp::dpp libpqxx::pqxx)

# Copy executable to a predictable location for simplified Docker COPY
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
