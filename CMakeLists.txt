cmake_minimum_required(VERSION 3.14)
project(PowerTOP VERSION 2.15 LANGUAGES CXX C)

# -----------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------
option(ENABLE_NLS "Enable Native Language Support (gettext)" OFF)
option(WITH_PCI   "Build with PCI device support (libpci)"   ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# -----------------------------------------------------------------------
# Platform detection
# -----------------------------------------------------------------------
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Building for Windows")
else()
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Building for Linux")
endif()

# -----------------------------------------------------------------------
# Find required packages (Linux only)
# -----------------------------------------------------------------------
if(PLATFORM_LINUX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(NCURSESW REQUIRED ncursesw)

    # Detect libnl version (prefer 3.x, fall back to 2.x)
    pkg_check_modules(LIBNL libnl-3.0 libnl-genl-3.0)
    if(LIBNL_FOUND)
        set(HAVE_LIBNL20 TRUE)
        pkg_check_modules(LIBNL_GENL REQUIRED libnl-genl-3.0)
    else()
        pkg_check_modules(LIBNL REQUIRED libnl-2.0)
        set(HAVE_LIBNL20 TRUE)
    endif()

    pkg_check_modules(TRACEFS  REQUIRED libtracefs)

    if(WITH_PCI)
        pkg_check_modules(LIBPCI libpci)
        if(LIBPCI_FOUND)
            add_definitions(-DHAVE_PCI)
        else()
            add_definitions(-DHAVE_NO_PCI)
            message(STATUS "libpci not found - PCI support disabled")
        endif()
    else()
        add_definitions(-DHAVE_NO_PCI)
    endif()

    find_package(Threads REQUIRED)
endif()

# -----------------------------------------------------------------------
# config.h substitute (CMake version)
# -----------------------------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# -----------------------------------------------------------------------
# Common source files (platform-independent)
# -----------------------------------------------------------------------
set(POWERTOP_COMMON_SOURCES
    src/devlist.cpp
    src/display.cpp
    src/lib.cpp
    src/main.cpp

    src/calibrate/calibrate.cpp

    src/cpu/abstract_cpu.cpp
    src/cpu/cpu.cpp
    src/cpu/cpu_core.cpp
    src/cpu/cpu_package.cpp
    src/cpu/cpudevice.cpp
    src/cpu/intel_cpus.cpp
    src/cpu/intel_gpu.cpp
    src/cpu/cpu_rapl_device.cpp
    src/cpu/dram_rapl_device.cpp
    src/cpu/rapl/rapl_interface.cpp

    src/devices/device.cpp
    src/devices/ahci.cpp
    src/devices/alsa.cpp
    src/devices/backlight.cpp
    src/devices/devfreq.cpp
    src/devices/network.cpp
    src/devices/rfkill.cpp
    src/devices/runtime_pm.cpp
    src/devices/thinkpad-fan.cpp
    src/devices/thinkpad-light.cpp
    src/devices/usb.cpp
    src/devices/i915-gpu.cpp
    src/devices/gpu_rapl_device.cpp

    src/measurement/acpi.cpp
    src/measurement/extech.cpp
    src/measurement/measurement.cpp
    src/measurement/sysfs.cpp
    src/measurement/opal-sensors.cpp

    src/parameters/learn.cpp
    src/parameters/parameters.cpp
    src/parameters/persistent.cpp

    src/perf/perf.cpp
    src/perf/perf_bundle.cpp

    src/process/do_process.cpp
    src/process/interrupt.cpp
    src/process/powerconsumer.cpp
    src/process/process.cpp
    src/process/processdevice.cpp
    src/process/timer.cpp
    src/process/work.cpp

    src/report/report.cpp
    src/report/report-maker.cpp
    src/report/report-data-html.cpp
    src/report/report-formatter-base.cpp
    src/report/report-formatter-csv.cpp
    src/report/report-formatter-html.cpp

    src/tuning/bluetooth.cpp
    src/tuning/ethernet.cpp
    src/tuning/iw.c
    src/tuning/runtime.cpp
    src/tuning/tunable.cpp
    src/tuning/tuning.cpp
    src/tuning/tuningi2c.cpp
    src/tuning/tuningsysfs.cpp
    src/tuning/tuningusb.cpp
    src/tuning/wifi.cpp

    src/wakeup/waketab.cpp
    src/wakeup/wakeup.cpp
    src/wakeup/wakeup_ethernet.cpp
    src/wakeup/wakeup_usb.cpp
)

# -----------------------------------------------------------------------
# Platform-specific source files
# -----------------------------------------------------------------------
if(PLATFORM_LINUX)
    list(APPEND POWERTOP_COMMON_SOURCES
        src/cpu/cpu_linux.cpp
        src/platform/platform_linux.cpp
    )
else()
    list(APPEND POWERTOP_COMMON_SOURCES
        src/platform/windows/platform_windows.cpp
        src/platform/windows/dir_win.cpp
    )
endif()

# -----------------------------------------------------------------------
# Executable target
# -----------------------------------------------------------------------
add_executable(powertop ${POWERTOP_COMMON_SOURCES})

target_include_directories(powertop PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# -----------------------------------------------------------------------
# Platform-specific link libraries
# -----------------------------------------------------------------------
if(PLATFORM_LINUX)
    target_include_directories(powertop PRIVATE
        ${NCURSESW_INCLUDE_DIRS}
        ${LIBNL_INCLUDE_DIRS}
        ${LIBNL_GENL_INCLUDE_DIRS}
        ${TRACEFS_INCLUDE_DIRS}
    )
    target_link_libraries(powertop
        ${NCURSESW_LIBRARIES}
        ${LIBNL_LIBRARIES}
        ${LIBNL_GENL_LIBRARIES}
        ${TRACEFS_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
    )
    if(LIBPCI_FOUND)
        target_include_directories(powertop PRIVATE ${LIBPCI_INCLUDE_DIRS})
        target_link_libraries(powertop ${LIBPCI_LIBRARIES})
    endif()
else()
    # Windows: link against PDCurses if available, otherwise define stub
    find_library(PDCURSES_LIB pdcurses)
    if(PDCURSES_LIB)
        target_link_libraries(powertop ${PDCURSES_LIB})
        target_compile_definitions(powertop PRIVATE HAVE_PDCURSES)
    else()
        message(STATUS "PDCurses not found - ncurses UI will be disabled on Windows")
        target_compile_definitions(powertop PRIVATE NO_CURSES)
    endif()
endif()

# -----------------------------------------------------------------------
# Compile definitions
# -----------------------------------------------------------------------
if(ENABLE_NLS)
    target_compile_definitions(powertop PRIVATE ENABLE_NLS)
endif()

target_compile_definitions(powertop PRIVATE
    PACKAGE_VERSION="${PROJECT_VERSION}"
    PACKAGE="${PROJECT_NAME}"
    HAVE_CONFIG_H=1
)

if(PLATFORM_LINUX)
    target_compile_definitions(powertop PRIVATE
        LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale"
    )
endif()

# -----------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------
if(MSVC)
    target_compile_options(powertop PRIVATE /W3 /wd4996)
else()
    target_compile_options(powertop PRIVATE -Wall -Wextra -Wno-unused-parameter -fpermissive)
    if(NOT WIN32)
        target_compile_options(powertop PRIVATE -D_FORTIFY_SOURCE=2)
    endif()
endif()

# -----------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------
install(TARGETS powertop DESTINATION bin)

# -----------------------------------------------------------------------
# Summary
# -----------------------------------------------------------------------
message(STATUS "")
message(STATUS "PowerTOP ${PROJECT_VERSION} build configuration:")
message(STATUS "  Platform  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  NLS       : ${ENABLE_NLS}")
message(STATUS "  PCI       : ${WITH_PCI}")
message(STATUS "")
