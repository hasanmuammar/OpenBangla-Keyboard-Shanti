cmake_minimum_required(VERSION 3.18...3.29)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" VERSION_STRING LIMIT_COUNT 1 REGEX "^.+")
project("OpenBangla-Keyboard" VERSION ${VERSION_STRING})
set(APP_NAME "OpenBangla-Keyboard")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This fork currently supports Linux only.")
endif()

set(OS_LINUX 1)

add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(cmake/corrosion-rs)
find_package(Qt5 COMPONENTS Widgets Network Svg DBus REQUIRED)

# Rust crates shared by the engine plugins (shanti-config, shanti-paths);
# imported as CMake targets so the engines can link them directly.
add_subdirectory(crates)

message(STATUS "Qt5 framework version: ${Qt5Core_VERSION}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed.\nPlease create a build directory and configure CMake from outside the source tree.")
endif()

if(NOT DEFINED BIN_DIR)
    set(BIN_DIR "${CMAKE_INSTALL_PREFIX}/bin")
endif()
if(NOT DEFINED DATADIR)
    set(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
endif()
if(NOT DEFINED LIBEXECDIR)
    set(LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/libexec")
endif()

set(PROJECT_DATADIR "${DATADIR}/openbangla-keyboard")
add_definitions(-DPROJECT_DATADIR="${PROJECT_DATADIR}")
add_definitions(-DBIN_DIR="${BIN_DIR}")
add_definitions(-DLIBEXECDIR="${LIBEXECDIR}")

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)

option(ENABLE_IBUS "Enable IBus support" OFF)
option(ENABLE_FCITX "Enable Fcitx5 support" OFF)

if(ENABLE_IBUS AND ENABLE_FCITX)
    message(FATAL_ERROR "Enable only one input-method backend.")
endif()

if(NOT ENABLE_IBUS AND NOT ENABLE_FCITX)
    message(FATAL_ERROR "Enable either ENABLE_IBUS or ENABLE_FCITX.")
endif()

pkg_check_modules(ZSTD REQUIRED libzstd)

if(ENABLE_IBUS)
    pkg_check_modules(IBUS REQUIRED ibus-1.0)
endif()

if(ENABLE_FCITX)
    find_package(Fcitx5Core 5.0.5 REQUIRED)
endif()

add_subdirectory(src/engine)
add_subdirectory(src/platform)
add_subdirectory(src/frontend)
add_subdirectory(src/shared)
