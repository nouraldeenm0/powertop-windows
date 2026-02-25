/*
 * getopt_win.h - getopt / getopt_long for Windows
 *
 * This is a portable implementation of getopt(3) and getopt_long(3) for use
 * on platforms that do not provide them (primarily Windows/MSVC).
 *
 * Based on the public-domain implementation by Ludvik Jerabek, with
 * enhancements for getopt_long.
 *
 * License: Public Domain / MIT
 */
#ifndef GETOPT_WIN_H
#define GETOPT_WIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Standard getopt variables */
extern char *optarg;
extern int   optind;
extern int   opterr;
extern int   optopt;

/* Standard getopt */
int getopt(int argc, char * const argv[], const char *optstring);

/* Long option descriptor (matches POSIX struct option) */
struct option {
	const char *name;
	int         has_arg;
	int        *flag;
	int         val;
};

#define no_argument       0
#define required_argument 1
#define optional_argument 2

/* getopt_long */
int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex);

#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------ */
/* Inline implementation (header-only for simplicity)                   */
/* ------------------------------------------------------------------ */
#ifdef GETOPT_WIN_IMPLEMENTATION

char *optarg = NULL;
int   optind = 1;
int   opterr = 1;
int   optopt = '?';

static int getopt_internal(int argc, char * const argv[],
                           const char *optstring,
                           const struct option *longopts,
                           int *longindex,
                           int long_only)
{
	static char *nextchar = NULL;
	static int   optreset = 0;

	if (optreset || !nextchar || !*nextchar) {
		optreset = 0;
		if (optind >= argc || argv[optind] == NULL) {
			nextchar = NULL;
			return -1;
		}
		if (argv[optind][0] != '-' ||
		    (argv[optind][1] == '\0')) {
			nextchar = NULL;
			return -1;
		}
		if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
			++optind;
			nextchar = NULL;
			return -1;
		}
		/* Handle long options */
		if (longopts && argv[optind][1] == '-') {
			const char *arg_start = argv[optind] + 2;
			const struct option *lo;
			int idx = 0;
			const struct option *match = NULL;
			int match_idx = 0;

			for (lo = longopts; lo->name; lo++, idx++) {
				size_t nlen = strlen(lo->name);
				if (strncmp(arg_start, lo->name, nlen) == 0 &&
				    (arg_start[nlen] == '\0' ||
				     arg_start[nlen] == '=')) {
					match = lo;
					match_idx = idx;
					break;
				}
			}
			++optind;
			nextchar = NULL;
			if (!match) {
				if (opterr)
					fprintf(stderr, "Unknown option: %s\n",
					        argv[optind - 1]);
				return '?';
			}
			if (longindex)
				*longindex = match_idx;
			if (match->has_arg == required_argument ||
			    match->has_arg == optional_argument) {
				const char *eq = strchr(arg_start, '=');
				if (eq) {
					optarg = (char *)(eq + 1);
				} else if (match->has_arg == required_argument &&
				           optind < argc) {
					optarg = argv[optind++];
				} else {
					optarg = NULL;
				}
			}
			if (match->flag) {
				*match->flag = match->val;
				return 0;
			}
			return match->val;
		}
		nextchar = argv[optind] + 1;
		++optind;
	}

	/* Short option */
	{
		char c = *nextchar++;
		const char *spec = strchr(optstring, c);

		if (!spec) {
			optopt = c;
			if (opterr)
				fprintf(stderr, "Unknown option: -%c\n", c);
			return '?';
		}
		if (spec[1] == ':') {
			if (*nextchar) {
				optarg = nextchar;
				nextchar = NULL;
			} else if (spec[2] == ':') {
				/* optional argument */
				optarg = NULL;
			} else if (optind < argc) {
				optarg = argv[optind++];
			} else {
				optopt = c;
				if (opterr)
					fprintf(stderr,
					        "Option -%c requires an argument\n",
					        c);
				return (optstring[0] == ':') ? ':' : '?';
			}
		} else {
			optarg = NULL;
		}
		return (unsigned char)c;
	}
}

int getopt(int argc, char * const argv[], const char *optstring)
{
	return getopt_internal(argc, argv, optstring, NULL, NULL, 0);
}

int getopt_long(int argc, char * const argv[], const char *optstring,
                const struct option *longopts, int *longindex)
{
	return getopt_internal(argc, argv, optstring, longopts, longindex, 0);
}

#endif /* GETOPT_WIN_IMPLEMENTATION */
#endif /* GETOPT_WIN_H */
