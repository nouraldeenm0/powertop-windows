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

/*
 * Windows-specific implementations of the PowerTOP platform abstraction layer.
 *
 * Uses Windows APIs for power management, CPU info, and system operations.
 * MSR access on Windows requires a kernel driver (e.g. WinRing0); stubs are
 * provided that return -1 when no driver is available.
 */

#ifdef _WIN32

#include "platform.h"
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>

using namespace std;

/* ------------------------------------------------------------------ */
/* Privilege / UID                                                      */
/* ------------------------------------------------------------------ */

int platform_get_uid(void)
{
	/* On Windows, return 0 if running as Administrator, else 1 */
	BOOL is_admin = FALSE;
	PSID admin_group = NULL;
	SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;

	if (AllocateAndInitializeSid(&nt_authority, 2,
	                             SECURITY_BUILTIN_DOMAIN_RID,
	                             DOMAIN_ALIAS_RID_ADMINS,
	                             0, 0, 0, 0, 0, 0, &admin_group)) {
		CheckTokenMembership(NULL, admin_group, &is_admin);
		FreeSid(admin_group);
	}
	return is_admin ? 0 : 1;
}

bool platform_is_privileged(void)
{
	return platform_get_uid() == 0;
}

/* ------------------------------------------------------------------ */
/* Debugfs / kernel modules (no-op on Windows)                          */
/* ------------------------------------------------------------------ */

int platform_mount_debugfs(void)
{
	/* Not applicable on Windows */
	return 0;
}

bool platform_has_debugfs(void)
{
	return false;
}

void platform_modprobe(const char *module_name)
{
	(void)module_name;
	/* Not applicable on Windows */
}

/* ------------------------------------------------------------------ */
/* File descriptor limits                                               */
/* ------------------------------------------------------------------ */

int platform_get_nr_open(void)
{
	/* Windows does not have the same fd limit concept; return a large value */
	return 65536;
}

void platform_set_nr_open(int nr)
{
	(void)nr;
	/* Not applicable on Windows */
}

/* ------------------------------------------------------------------ */
/* Data directory                                                       */
/* ------------------------------------------------------------------ */

void platform_create_data_dir(void)
{
	char path[MAX_PATH];
	const char *appdata = getenv("LOCALAPPDATA");
	if (!appdata)
		appdata = getenv("APPDATA");
	if (!appdata)
		appdata = ".";
	snprintf(path, sizeof(path), "%s\\powertop", appdata);
	_mkdir(path);
}

string platform_get_data_dir(void)
{
	char path[MAX_PATH];
	const char *appdata = getenv("LOCALAPPDATA");
	if (!appdata)
		appdata = getenv("APPDATA");
	if (!appdata)
		appdata = ".";
	snprintf(path, sizeof(path), "%s\\powertop", appdata);
	return string(path);
}

/* ------------------------------------------------------------------ */
/* Power / battery information                                          */
/* ------------------------------------------------------------------ */

double platform_get_battery_power_watts(void)
{
	SYSTEM_POWER_STATUS sps;
	if (!GetSystemPowerStatus(&sps))
		return 0.0;

	/*
	 * Windows GetSystemPowerStatus does not expose instantaneous power draw.
	 * We return 0 here; a more complete implementation would use
	 * CallNtPowerInformation(BatteryEstimatedTime, ...) or WMI
	 * (Win32_Battery.EstimatedChargeRemaining with rate tracking).
	 */
	return 0.0;
}

double platform_get_battery_charge_pct(void)
{
	SYSTEM_POWER_STATUS sps;
	if (!GetSystemPowerStatus(&sps))
		return -1.0;
	if (sps.BatteryLifePercent == 255)
		return -1.0;    /* unknown */
	return (double)sps.BatteryLifePercent;
}

int platform_get_ac_status(void)
{
	SYSTEM_POWER_STATUS sps;
	if (!GetSystemPowerStatus(&sps))
		return -1;
	/* ACLineStatus: 0=offline, 1=online, 255=unknown */
	if (sps.ACLineStatus == 1)
		return 1;
	if (sps.ACLineStatus == 0)
		return 0;
	return -1;
}

/* ------------------------------------------------------------------ */
/* CPU information                                                      */
/* ------------------------------------------------------------------ */

int platform_get_cpu_count(void)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return (int)si.dwNumberOfProcessors;
}

/* MSR access on Windows requires a privileged kernel driver.
 * These stubs return -1.  A full implementation would use WinRing0
 * (https://openlibsys.org/) or a custom driver. */

int platform_read_msr(int cpu, uint64_t offset, uint64_t *value)
{
	(void)cpu; (void)offset; (void)value;
	return -1;
}

int platform_write_msr(int cpu, uint64_t offset, uint64_t value)
{
	(void)cpu; (void)offset; (void)value;
	return -1;
}

/* ------------------------------------------------------------------ */
/* clock_gettime emulation                                              */
/* ------------------------------------------------------------------ */

int pt_clock_gettime(int clk_id, struct timespec *ts)
{
	(void)clk_id;
	LARGE_INTEGER freq, counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	ts->tv_sec  = (time_t)(counter.QuadPart / freq.QuadPart);
	ts->tv_nsec = (long)(((counter.QuadPart % freq.QuadPart) * 1000000000LL)
	                     / freq.QuadPart);
	return 0;
}

#endif /* _WIN32 */
