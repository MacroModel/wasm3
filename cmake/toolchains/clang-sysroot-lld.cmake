# Toolchain helper for Clang builds that require SYSROOT + lld.
#
# Usage:
#   export SYSROOT=/path/to/sysroot
#   cmake -S . -B build-clang-release \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-sysroot-lld.cmake \
#     -DCMAKE_BUILD_TYPE=Release

if(NOT DEFINED ENV{SYSROOT} OR "$ENV{SYSROOT}" STREQUAL "")
  message(FATAL_ERROR "SYSROOT environment variable is not set (required for this toolchain file).")
endif()

if("$ENV{SYSROOT}" MATCHES " ")
  message(FATAL_ERROR "SYSROOT contains spaces, which is not supported: '$ENV{SYSROOT}'")
endif()

find_program(_wasm3_clang clang REQUIRED)
find_program(_wasm3_clangxx clang++ REQUIRED)
set(CMAKE_C_COMPILER "${_wasm3_clang}")
set(CMAKE_CXX_COMPILER "${_wasm3_clangxx}")

set(_wasm3_sysroot "--sysroot=$ENV{SYSROOT}")
set(_wasm3_lld "-fuse-ld=lld")

set(CMAKE_C_FLAGS_INIT "${_wasm3_sysroot}")
set(CMAKE_CXX_FLAGS_INIT "${_wasm3_sysroot}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_wasm3_sysroot} ${_wasm3_lld}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_wasm3_sysroot} ${_wasm3_lld}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_wasm3_sysroot} ${_wasm3_lld}")
