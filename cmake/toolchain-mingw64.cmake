# cmake/toolchain-mingw64.cmake
# CMake toolchain file for cross-compiling PowerTOP for Windows
# using the MinGW-w64 toolchain on a Linux host.
#
# Usage:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake
#
# Install MinGW-w64 on Ubuntu/Debian:
#   sudo apt install mingw-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Prefer the posix threads variant for better C++11 thread support
find_program(MINGW_GCC   x86_64-w64-mingw32-gcc-posix)
find_program(MINGW_GXX   x86_64-w64-mingw32-g++-posix)

if(NOT MINGW_GCC)
    find_program(MINGW_GCC x86_64-w64-mingw32-gcc)
endif()
if(NOT MINGW_GXX)
    find_program(MINGW_GXX x86_64-w64-mingw32-g++)
endif()

set(CMAKE_C_COMPILER   ${MINGW_GCC})
set(CMAKE_CXX_COMPILER ${MINGW_GXX})

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Windows-specific preprocessor definitions
add_compile_definitions(_WIN32_WINNT=0x0600)   # Vista and above
add_compile_definitions(WINVER=0x0600)
