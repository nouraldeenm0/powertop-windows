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
 * Linux-specific implementations of the PowerTOP platform abstraction layer.
 */

#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <limits.h>

using namespace std;

#define DEBUGFS_MAGIC  0x64626720
#define NR_OPEN_DEF    (1024 * 1024)

int platform_get_uid(void)
{
	return (int)geteuid();
}

bool platform_is_privileged(void)
{
	return geteuid() == 0;
}

int platform_mount_debugfs(void)
{
	struct statfs st_fs;
	int ret = 0;

	statfs("/sys/kernel/debug", &st_fs);
	if (st_fs.f_type != (long)DEBUGFS_MAGIC) {
		if (access("/bin/mount", X_OK) == 0)
			ret = system("/bin/mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
		else
			ret = system("mount -t debugfs debugfs /sys/kernel/debug > /dev/null 2>&1");
	}
	return ret;
}

bool platform_has_debugfs(void)
{
	struct statfs st_fs;
	statfs("/sys/kernel/debug", &st_fs);
	return st_fs.f_type == (long)DEBUGFS_MAGIC;
}

void platform_modprobe(const char *module_name)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "/sbin/modprobe %s > /dev/null 2>&1", module_name);
	if (system(cmd))
		fprintf(stderr, "modprobe %s failed\n", module_name);
}

int platform_get_nr_open(void)
{
	int nr_open = NR_OPEN_DEF;
	ifstream file;

	file.open("/proc/sys/fs/nr_open", ios::in);
	if (file) {
		file >> nr_open;
		file.close();
	}
	return nr_open;
}

void platform_set_nr_open(int nr)
{
	struct rlimit rlmt;
	rlmt.rlim_cur = rlmt.rlim_max = (rlim_t)nr;
	setrlimit(RLIMIT_NOFILE, &rlmt);
}

void platform_create_data_dir(void)
{
	if (access("/var/cache/", W_OK) == 0)
		mkdir("/var/cache/powertop", 0600);
	else
		mkdir("/data/local/powertop", 0600);
}

string platform_get_data_dir(void)
{
	if (access("/var/cache/powertop", W_OK) == 0)
		return "/var/cache/powertop";
	return "/data/local/powertop";
}

double platform_get_battery_power_watts(void)
{
	/* Try ACPI sysfs interface */
	FILE *f;
	double power = 0.0;

	f = fopen("/sys/class/power_supply/BAT0/power_now", "r");
	if (!f)
		f = fopen("/sys/class/power_supply/BAT1/power_now", "r");
	if (f) {
		unsigned long uw = 0;
		if (fscanf(f, "%lu", &uw) == 1)
			power = uw / 1000000.0;
		fclose(f);
	}
	return power;
}

double platform_get_battery_charge_pct(void)
{
	FILE *f;
	int cap = -1;

	f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f)
		f = fopen("/sys/class/power_supply/BAT1/capacity", "r");
	if (f) {
		fscanf(f, "%d", &cap);
		fclose(f);
	}
	return (double)cap;
}

int platform_get_ac_status(void)
{
	FILE *f;
	int online = -1;

	f = fopen("/sys/class/power_supply/AC/online", "r");
	if (!f)
		f = fopen("/sys/class/power_supply/AC0/online", "r");
	if (f) {
		fscanf(f, "%d", &online);
		fclose(f);
	}
	return online;
}

int platform_get_cpu_count(void)
{
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

int platform_read_msr(int cpu, uint64_t offset, uint64_t *value)
{
#if defined(__i386__) || defined(__x86_64__)
	ssize_t retval;
	int fd;
	char msr_path[256];

	snprintf(msr_path, sizeof(msr_path), "/dev/cpu/%d/msr", cpu);
	if (access(msr_path, R_OK) != 0) {
		snprintf(msr_path, sizeof(msr_path), "/dev/msr%d", cpu);
		if (access(msr_path, R_OK) != 0)
			return -1;
	}
	fd = open(msr_path, O_RDONLY);
	if (fd < 0)
		return -1;
	retval = pread(fd, value, sizeof(*value), (off_t)offset);
	close(fd);
	return (retval == (ssize_t)sizeof(*value)) ? 0 : -1;
#else
	(void)cpu; (void)offset; (void)value;
	return -1;
#endif
}

int platform_write_msr(int cpu, uint64_t offset, uint64_t value)
{
#if defined(__i386__) || defined(__x86_64__)
	ssize_t retval;
	int fd;
	char msr_path[256];

	snprintf(msr_path, sizeof(msr_path), "/dev/cpu/%d/msr", cpu);
	if (access(msr_path, W_OK) != 0) {
		snprintf(msr_path, sizeof(msr_path), "/dev/msr%d", cpu);
		if (access(msr_path, W_OK) != 0)
			return -1;
	}
	fd = open(msr_path, O_WRONLY);
	if (fd < 0)
		return -1;
	retval = pwrite(fd, &value, sizeof(value), (off_t)offset);
	close(fd);
	return (retval == (ssize_t)sizeof(value)) ? 0 : -1;
#else
	(void)cpu; (void)offset; (void)value;
	return -1;
#endif
}
