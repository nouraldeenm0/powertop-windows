# PowerTOP (Windows-compatible Fork)

PowerTOP is a tool used to diagnose issues with power consumption and
power management. This is a fork that adds Windows compatibility alongside
the original Linux support.

On Linux, PowerTOP provides full functionality including interactive mode,
C/P-state monitoring, device power management, and process power attribution.

On Windows, PowerTOP provides battery status, AC/DC detection, and basic
power information via Windows APIs. Linux-specific features (perf events,
tracefs, sysfs) are gracefully disabled.


# Build dependencies

PowerTOP is coded in C++11. It supports two build systems:

* **Autotools** (Linux only — original build system)
* **CMake** (Linux and Windows — cross-platform build system)

## Linux build dependencies

* kernel version >= 2.6.38
* `ncurses-devel` (required)
* `libnl-devel` (required)
* `libtracefs-devel` (required)
* `pciutils-devel` (optional — PCI device info)
* `autoconf-archive` (for autotools builds)

Example packages to install in Ubuntu:

    sudo apt install libpci-dev libnl-3-dev libnl-genl-3-dev gettext \
    libgettextpo-dev autopoint gettext libncurses5-dev libncursesw5-dev libtool-bin \
    dh-autoreconf autoconf-archive pkg-config libtracefs-dev


## Windows build dependencies

* CMake >= 3.14
* MinGW-w64 (for cross-compilation from Linux) or MSVC (native Windows)
* PDCurses (optional — for the interactive ncurses UI)

Cross-compile on Linux targeting Windows:

    sudo apt install mingw-w64 cmake

## Installing PowerTOP on Windows (automated)

The easiest way to build and install PowerTOP on a native Windows machine is
to use the provided PowerShell installer script.  It checks for all required
and optional dependencies, asks your permission before installing anything,
builds PowerTOP, and optionally puts the binary on your `PATH`.

Open an **Administrator** PowerShell window and run:

    Set-ExecutionPolicy Bypass -Scope Process -Force
    .\scripts\install.ps1

The script will:
1. Detect missing dependencies (CMake, a C++ compiler, Git, optional PDCurses).
2. Offer to install each missing dependency via **winget** (preferred) or
   **Chocolatey** (fallback) — always asking first.
3. Configure and build PowerTOP with CMake.
4. Optionally copy `powertop.exe` to a directory of your choice and add it to
   the system `PATH`.

> **Note:** PowerTOP itself must always be run as Administrator.

## Building PowerTOP (Linux — autotools)

The `autogen.sh` script needs to be run only once to generate `configure`.

    ./autogen.sh
    ./configure
    make

## Building PowerTOP (Linux — CMake)

    mkdir build && cd build
    cmake ..
    make

## Building PowerTOP (Windows — MinGW-w64 cross-compilation)

Cross-compile from Linux for Windows:

    mkdir build-win && cd build-win
    cmake .. \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw64.cmake \
        -DCMAKE_BUILD_TYPE=Release
    make

Or natively on Windows with MSVC/CMake:

    mkdir build && cd build
    cmake .. -G "Visual Studio 17 2022"
    cmake --build . --config Release


# Running PowerTOP

The following sections go over basic operation of PowerTOP. This includes
kernel configuration options (or kernel patches) needed for full functionality.
Run `powertop --help` to see all options.

PowerTOP must be run with administrator/root privileges.

## Windows notes

