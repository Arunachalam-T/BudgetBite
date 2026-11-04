cmake_minimum_required(VERSION 3.20)

# Allow downloads on environments with unconfigured CA bundles
set(CMAKE_TLS_VERIFY 0 CACHE BOOL "" FORCE)

# CMake 4.0+ compatibility for older sub-dependencies (e.g. miniz in OpenXLSX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

project(SmartMealOptimizer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Output directory ───────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

# ─── Dependencies via FetchContent ─────────────────────────────────────────
include(FetchContent)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# ─── Asio (required by Crow) ────────────────────────────────────────────────
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()

# ─── Crow (header-only HTTP framework) ──────────────────────────────────────
FetchContent_Declare(
    crow_fetch
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(crow_fetch)
if(NOT crow_fetch_POPULATED)
    FetchContent_Populate(crow_fetch)
endif()

# Define header-only Crow interface library
add_library(Crow INTERFACE)
target_include_directories(Crow INTERFACE
    ${crow_fetch_SOURCE_DIR}/include
    ${asio_SOURCE_DIR}/asio/include
)
if(WIN32)
    target_compile_definitions(Crow INTERFACE ASIO_STANDALONE _WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
else()
    target_compile_definitions(Crow INTERFACE ASIO_STANDALONE)
endif()

# ─── OpenXLSX (Excel reading/writing) ───────────────────────────────────────
set(OPENXLSX_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OPENXLSX_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    OpenXLSX
    GIT_REPOSITORY https://github.com/troldal/OpenXLSX.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(OpenXLSX)

# ─── Backend library ─────────────────────────────────────────────────────────
set(BACKEND_SOURCES
    src/validator.cpp
    src/recipe_calculator.cpp
    src/plan_evaluator.cpp
    src/csv_reader.cpp
    src/excel_reader.cpp
    src/data_loader.cpp
    src/optimization_config.cpp
    src/optimization_model.cpp
    src/brute_force.cpp
    src/greedy.cpp
    src/dynamic_programming.cpp
    src/backtracking.cpp
    src/branch_and_bound.cpp
    src/benchmark.cpp
    src/excel_exporter.cpp
    src/api.cpp
)

add_library(smartmeal_lib STATIC ${BACKEND_SOURCES})

target_include_directories(smartmeal_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(smartmeal_lib PUBLIC
    nlohmann_json::nlohmann_json
    Crow
    OpenXLSX::OpenXLSX
)

target_include_directories(smartmeal_lib PUBLIC
    ${asio_SOURCE_DIR}/asio/include
)

# ─── Main executable ─────────────────────────────────────────────────────────
add_executable(smartmeal_server src/main.cpp)
target_link_libraries(smartmeal_server PRIVATE smartmeal_lib)

# Windows: link ws2_32 for Asio sockets
if(WIN32)
    target_link_libraries(smartmeal_server PRIVATE ws2_32 mswsock)
    target_link_libraries(smartmeal_lib PUBLIC ws2_32 mswsock)
endif()

# ─── Tests ───────────────────────────────────────────────────────────────────
enable_testing()

set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_parser.cpp
    tests/test_validator.cpp
    tests/test_calculator.cpp
    tests/test_evaluator.cpp
    tests/test_algorithms.cpp
    tests/test_exporter.cpp
)

add_executable(smartmeal_tests ${TEST_SOURCES})
target_link_libraries(smartmeal_tests PRIVATE smartmeal_lib)

add_test(NAME SmartMealTests COMMAND smartmeal_tests)

# ─── Compiler warnings ────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(smartmeal_lib PRIVATE /W3)
else()
    target_compile_options(smartmeal_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "SmartMeal Optimizer backend configured.")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:   ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

