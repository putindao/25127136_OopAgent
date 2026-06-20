# =============================================================================
#  OOP AI Agent Framework — build configuration
#  Toolchain: MSYS2 UCRT64 (g++ 16.1.0), Ninja generator.
#  Configure: cmake -S . -B build -G Ninja
#  Build:     cmake --build build
# =============================================================================
cmake_minimum_required(VERSION 3.25)
project(oop_ai_agent LANGUAGES CXX)

# We target C++26 (GCC 16 supports it); the brief requires features from
# C++17/20/23/26. STANDARD_REQUIRED forbids silent fallback to an older standard.
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
add_compile_options(-Wall -Wextra -Wpedantic)

# Third-party libraries (installed via MSYS2 pacman).
find_package(CURL REQUIRED)            # libcurl  -> OllamaClient HTTP
find_package(nlohmann_json REQUIRED)   # JSON     -> request/response parsing
find_package(SQLite3 REQUIRED)         # SQLite   -> memory tool

# All agent sources except the application entry point form the core target.
# (For P0 only src/main.cpp exists; later phases drop more .cpp files in and
#  CONFIGURE_DEPENDS re-globs automatically.)
file(GLOB_RECURSE AGENT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_executable(agent ${AGENT_SOURCES})
target_include_directories(agent PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(agent PRIVATE
  CURL::libcurl
  nlohmann_json::nlohmann_json
  SQLite3::SQLite3
  stdc++exp           # GCC/MinGW: required by std::print/std::println (C++23)
)