On Windows, PowerTOP uses the `GetSystemPowerStatus` API for battery and AC
status information. MSR (Model-Specific Register) access requires a kernel
driver such as [WinRing0](https://openlibsys.org/); without it, C-state
data will not be available.

The interactive ncurses UI requires [PDCurses](https://pdcurses.org/) to be
installed. Without it, PowerTOP runs in report-only mode.

Windows-specific limitations:
- No perf-events or tracefs support (Linux kernel feature)
- No sysfs/procfs device enumeration  
- MSR reads require a privileged kernel driver
- Extech power meter (`--extech`) not supported


## Kernel parameters (Linux only)

PowerTOP needs some kernel config options enabled to function
properly. As of linux-3.3.0, these are (the list probably is incomplete):

    CONFIG_NO_HZ
    CONFIG_HIGH_RES_TIMERS
    CONFIG_HPET_TIMER
    CONFIG_CPU_FREQ_GOV_ONDEMAND
    CONFIG_USB_SUSPEND
    CONFIG_SND_AC97_POWER_SAVE
    CONFIG_TIMER_STATS
    CONFIG_PERF_EVENTS
    CONFIG_PERF_COUNTERS
    CONFIG_TRACEPOINTS
    CONFIG_TRACING
    CONFIG_EVENT_POWER_TRACING_DEPRECATED
    CONFIG_X86_MSR
    ACPI_PROCFS_POWER
    CONFIG_DEBUG_FS

Use these configs from linux-3.13.rc1:

    CONFIG_POWERCAP
    CONFIG_INTEL_RAPL

The patches in the `patches/` sub-directory are optional but enable *full*
PowerTOP functionality.


## Outputting a report

When PowerTOP is executed as root and without arguments, it runs in
interactive mode. In this mode, PowerTOP most resembles `top`.

For generating reports, or for filing functional bug reports, there are two
output modes: CSV and HTML. You can set sample times, the number of iterations,
a workload over which to run PowerTOP, and whether to include
`debug`-level output.

For an HTML report, execute PowerTOP with this option:

    powertop --html=report.html

This creates a static `report.html` file, suitable for sharing.

For a CSV report, execute PowerTOP with this option:

    powertop --csv=report.csv

This creates a static `powertop.csv` file, also suitable for sharing.

If you wish to file a functional bug report, generate and share a
`debug`-mode HTML report and share it, using the following command:

    powertop --debug --html=report.html

**Important Note:** As PowerTOP is intended for privileged (`root`/Administrator)
use, your reports-- especially when run with `--debug`-- will contain verbose
system information. PowerTOP **does not** sanitize, scrub, or otherwise
anonymize its reports. Be mindful of this when sharing reports.


## Calibrating and power numbers

PowerTOP, when running on battery, tracks power consumption and activity on
the system. Once there are sufficient measurements, PowerTOP can start to
report power estimates for various activities. You can help increase the
accuracy of the estimation by running a calibration cycle at least once:

    powertop --calibrate


## Extech Power Analyzer / Datalogger support (Linux only)

Our analysis teams use the Extech* Power Analyzer/Datalogger (model
number 380803). PowerTOP supports this device over the serial cable by passing
the device node on the command line using this command:

    powertop --extech=/dev/ttyUSB0

(where ttyUSB0 is the devicenode of the serial-to-usb adapter on our system)


# Contributing to PowerTOP and getting support

There are numerous ways you and your friends can contribute to PowerTOP. See
the `CONTRIBUTE.md` document for more details. Elevator summary: "fork, and
send PRs!".

We have a mailing list: `powertop@lists.01.org`:
* Subscribe at:
  * https://lists.01.org/postorius/lists/powertop.lists.01.org/
* Browse archives at:
  * https://lists.01.org/hyperkitty/list/powertop@lists.01.org/

If you find bugs, you can file an issue-- see `CONTRIBUTE.md` for further
details:
* File bugs/wishes at:
  * https://github.com/fenrus75/powertop/issues


# Code from other open source projects

PowerTOP contains some code from other open source projects; we'd like to thank
the authors of those projects for their work. Specifically, PowerTOP contains
code from

```
Parse Event Library - Copyright 2009, 2010 Red Hat Inc  Steven Rostedt <srostedt@redhat.com>
nl80211 userspace tool - Copyright 2007, 2008	Johannes Berg <johannes@sipsolutions.net>
```


# Copyright and License

    PowerTOP
    Copyright (C) 2020  Intel Corporation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

See `COPYING` file for a copy of the aforementioned (GPLv2) license.


## SPDX Tag

    /* SPDX-License-Identifier: GPL-2.0-only */

From: https://spdx.org/licenses/GPL-2.0-only.html
