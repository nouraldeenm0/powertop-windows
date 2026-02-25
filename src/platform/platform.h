/*
 * Copyright 2010, Intel Corporation
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 */
#ifndef INCLUDE_GUARD_PLATFORM_H
#define INCLUDE_GUARD_PLATFORM_H

/*
 * Cross-platform abstraction layer for PowerTOP Windows port.
 * Abstracts OS-specific APIs for file system, process, power, and system info.
 */

#include <stdint.h>
#include <string>

#ifdef _WIN32
#  include <windows.h>
#  include <direct.h>      /* _mkdir */
#  include <io.h>          /* _access */
#  include <process.h>     /* getpid */

/* Map POSIX names to Windows equivalents */
#  define sleep(s)         Sleep((s) * 1000)
#  define usleep(us)       Sleep((us) / 1000)
#  define access(p,m)      _access((p),(m))
#  define mkdir(p,m)       _mkdir(p)
#  define PATH_MAX         MAX_PATH
#  define R_OK             4
#  define W_OK             2
#  define X_OK             1
#  define F_OK             0

/* Suppress POSIX deprecation warnings on MSVC */
#  ifdef _MSC_VER
#    pragma warning(disable: 4996)
#  endif

#else /* Linux / POSIX */
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <limits.h>
#endif /* _WIN32 */

/* On Windows without PDCurses, use the stub ncurses implementation */
#if defined(_WIN32) && !defined(HAVE_PDCURSES)
#  include "windows/ncurses_stub.h"
#endif

/* ------------------------------------------------------------------ */
/* Thread abstraction                                                   */
/* ------------------------------------------------------------------ */
#ifndef _WIN32
#  include <pthread.h>
typedef pthread_t pt_thread_t;
static inline int pt_thread_create(pt_thread_t *t, void *(*fn)(void *), void *arg) {
    return pthread_create(t, NULL, fn, arg);
}
static inline int pt_thread_join(pt_thread_t t) {
    return pthread_join(t, NULL);
}
#else
#  include <process.h>
typedef HANDLE pt_thread_t;
static inline int pt_thread_create(pt_thread_t *t, void *(*fn)(void *), void *arg) {
    *t = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall *)(void *))fn, arg, 0, NULL);
    return (*t == NULL) ? 1 : 0;
}
static inline int pt_thread_join(pt_thread_t t) {
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
    return 0;
}
#endif /* _WIN32 */

/* ------------------------------------------------------------------ */
/* getopt (Windows does not provide getopt_long)                        */
/* ------------------------------------------------------------------ */
#ifdef _WIN32
#  include "windows/getopt_win.h"
#else
#  include <getopt.h>
#endif

/* ------------------------------------------------------------------ */
/* Platform-specific power/system helpers                               */
/* ------------------------------------------------------------------ */

/* Returns the effective user ID (0 = root on Linux, administrator on Win) */
extern int platform_get_uid(void);

/* Returns true if the process has sufficient privileges to run PowerTOP */
extern bool platform_is_privileged(void);

/* Mount debugfs if not already mounted (no-op on Windows) */
extern int platform_mount_debugfs(void);

/* Check if debugfs is available */
extern bool platform_has_debugfs(void);

/* Load a kernel module (no-op on Windows) */
extern void platform_modprobe(const char *module_name);

/* Get the maximum number of open file descriptors */
extern int platform_get_nr_open(void);

/* Set the maximum number of open file descriptors */
extern void platform_set_nr_open(int nr);

/* Create the powertop data directory */
extern void platform_create_data_dir(void);

/* Get the path to the powertop data directory */
extern std::string platform_get_data_dir(void);

/* ------------------------------------------------------------------ */
/* Power measurement                                                    */
/* ------------------------------------------------------------------ */

/* Returns current battery power draw in watts (negative = discharging) */
extern double platform_get_battery_power_watts(void);

/* Returns battery charge percentage [0..100], or -1 if unknown */
extern double platform_get_battery_charge_pct(void);

/* Returns AC adapter status: 1=on AC, 0=on battery, -1=unknown */
extern int platform_get_ac_status(void);

/* ------------------------------------------------------------------ */
/* CPU information                                                      */
/* ------------------------------------------------------------------ */

/* Returns the number of logical CPUs */
extern int platform_get_cpu_count(void);

/* Read a CPU model-specific register (MSR).
 * Returns 0 on success, -1 on failure. */
extern int platform_read_msr(int cpu, uint64_t offset, uint64_t *value);

/* Write a CPU model-specific register (MSR).
 * Returns 0 on success, -1 on failure. */
extern int platform_write_msr(int cpu, uint64_t offset, uint64_t value);

/* ------------------------------------------------------------------ */
/* Time helpers                                                         */
/* ------------------------------------------------------------------ */
#ifdef _WIN32
#  include <time.h>
/* Provide clock_gettime(CLOCK_MONOTONIC, ...) on Windows */
#  ifndef CLOCK_MONOTONIC
#    define CLOCK_MONOTONIC 1
#  endif
extern int pt_clock_gettime(int clk_id, struct timespec *ts);
#  define clock_gettime pt_clock_gettime
#endif /* _WIN32 */

#endif /* INCLUDE_GUARD_PLATFORM_H */
