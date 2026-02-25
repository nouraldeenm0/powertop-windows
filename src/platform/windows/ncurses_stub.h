/*
 * ncurses_stub.h - minimal ncurses stub for Windows without PDCurses
 *
 * When building on Windows without PDCurses, this header provides no-op
 * stubs for all ncurses calls used by PowerTOP so that the code compiles.
 * The interactive UI will not be available in this configuration.
 */
#ifndef NCURSES_STUB_H
#define NCURSES_STUB_H

#ifdef _WIN32
#ifndef HAVE_PDCURSES

#include <stdio.h>
#include <stdarg.h>

/* Types */
typedef int WINDOW;
#define stdscr  ((WINDOW *)NULL)

/* Colour/attribute constants */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7
#define COLOR_PAIR(n) 0
#define A_BOLD        0
#define A_REVERSE     0
#define A_NORMAL      0

/* Screen size constants */
#define LINES 25
#define COLS  80

/* Boolean */
#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

/* Key constants */
#define KEY_UP    259
#define KEY_DOWN  258
#define KEY_LEFT  260
#define KEY_RIGHT 261
#define KEY_NPAGE 338
#define KEY_PPAGE 339
#define KEY_BTAB  353
#define KEY_EXIT  361    /* 0551 octal, same as real ncurses */
#define ERR       (-1)
#define OK        0

/* Core stubs */
static inline WINDOW *initscr(void)           { return stdscr; }
static inline int endwin(void)                { return 0; }
static inline int clear(void)                 { return 0; }
static inline int refresh(void)               { return 0; }
static inline int noecho(void)                { return 0; }
static inline int echo(void)                  { return 0; }
static inline int cbreak(void)                { return 0; }
static inline int nocbreak(void)              { return 0; }
static inline int resetterm(void)             { return 0; }
static inline int keypad(WINDOW *w, int e)    { (void)w; (void)e; return 0; }
static inline int start_color(void)           { return 0; }
static inline int use_default_colors(void)    { return 0; }
static inline int attron(int a)               { (void)a; return 0; }
static inline int attroff(int a)              { (void)a; return 0; }
static inline int wattrset(WINDOW *w, int a)  { (void)w; (void)a; return 0; }
static inline int halfdelay(int t)            { (void)t; return 0; }
static inline int getch(void)                 { return ERR; }
static inline int move(int y, int x)          { (void)y; (void)x; return 0; }
static inline int clrtoeol(void)              { return 0; }
static inline int clrtobot(void)              { return 0; }
static inline int getmaxy(WINDOW *w)          { (void)w; return LINES; }
static inline int getmaxx(WINDOW *w)          { (void)w; return COLS; }
static inline int init_pair(int p, int f, int b) { (void)p;(void)f;(void)b; return 0; }
static inline int delwin(WINDOW *w)           { (void)w; return 0; }
static inline int wrefresh(WINDOW *w)         { (void)w; return 0; }
static inline int wclear(WINDOW *w)           { (void)w; return 0; }

/* Window creation */
static inline WINDOW *newwin(int h, int w, int y, int x)
{ (void)h; (void)w; (void)y; (void)x; return stdscr; }

static inline WINDOW *newpad(int h, int w)
{ (void)h; (void)w; return stdscr; }

/* Refresh with scroll position */
static inline int prefresh(WINDOW *p, int py, int px, int sy, int sx, int ey, int ex)
{ (void)p; (void)py; (void)px; (void)sy; (void)sx; (void)ey; (void)ex; return 0; }

/* Print functions */
static inline int mvprintw(int y, int x, const char *fmt, ...)
{
    (void)y; (void)x;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return 0;
}

static inline int mvwprintw(WINDOW *w, int y, int x, const char *fmt, ...)
{
    (void)w; (void)y; (void)x;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return 0;
}

static inline int printw(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return 0;
}

static inline int wprintw(WINDOW *w, const char *fmt, ...)
{
    (void)w;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    return 0;
}

static inline int getnstr(char *buf, int n)
{
    if (!fgets(buf, n, stdin))
        return ERR;
    return OK;
}

static inline int waddstr(WINDOW *w, const char *s)
{ (void)w; printf("%s", s); return 0; }

static inline int wmove(WINDOW *w, int y, int x)
{ (void)w; (void)y; (void)x; return 0; }

#endif /* !HAVE_PDCURSES */
#endif /* _WIN32 */
#endif /* NCURSES_STUB_H */
