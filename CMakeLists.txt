cmake_minimum_required(VERSION 3.22)

project(BomAnalyzer
    VERSION 1.0.0
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BOM_BUILD_APP   "Build the core library and the BOM viewer application" ON)
option(BOM_BUILD_TESTS "Build unit tests" ON)
option(BOM_BUILD_DELPHI "Build the Delphi wrapper demo" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(BOM_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    include(BomTesting)
endif()

# ---------------------------------------------------------------------------
# Components
# ---------------------------------------------------------------------------
add_subdirectory(components/profile_analyzer)

if(WIN32 AND BOM_BUILD_DELPHI)
    add_subdirectory(components/delphi)
endif()

if(BOM_BUILD_APP)
    add_subdirectory(components/core)
    add_subdirectory(components/app)
endif()

if(BOM_BUILD_TESTS)
    add_subdirectory(tests)
endif()
