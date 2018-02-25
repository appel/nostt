#define DEBUG_CHARS	0
#define DEBUG_COLORS	0

#define ENABLE_COLOR	1

#define USAGE		"usage: nostt [-cu] <page>"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <err.h>
#include "api.h"

static int
mapfg(enum ttcolor c)
{
	switch (c) {
	case TT_BLACK:		return 30;
	case TT_BLUE:		return 34;
	case TT_GREEN:		return 32;
	case TT_CYAN:		return 36;
	case TT_RED:		return 31;
	case TT_MAGENTA:	return 35;
	case TT_YELLOW:		return 33;
	default:		return 37;
	}
}

static int
mapbg(enum ttcolor c)
{
	switch (c) {
	case TT_BLACK:		return 40;
	case TT_BLUE:		return 44;
	case TT_GREEN:		return 42;
	case TT_CYAN:		return 46;
	case TT_RED:		return 41;
	case TT_MAGENTA:	return 45;
	case TT_YELLOW:		return 43;
	default:		return 47;
	}
}

static void
putcell_bw(struct ttpage *page, int line, int col)
{
#if DEBUG_CHARS
	printf("%04x %lc ", page->chars[line][col]);
#else
	printf("%lc", page->chars[line][col]);
#endif
}

static void
putcell_color(struct ttpage *page, int line, int col)
{
	wchar_t		 wc;
	struct ttattrs	*attrs;
	struct ttattrs	*prevattrs;
	int		 colorflag;

	attrs = &page->attrs[line][col];
	if (!col) {
		colorflag = 1;
	} else {
		prevattrs = &page->attrs[line][col-1];
		colorflag = memcmp(attrs, prevattrs, sizeof(*attrs));
	}

	if (colorflag)
		printf("\e[%d;%dm", mapfg(attrs->fg), mapbg(attrs->bg));

	wc = page->chars[line][col];
#if DEBUG_CHARS
	printf("%04x %lc ", wc);
#elif DEBUG_COLORS
	printf("%x%x %lc ", attrs->fg, attrs->bg, wc);
#else
	printf("%lc", wc);
#endif
}

int
main(int argc, char **argv)
{
	const char	*id;
	int		 c;
	struct ttpage 	 page;
	enum tterr	 ret;
	struct ttgetopts getopts;
	int		 colorflag = 0;
	int		 line, col;

	setlocale(LC_ALL, "");

	memset(&getopts, 0, sizeof(getopts));
	while ((c = getopt(argc, argv, "cu")) != -1) {
		switch (c) {
		case 'c':
			colorflag = 1;
			break;
		case 'u':
			getopts.mapmode = TT_MUNICODE;
			break;
		default:
			errx(1, USAGE);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1 || !*argv[0])
		errx(1, USAGE);

	id = argv[0];
	while (*id) {
		ret = tt_get(id, &page, &getopts);
		if (ret != TT_OK)
			errx(1, "%s", tt_errstr(ret));

		if (colorflag) {
			for (line = 0; line < TT_NLINES; line++) {
				for (col = 0; col < TT_NCOLS; col++)
					putcell_color(&page, line, col);
				puts("\e[0m");
			}
		} else {
			for (line = 0; line < TT_NLINES; line++) {
				for (col = 0; col < TT_NCOLS; col++)
					putcell_bw(&page, line, col);
				putchar('\n');
			}
		}

		/* If a subpage was requested (e.g. 101-2), don't print any
		   further pages. */
		if (id == argv[0] && strchr(id, '-'))
			break;

		putchar('\n');
		id = page.nextsub;
	}

	return 0;
}
