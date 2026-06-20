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

# Core library = every src/*.cpp except the application entry point. The agent
# CLI, the tests, and the benchmark runner all link against this one library.
# CONFIGURE_DEPENDS re-globs automatically when files are added.
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

add_library(agent_core STATIC ${CORE_SOURCES})
target_include_directories(agent_core PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(agent_core PUBLIC
  CURL::libcurl
  nlohmann_json::nlohmann_json
  SQLite3::SQLite3
  stdc++exp           # GCC/MinGW: required by std::print/std::println (C++23)
)

# The agent CLI.
add_executable(agent "${CMAKE_SOURCE_DIR}/src/main.cpp")
target_link_libraries(agent PRIVATE agent_core)

# Unit tests: one executable per tests/*.cpp, registered with CTest.
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
foreach(test_src ${TEST_SOURCES})
  get_filename_component(test_name "${test_src}" NAME_WE)
  add_executable(${test_name} "${test_src}")
  target_link_libraries(${test_name} PRIVATE agent_core)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
