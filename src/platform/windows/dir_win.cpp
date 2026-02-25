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
 * Windows stubs for Linux directory/glob operations.
 * Provides process_directory() and process_glob() using Win32 FindFirstFile.
 */

#ifdef _WIN32

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <string>

typedef void (*callback)(const char *);

void process_directory(const char *d_name, callback fn)
{
	char pattern[MAX_PATH];
	WIN32_FIND_DATAA fd;
	HANDLE h;

	snprintf(pattern, sizeof(pattern), "%s\\*", d_name);
	h = FindFirstFileA(pattern, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do {
		if (fd.cFileName[0] == '.')
			continue;
		fn(fd.cFileName);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

void process_glob(const char *glob_pat, callback fn)
{
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(glob_pat, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;

	/* Extract directory part of the glob pattern */
	char dir[MAX_PATH];
	const char *last_sep = strrchr(glob_pat, '\\');
	if (!last_sep)
		last_sep = strrchr(glob_pat, '/');
	if (last_sep) {
		size_t dir_len = (size_t)(last_sep - glob_pat + 1);
		if (dir_len >= sizeof(dir))
			dir_len = sizeof(dir) - 1;
		memcpy(dir, glob_pat, dir_len);
		dir[dir_len] = '\0';
	} else {
		dir[0] = '\0';
	}

	do {
		char full[MAX_PATH];
		snprintf(full, sizeof(full), "%s%s", dir, fd.cFileName);
		fn(full);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

#endif /* _WIN32 */
