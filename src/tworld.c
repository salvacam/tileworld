/*
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"err.h"
#include	"series.h"
#include	"res.h"
#include	"play.h"
#include	"score.h"
#include	"solution.h"
#include	"unslist.h"
#include	"help.h"
#include	"oshw.h"
#include	"cmdline.h"
#include	"ver.h"


//DKS - added.. we're going to put the main menu in here, and I don't feel
// like abstracting it. So, we'll need SDL
#include	<SDL.h>
#include    <SDL_gfxPrimitives.h>
#include	"oshw-sdl/sdlgen.h"
#include "oshw-sdl/port_cfg.h"

//DKS - added for parsedatadir() function
#include	<glob.h>
#include	<sys/stat.h>
#include	<errno.h>
#include 	<fcntl.h>
#include	<unistd.h>

/* Bell-ringing macro.
 */
#define	bell()	(silence ? (void)0 : ding())

enum { Play_None, Play_Normal, Play_Back, Play_Verify };

/* The data needed to identify what level is being played.
 */
typedef	struct gamespec {
	gameseries	series;		/* the complete set of levels */
	int		currentgame;	/* which level is currently selected */
	int		playmode;	/* which mode to play */
	int		usepasswds;	/* FALSE if passwords are to be ignored */
	int		status;		/* final status of last game played */
	int		enddisplay;	/* TRUE if the final level was completed */
	int		melindacount;	/* count for Melinda's free pass */
} gamespec;

/* Structure used to hold data collected by initoptionswithcmdline().
 */
typedef	struct startupdata {
	char       *filename;	/* which data file to use */
	char       *savefilename;	/* an alternate solution file */
	int		levelnum;	/* a selected initial level */
	int		listdirs;	/* TRUE if directories should be listed */
	int		listseries;	/* TRUE if the files should be listed */
	int		listscores;	/* TRUE if the scores should be listed */
	int		listtimes;	/* TRUE if the times should be listed */
	int		batchverify;	/* TRUE to enter batch verification */
} startupdata;

/* Structure used to hold the complete list of available series.
 */
typedef	struct seriesdata {
	gameseries *list;		/* the array of available series */
	int		count;		/* size of arary */
	tablespec	table;		/* table for displaying the array */
} seriesdata;

////DKS disable sound on PC for now:
//#ifdef PLATFORM_PC
///* TRUE suppresses sound and the console bell.
// */
//static int	silence = TRUE;
//#else //PLATFORM_PC
///* TRUE suppresses sound and the console bell.
// */
static int	silence = FALSE;
//#endif


//DKS - modified
/* TRUE means the program should attempt to run in fullscreen mode.
 */
//static int	fullscreen = FALSE;
#ifdef PLATFORM_PC
static int fullscreen = FALSE;
#else
// Must be on a GP2X or other handheld
static int fullscreen = TRUE;
#endif //PLATFORM_PC

/* FALSE suppresses all password checking.
 */
static int	usepasswds = TRUE;

/* TRUE if the user requested an idle-time histogram.
 */
static int	showhistogram = FALSE;

/* Slowdown factor, used for debugging.
 */
static int	mudsucking = 1;

/* Frame-skipping disable flag.
 */
static int	noframeskip = FALSE;

/* The sound buffer scaling factor.
 */
static int	soundbufsize = -1;

/* The initial volume level.
 */
static int	volumelevel = -1;

//DKS - added this for when user wants to restart a level or dies
// 		(I don't want the program prompting them)
//		it is used in runcurrentlevel() and playgame() to determine whether to show
//		level select overlays and get button press before starting same or next level
static int displaylevelselect = 1;
//DKS added so playgame can signal the user selected "Quit to main menu" from in-game menu
static int quittomainmenu = 0;

//DKS - don't need this anymore
/* The top of the stack of subtitles.
 */
//static void   **subtitlestack = NULL;

/*
 * Text-mode output functions.
 */
// void myflip(void)
// {
// 	SDL_Surface *p = SDL_ConvertSurface(sdlg.realscreen, sdlg.ScreenSurface->format, 0);
// 	if(SDL_MUSTLOCK(sdlg.ScreenSurface)) SDL_LockSurface(sdlg.ScreenSurface);
// #if 1
// 	SDL_SoftStretch(p, NULL, sdlg.ScreenSurface, NULL);
// #else
// 	uint16_t *d = sdlg.ScreenSurface->pixels;
// 	uint32_t *s = sdlg.realscreen->pixels, tmp;
// 	int x, y;
// 	for(y=0; y<240; y++){
// 		for(x=0; x<320; x++){
// 			tmp = *s++;
// 			*d++ = ((tmp & 0x00f80000) >> 8) | ((tmp & 0x0000fc00) >> 5) | ((tmp & 0xf8) >> 3);
// 		}
// 		d+= 320;
// 	}
// 	if(SDL_MUSTLOCK(sdlg.ScreenSurface)) SDL_UnlockSurface(sdlg.ScreenSurface);
// #endif
// 	SDL_Flip(sdlg.ScreenSurface);
// 	SDL_FreeSurface(p);
// }

/* Find a position to break a string inbetween words. The integer at
 * breakpos receives the length of the string prefix less than or
 * equal to len. The string pointer *str is advanced to the first
 * non-whitespace after the break. The original string pointer is
 * returned.
 */
static char *findstrbreak(char const **str, int maxlen, int *breakpos)
{
	char const *start;
	int		n;

retry:
	start = *str;
	n = strlen(start);
	if (n <= maxlen) {
		*str += n;
		*breakpos = n;
	} else {
		n = maxlen;
		if (isspace(start[n])) {
			*str += n;
			while (isspace(**str))
				++*str;
			while (n > 0 && isspace(start[n - 1]))
				--n;
			if (n == 0)
				goto retry;
			*breakpos = n;
		} else {
			while (n > 0 && !isspace(start[n - 1]))
				--n;
			if (n == 0) {
				*str += maxlen;
				*breakpos = maxlen;
			} else {
				*str = start + n;
				while (n > 0 && isspace(start[n - 1]))
					--n;
				if (n == 0)
					goto retry;
				*breakpos = n;
			}
		}
	}
	return (char*)start;
}

/* Render a table to the given file. This function encapsulates both
 * the process of determining the necessary widths for each column of
 * the table, and then sequentially rendering the table's contents to
 * a stream. On the first pass through the data, single-cell
 * non-word-wrapped entries are measured and each column sized to fit.
 * If the resulting table is too large for the given area, then the
 * collapsible column is reduced as necessary. If there is still
 * space, however, then the entries that span multiple cells are
 * measured in a second pass, and columns are grown to fit them as
 * well where needed. If there is still space after this, the column
 * containing word-wrapped entries may be expanded as well.
 */
void printtable(FILE *out, tablespec const *table)
{
	int const	maxwidth = 79;
	char const *mlstr;
	char const *p;
	int	       *colsizes;
	int		mlindex, mlwidth, mlpos;
	int		diff, pos;
	int		i, j, n, i0, c, w, z;

	if (!(colsizes = malloc(table->cols * sizeof *colsizes)))
		return;
	for (i = 0 ; i < table->cols ; ++i)
		colsizes[i] = 0;
	mlindex = -1;
	mlwidth = 0;
	n = 0;
	for (j = 0 ; j < table->rows ; ++j) {
		for (i = 0 ; i < table->cols ; ++n) {
			c = table->items[n][0] - '0';
			if (c == 1) {
				w = strlen(table->items[n] + 2);
				if (table->items[n][1] == '!') {
					if (w > mlwidth || mlindex != i)
						mlwidth = w;
					mlindex = i;
				} else {
					if (w > colsizes[i])
						colsizes[i] = w;
				}
			}
			i += c;
		}
	}

	w = -table->sep;
	for (i = 0 ; i < table->cols ; ++i)
		w += colsizes[i] + table->sep;
	diff = maxwidth - w;

	if (diff < 0 && table->collapse >= 0) {
		w = -diff;
		if (colsizes[table->collapse] < w)
			w = colsizes[table->collapse] - 1;
		colsizes[table->collapse] -= w;
		diff += w;
	}

	if (diff > 0) {
		n = 0;
		for (j = 0 ; j < table->rows && diff > 0 ; ++j) {
			for (i = 0 ; i < table->cols ; ++n) {
				c = table->items[n][0] - '0';
				if (c > 1 && table->items[n][1] != '!') {
					w = table->sep + strlen(table->items[n] + 2);
					for (i0 = i ; i0 < i + c ; ++i0)
						w -= colsizes[i0] + table->sep;
					if (w > 0) {
						if (table->collapse >= i && table->collapse < i + c)
							i0 = table->collapse;
						else if (mlindex >= i && mlindex < i + c)
							i0 = mlindex;
						else
							i0 = i + c - 1;
						if (w > diff)
							w = diff;
						colsizes[i0] += w;
						diff -= w;
						if (diff == 0)
							break;
					}
				}
				i += c;
			}
		}
	}
	if (diff > 0 && mlindex >= 0 && colsizes[mlindex] < mlwidth) {
		mlwidth -= colsizes[mlindex];
		w = mlwidth < diff ? mlwidth : diff;
		colsizes[mlindex] += w;
		diff -= w;
	}

	n = 0;
	for (j = 0 ; j < table->rows ; ++j) {
		mlstr = NULL;
		mlwidth = mlpos = 0;
		pos = 0;
		for (i = 0 ; i < table->cols ; ++n) {
			if (i)
				pos += fprintf(out, "%*s", table->sep, "");
			c = table->items[n][0] - '0';
			w = -table->sep;
			while (c--)
				w += colsizes[i++] + table->sep;
			if (table->items[n][1] == '-')
				fprintf(out, "%-*.*s", w, w, table->items[n] + 2);
			else if (table->items[n][1] == '+')
				fprintf(out, "%*.*s", w, w, table->items[n] + 2);
			else if (table->items[n][1] == '.') {
				z = (w - strlen(table->items[n] + 2)) / 2;
				if (z < 0)
					z = w;
				fprintf(out, "%*.*s%*s",
						w - z, w - z, table->items[n] + 2, z, "");
			} else if (table->items[n][1] == '!') {
				mlwidth = w;
				mlpos = pos;
				mlstr = table->items[n] + 2;
				p = findstrbreak(&mlstr, w, &z);
				fprintf(out, "%.*s%*s", z, p, w - z, "");
			}
			pos += w;
		}
		fputc('\n', out);
		while (mlstr && *mlstr) {
			p = findstrbreak(&mlstr, mlwidth, &w);
			fprintf(out, "%*s%.*s\n", mlpos, "", w, p);
		}
	}
	free(colsizes);
}

/* Display directory settings.
 */
static void printdirectories(void)
{
	//DKS - added for new persistent settings saved in $HOME/.tworld/port_cfg file
	printf("Persistent settings file:        %s\n", portcfgfilename);
	printf("Resource files read from:        %s\n", resdir);
	printf("Level sets read from:            %s\n", seriesdir);
	printf("Configured data files read from: %s\n", seriesdatdir);
	printf("Solution files saved in:         %s\n", savedir);
}

/*
 * Callback functions for oshw.
 */

/* An input callback that only accepts the characters Y and N.
 */
static int yninputcallback(void)
{
	switch (input(TRUE)) {
		case 'Y': case 'y':	return 'Y';
		case 'N': case 'n':	return 'N';
		case CmdWest:		return '\b';
		case CmdProceed:		return '\n';
		case CmdQuitLevel:	return -1;
		case CmdQuit:		exit(0);
	}
	return 0;
}

/* An input callback that accepts only alphabetic characters.
 */
static int keyinputcallback(void)
{
	int	ch;

	ch = input(TRUE);
	switch (ch) {
		case CmdWest:		return '\b';
		case CmdProceed:	return '\n';
		case CmdQuitLevel:	return -1;
		case CmdQuit:		exit(0);
		default:
								if (isalpha(ch))
									return toupper(ch);
	}
	return 0;
}

/* An input callback used while displaying a scrolling list.
 */
static int scrollinputcallback(int *move)
{
	int cmd;
	switch ((cmd = input(TRUE))) {
		case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
		case CmdNorth:		*move = SCROLL_UP;		break;
		case CmdPrev:		*move = SCROLL_UP;		break;
		case CmdPrevLevel:	*move = SCROLL_UP;		break;
		case CmdSouth:		*move = SCROLL_DN;		break;
		case CmdNext:		*move = SCROLL_DN;		break;
		case CmdNextLevel:	*move = SCROLL_DN;		break;
		case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
		case CmdProceed:		*move = CmdProceed;		return FALSE;
		case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
		case CmdHelp:		*move = CmdHelp;		return FALSE;
		case CmdQuit:						exit(0);

	}
	return TRUE;
}

/* An input callback used while displaying the scrolling list of scores.
 */
static int scorescrollinputcallback(int *move)
{
	int cmd;
	switch ((cmd = input(TRUE))) {
		case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
		case CmdNorth:		*move = SCROLL_UP;		break;
		case CmdPrev:		*move = SCROLL_UP;		break;
		case CmdPrevLevel:	*move = SCROLL_UP;		break;
		case CmdSouth:		*move = SCROLL_DN;		break;
		case CmdNext:		*move = SCROLL_DN;		break;
		case CmdNextLevel:	*move = SCROLL_DN;		break;
		case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
		case CmdProceed:		*move = CmdProceed;		return FALSE;
		case CmdSeeSolutionFiles:	*move = CmdSeeSolutionFiles;	return FALSE;
		case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
		case CmdHelp:		*move = CmdHelp;		return FALSE;
		case CmdQuit:						exit(0);
	}
	return TRUE;
}

/* An input callback used while displaying the scrolling list of solutions.
 */
static int solutionscrollinputcallback(int *move)
{
	int cmd;
	switch ((cmd = input(TRUE))) {
		case CmdPrev10:		*move = SCROLL_HALFPAGE_UP;	break;
		case CmdNorth:		*move = SCROLL_UP;		break;
		case CmdPrev:		*move = SCROLL_UP;		break;
		case CmdPrevLevel:	*move = SCROLL_UP;		break;
		case CmdSouth:		*move = SCROLL_DN;		break;
		case CmdNext:		*move = SCROLL_DN;		break;
		case CmdNextLevel:	*move = SCROLL_DN;		break;
		case CmdNext10:		*move = SCROLL_HALFPAGE_DN;	break;
		case CmdProceed:		*move = CmdProceed;		return FALSE;
		case CmdSeeScores:	*move = CmdSeeScores;		return FALSE;
		case CmdQuitLevel:	*move = CmdQuitLevel;		return FALSE;
		case CmdHelp:		*move = CmdHelp;		return FALSE;
		case CmdQuit:						exit(0);
	}
	return TRUE;
}

/*
 * Basic game activities.
 */

/* Return TRUE if the given level is a final level.
 */
static int islastinseries(gamespec const *gs, int index)
{
	return index == gs->series.count - 1
		|| gs->series.games[index].number == gs->series.final;
}

/* Return TRUE if the current level has a solution.
 */
static int issolved(gamespec const *gs, int index)
{
	return hassolution(gs->series.games + index);
}

/* Mark the current level's solution as replaceable.
 */
static void replaceablesolution(gamespec *gs, int change)
{
	if (change < 0)
		gs->series.games[gs->currentgame].sgflags ^= SGF_REPLACEABLE;
	else if (change > 0)
		gs->series.games[gs->currentgame].sgflags |= SGF_REPLACEABLE;
	else
		gs->series.games[gs->currentgame].sgflags &= ~SGF_REPLACEABLE;
}

/* Mark the current level's password as known to the user.
 */
static void passwordseen(gamespec *gs, int number)
{
	if (!(gs->series.games[number].sgflags & SGF_HASPASSWD)) {
		gs->series.games[number].sgflags |= SGF_HASPASSWD;
		savesolutions(&gs->series);
	}
}

/* Change the current level, ensuring that the user is not granted
 * access to a forbidden level. FALSE is returned if the specified
 * level is not available to the user.
 */
static int setcurrentgame(gamespec *gs, int n)
{

	if (n == gs->currentgame)
		return TRUE;
	if (n < 0 || n >= gs->series.count)
		return FALSE;

	if (gs->usepasswds)
		if (n > 0 && !(gs->series.games[n].sgflags & SGF_HASPASSWD)
				&& !issolved(gs, n -1))
			return FALSE;

	gs->currentgame = n;
	gs->melindacount = 0;
	return TRUE;
}

//DKS - modified for music handling
/* Change the current level by a delta value. If the user cannot go to
 * that level, the "nearest" level in that direction is chosen
 * instead. FALSE is returned if the current level remained unchanged.
 */
static int changecurrentgame(gamespec *gs, int offset)
{

	int	sign, m, n;

	if (offset == 0)
		return FALSE;

	m = gs->currentgame;
	n = m + offset;
	if (n < 0)
		n = 0;
	else if (n >= gs->series.count)
		n = gs->series.count - 1;

	if (gs->usepasswds && n > 0) {
		sign = offset < 0 ? -1 : +1;
		for ( ; n >= 0 && n < gs->series.count ; n += sign) {
			if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
					|| issolved(gs, n - 1)) {
				m = n;
				break;
			}
		}
		n = m;
		if (n == gs->currentgame && offset != sign) {
			n = gs->currentgame + offset - sign;
			for ( ; n != gs->currentgame ; n -= sign) {
				if (n < 0 || n >= gs->series.count)
					continue;
				if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
						|| issolved(gs, n - 1))
					break;
			}
		}
	}

	if (n == gs->currentgame)
		return FALSE;

	gs->currentgame = n;
	gs->melindacount = 0;
	return TRUE;
}

/* Return TRUE if Melinda is watching Chip's progress on this level --
 * i.e., if it is possible to earn a pass to the next level.
 */
static int melindawatching(gamespec const *gs)
{
	if (!gs->usepasswds)
		return FALSE;
	if (islastinseries(gs, gs->currentgame))
		return FALSE;
	if (gs->series.games[gs->currentgame + 1].sgflags & SGF_HASPASSWD)
		return FALSE;
	if (issolved(gs, gs->currentgame))
		return FALSE;
	return TRUE;
}

/*
 * The subtitle stack
 */
//DKS - don't need this anymore
//static void pushsubtitle(char const *subtitle)
//{
//    void      **stk;
//    int		n;
//
//    if (!subtitle)
//	subtitle = "";
//    n = strlen(subtitle) + 1;
//    stk = NULL;
//    xalloc(stk, sizeof(void**) + n);
//    *stk = subtitlestack;
//    subtitlestack = stk;
//    memcpy(stk + 1, subtitle, n);
//    setsubtitle(subtitle);
//}

//DKS - don't need this anymore
//static void popsubtitle(void)
//{
//    void      **stk;
//
//    if (subtitlestack) {
//	stk = *subtitlestack;
//	free(subtitlestack);
//	subtitlestack = stk;
//    }
//    setsubtitle(subtitlestack ? (char*)(subtitlestack + 1) : NULL);
//}

//DKS - don't need this anymore
//static void changesubtitle(char const *subtitle)
//{
//    int		n;
//
//    if (!subtitle)
//	subtitle = "";
//    n = strlen(subtitle) + 1;
//    xalloc(subtitlestack, sizeof(void**) + n);
//    memcpy(subtitlestack + 1, subtitle, n);
//    setsubtitle(subtitle);
//}

/*
 *
 */

static void dohelp(int topic)
{
	//    pushsubtitle("Help");
	switch (topic) {
		case Help_First:
		case Help_FileListKeys:
		case Help_ScoreListKeys:
			onlinecontexthelp(topic);
			break;
		default:
			onlinemainhelp(topic);
			break;
	}
	//    popsubtitle();
}

//DKS - modified
static int showsolutionfiles(gamespec *gs)
{
	tablespec		table;
	char const	      **filelist;
	int			readonly = FALSE;
	int			count, current, f, n;

	if (haspathname(gs->series.name) || (gs->series.savefilename
				&& haspathname(gs->series.savefilename))) {
		bell();
		return FALSE;
	} else if (!createsolutionfilelist(&gs->series, FALSE, &filelist,
				&count, &table)) {
		bell();
		return FALSE;
	}

	current = -1;
	n = 0;
	if (gs->series.savefilename) {
		for (n = 0 ; n < count ; ++n)
			if (!strcmp(filelist[n], gs->series.savefilename))
				break;
		if (n == count)
			n = 0;
		else
			current = n;
	}

	//    pushsubtitle(gs->series.name);
	for (;;) {
		f = displaylist("SOLUTION FILES", &table, &n,
				solutionscrollinputcallback);
		if (f == CmdProceed) {
			readonly = FALSE;
			break;
		} else if (f == CmdSeeScores) {
			readonly = TRUE;
			break;
		} else if (f == CmdQuitLevel) {
			n = -1;
			break;
		} else if (f == CmdHelp) {
			dohelp(Help_FileListKeys);
		}
	}
	//    popsubtitle();

	f = n >= 0 && n != current;
	if (f) {
		clearsolutions(&gs->series);
		if (!gs->series.savefilename)
			gs->series.savefilename = getpathbuffer();
		sprintf(gs->series.savefilename, "%.*s", getpathbufferlen(),
				filelist[n]);
		if (readsolutions(&gs->series)) {
			if (readonly)
				gs->series.gsflags |= GSF_NOSAVING;
		} else {
			bell();
		}
		n = gs->currentgame;
		gs->currentgame = 0;
		passwordseen(gs, 0);
		changecurrentgame(gs, n);
	}

	freesolutionfilelist(filelist, &table);
	return f;
}


//DKS - modified
static int showscores(gamespec *gs)
{
	tablespec	table;
	int	       *levellist;
	int		ret = FALSE;
	int		count, f, n;

restart:
	if (!createscorelist(&gs->series, gs->usepasswds, CHAR_MZERO,
				&levellist, &count, &table)) {
		bell();
		return ret;
	}
	for (n = 0 ; n < count ; ++n)
		if (levellist[n] == gs->currentgame)
			break;
	//    pushsubtitle(gs->series.name);
	for (;;) {
		f = displaylist(gs->series.filebase, &table, &n,
				scorescrollinputcallback);
		if (f == CmdProceed) {
			n = levellist[n];
			break;
		} else if (f == CmdSeeSolutionFiles) {
			if (!(gs->series.gsflags & GSF_NODEFAULTSAVE)) {
				n = levellist[n];
				break;
			}
		} else if (f == CmdQuitLevel) {
			n = -1;
			break;
		} else if (f == CmdHelp) {
			dohelp(Help_ScoreListKeys);
		}
	}
	//    popsubtitle();
	freescorelist(levellist, &table);
	if (f == CmdSeeSolutionFiles) {
		setcurrentgame(gs, n);
		ret = showsolutionfiles(gs);
		goto restart;
	}
	if (n < 0)
		return ret;
	return setcurrentgame(gs, n) || ret;
}

/* Obtain a password from the user and move to the requested level.
 */
static int selectlevelbypassword(gamespec *gs)
{
	char	passwd[5] = "";
	int		n;

	setgameplaymode(BeginInput);
	n = displayinputprompt("Enter Password", passwd, 4, keyinputcallback);
	setgameplaymode(EndInput);
	if (!n)
		return FALSE;

	n = findlevelinseries(&gs->series, 0, passwd);
	if (n < 0) {
		bell();
		return FALSE;
	}
	passwordseen(gs, n);
	return setcurrentgame(gs, n);
}

/*
 * The game-playing functions.
 */

#define	leveldelta(n)	if (!changecurrentgame(gs, (n))) { bell(); continue; }

//DKS - modified
static int startinput(gamespec *gs)
{
	static int	lastlevel = -1;
	char	yn[2];
	int		cmd, n;
	char	buf[300];

	if (gs->currentgame != lastlevel) {
		lastlevel = gs->currentgame;
		setstepping(0, FALSE);
	}
	drawscreen(TRUE, FALSE);
	gs->playmode = Play_None;

	SDL_Rect srcrect, dstrect;

	//copy shaded backdrop for interface window and level title
	srcrect.x = 4;
	srcrect.y = 156;
	srcrect.w = 312;
	srcrect.h = 80;
	SDL_BlitSurface(sdlg.infobg, &srcrect, sdlg.screen, &srcrect);

	if (gs->currentgame > 0) {	
		//draw L-trigger icon and text
		srcrect.x = 0;
		srcrect.y = 1;
		srcrect.w = 31;
		srcrect.h = 12;
		dstrect.x = 11;
		dstrect.y = 164;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect); 
		SFont_Write(sdlg.screen, sdlg.font_tiny, 12, 182, "Prev 10");

		//draw left joy icon and text
		srcrect.x = 231;
		srcrect.y = 0;
		srcrect.w = 18;
		srcrect.h = 18;
		dstrect.x = 66;
		dstrect.y = 160;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);
		SFont_Write(sdlg.screen, sdlg.font_tiny, 65, 182, "Prev");
	}

	if (gs->currentgame < (gs->series.count - 1)) {	
		//draw R-trigger icon and text
		srcrect.x = 32;
		srcrect.y = 1;
		srcrect.w = 30;
		srcrect.h = 12;
		dstrect.x = 280;
		dstrect.y = 164;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);
		SFont_Write(sdlg.screen, sdlg.font_tiny, 280, 182, "Next 10");


		//draw right joy icon and text
		srcrect.x = 252;
		srcrect.y = 0;
		srcrect.w = 18;
		srcrect.h = 18;
		dstrect.x = 236;
		dstrect.y = 160;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);
		SFont_Write(sdlg.screen, sdlg.font_tiny, 237, 182, "Next");
	}

	//draw select/start (menu) button icon & text
	srcrect.x = 64;
	srcrect.y = 0;
	srcrect.w = 42;
	srcrect.h = 15;
	dstrect.x = 139;
	dstrect.y = 161;
	dstrect.w = srcrect.w;
	dstrect.h = srcrect.h;
	SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);
	SFont_Write(sdlg.screen, sdlg.font_tiny, 140, 182, "Main Menu");


	//draw level's name
	strcpy(buf, "\"");
	// limit display of huge level names
	for (n = 0; n < 70; ++n) {
		buf[n + 1] = gs->series.games[gs->currentgame].name[n];
	}

	//if the filename was huge, cut it off nicely:
	buf[71] = '.';
	buf[72] = '.';
	buf[73] = '.';
	buf[74] = '\0';
	strcat(buf, "\"");
	SFont_WriteCenter(sdlg.screen, sdlg.font_tiny, 196, buf );

	SFont_Write(sdlg.screen, sdlg.font_tiny, 12, 216, "SOLVED:");

	if (issolved(gs, gs->currentgame)) {
		// if level's already been solved, draw a check and the best time
		srcrect.x = 119;
		srcrect.y = 1;
		srcrect.w = 14;
		srcrect.h = 12;
		dstrect.x = 50;
		dstrect.y = 213;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);

		if (gs->series.games[gs->currentgame].time)
			// Level actually has a time limit
			sprintf(buf, "BEST TIME: %d sec rem.", 
					(gs->series.games[gs->currentgame].time) - 
					(gs->series.games[gs->currentgame].besttime / TICKS_PER_SECOND));
		else
			// No time limit
			sprintf(buf, "BEST TIME: %d sec", 
					(gs->series.games[gs->currentgame].besttime / TICKS_PER_SECOND));

		SFont_WriteCenter(sdlg.screen, sdlg.font_tiny, 216 ,buf);

		int basescore, timescore;
		long totalscore;
		getscoresforlevel(&gs->series, gs->currentgame,
				&basescore, &timescore, &totalscore);
		sprintf(buf, "HI-SCORE: %d", basescore+timescore);
		SFont_Write(sdlg.screen, sdlg.font_tiny, (320 - SFont_TextWidth(sdlg.font_tiny, buf) - 12) , 216, buf);


	} else {
		// if level's not solved, draw a X
		srcrect.x = 108;
		srcrect.y = 1;
		srcrect.w = 11;
		srcrect.h = 12;
		dstrect.x = 50;
		dstrect.y = 213;
		dstrect.w = srcrect.w;
		dstrect.h = srcrect.h;
		SDL_BlitSurface(sdlg.sprites, &srcrect, sdlg.screen, &dstrect);
	}

	SDL_BlitSurface(sdlg.screen, NULL, sdlg.realscreen, NULL);
	SDL_Flip(sdlg.realscreen);
	// myflip();


	for (;;) {

		setkeyboardinputmode(TRUE);
#ifdef PLATFORM_GP2X	

		for (;;) {
			n = anykey();
			if (n == GP2X_BUTTON_L) {
				leveldelta(-10);
				return CmdNone;
			} else if (n == GP2X_BUTTON_R) {
				leveldelta(+10);
				return CmdNone;
			} else if (n == GP2X_BUTTON_RIGHT) {
				leveldelta(+1);
				return CmdNone;
			} else if (n == GP2X_BUTTON_LEFT) {
				leveldelta(-1);
				return CmdNone;
			} else if (n == GP2X_BUTTON_SELECT) {
				return CmdQuitLevel;
			} else if ((n == GP2X_BUTTON_START) || (n == GP2X_BUTTON_A) ||
					(n == GP2X_BUTTON_B) || (n == GP2X_BUTTON_X) || 
					(n == GP2X_BUTTON_Y)) {
				gs->playmode = Play_Normal;
				setkeyboardinputmode(FALSE);
				controlleddelay(150);
				return CmdProceed;
			}
		}		
#else

		cmd = input(TRUE);

		switch (cmd) {
			case CmdProceed:	gs->playmode = Play_Normal; setkeyboardinputmode(FALSE); controlleddelay(200);	return CmdProceed;
			case CmdQuitLevel:					return cmd;
														//DKS: modified for GCW (levels scroll too fast with analog stick)
#ifdef PLATFORM_GCW
			case CmdPrev10:	leveldelta(-10); controlleddelay(150);	return CmdNone;
			case CmdPrev:		leveldelta(-1); controlleddelay(150); return CmdNone;
			case CmdPrevLevel:	leveldelta(-1); controlleddelay(150); return CmdNone;
			case CmdNextLevel:	leveldelta(+1); controlleddelay(150); return CmdNone;
			case CmdNext:		leveldelta(+1); controlleddelay(150); return CmdNone;
			case CmdNext10:	leveldelta(+10);		return CmdNone;
#else
			case CmdPrev10:	leveldelta(-10);		return CmdNone;
			case CmdPrev:		leveldelta(-1);			return CmdNone;
			case CmdPrevLevel:	leveldelta(-1);			return CmdNone;
			case CmdNextLevel:	leveldelta(+1);			return CmdNone;
			case CmdNext:		leveldelta(+1);			return CmdNone;
			case CmdNext10:	leveldelta(+10);		return CmdNone;
#endif
			case CmdStepping:	changestepping(4, TRUE);	break;
			case CmdSubStepping:	changestepping(1, TRUE);	break;
			case CmdVolumeUp:	changevolume(+2, TRUE);		break;
			case CmdVolumeDown:	changevolume(-2, TRUE);		break;
			case CmdHelp:		dohelp(Help_KeysBetweenGames);	break;
			case CmdQuit:						exit(0);
			case CmdPlayback:
													if (prepareplayback()) {
														gs->playmode = Play_Back;
														return CmdProceed;
													}
													bell();
													break;
			case CmdCheckSolution:
													if (prepareplayback()) {
														gs->playmode = Play_Verify;
														return CmdProceed;
													}
													bell();
													break;
			case CmdReplSolution:
													if (issolved(gs, gs->currentgame))
														replaceablesolution(gs, -1);
													else
														bell();
													break;
			case CmdKillSolution:
													if (!issolved(gs, gs->currentgame)) {
														bell();
														break;
													}
													yn[0] = '\0';
													setgameplaymode(BeginInput);
													n = displayinputprompt("Really delete solution?",
															yn, 1, yninputcallback);
													setgameplaymode(EndInput);
													if (n && *yn == 'Y')
														if (deletesolution())
															savesolutions(&gs->series);
													break;
			case CmdSeeScores:
													if (showscores(gs))
														return CmdNone;
													break;
			case CmdSeeSolutionFiles:
													if (showsolutionfiles(gs))
														return CmdNone;
													break;
			case CmdGotoLevel:
													if (selectlevelbypassword(gs))
														return CmdNone;
													break;
			default:
													continue;
		}
#endif //PLATFORM_GP2X

		drawscreen(TRUE, FALSE);
	}
}

//DKS - modifed to get passed additional int, newbesttime, which is zero if a new
// best time wasn't achieved, or set to the new best time in seconds remaining.
/* Get a key command from the user at the completion of the current
 * level.
 */
static int endinput(gamespec *gs, int newbesttime, int wasbesttime)
{
	int		bscore = 0, tscore = 0;
	long	gscore = 0;

	if (gs->status < 0) {
		SDL_Rect dstrect;
		dstrect.x = (sdlg.screen->w - sdlg.oopsbg->w) / 2;
		dstrect.y = (sdlg.screen->h - sdlg.oopsbg->h) / 2;
		dstrect.h = sdlg.oopsbg->h;
		dstrect.w = sdlg.oopsbg->w;
		SDL_BlitSurface(sdlg.oopsbg, NULL, sdlg.screen, &dstrect);
		SFont_WriteCenter(sdlg.screen, sdlg.font_big, dstrect.y + 4, "OOPS");
		SDL_BlitSurface(sdlg.screen, NULL, sdlg.realscreen, NULL);
		SDL_Flip(sdlg.realscreen);
		// myflip();
		controlleddelay(750);
		displaylevelselect = 0;
		return TRUE;
	} else if (gs->status == 0) {
		//DKS added this check for gs->status, so we don't prompt for input
		// when we're restarting a level or selecting a new one
		return(TRUE);
	} else {

		// player completed level

		displaylevelselect = 1;

		getscoresforlevel(&gs->series, gs->currentgame,
				&bscore, &tscore, &gscore);

		//DKS - this now has more parameters so we can show the new best time, if 
		//			any.
		//    displayendmessage(bscore, tscore, gscore, gs->status);
		displayendmessage(bscore, tscore, gscore, gs->status, newbesttime, wasbesttime);

		SDL_BlitSurface(sdlg.screen, NULL, sdlg.realscreen, NULL);
		SDL_Flip(sdlg.realscreen);
		// myflip();
		controlleddelay(600);
		SFont_WriteCenter(sdlg.screen, sdlg.font_tiny, 226, "Press any button");
		SDL_BlitSurface(sdlg.screen, NULL, sdlg.realscreen, NULL);
		SDL_Flip(sdlg.realscreen);
		// myflip();

		anykey();
		if (gs->status > 0) {
			if (islastinseries(gs, gs->currentgame))
				gs->enddisplay = TRUE;
			else
				changecurrentgame(gs, +1);
		}    

		return TRUE;
	}
}

//DKS modified
/* Get a key command from the user at the completion of the current
 * series.
 */
static int finalinput(gamespec *gs)
{
	int	cmd;

	for (;;) {
		cmd = input(TRUE);
		switch (cmd) {
			case CmdSameLevel:
			case CmdSame:
				return TRUE;
			case CmdPrevLevel:
			case CmdPrev:
			case CmdNextLevel:
			case CmdNext:
				setcurrentgame(gs, 0);
				return TRUE;
			case CmdQuit:
				exit(0);
			default:
				return FALSE;
		}
	}

}

//DKS new in-game menu returns -1 if user wants to quit to main menu,
//0 if nothing happened (user quit menu), 1 if user wants to
//restart level, 2 if user wants to select new level
static int ingamemenu(void)
{
	setsoundeffects(0);

	setkeyboardinputmode(TRUE);

	SDL_Surface *tmpscreen;
	SDL_Rect tmprect, dstrect;

	int cursor_pos = 0;
	int selection_made = -1;

	//DKS new
	tmpscreen = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCCOLORKEY,
			sdlg.realscreen->w, sdlg.realscreen->h, sdlg.realscreen->format->BitsPerPixel,
			sdlg.realscreen->format->Rmask, sdlg.realscreen->format->Gmask,
			sdlg.realscreen->format->Bmask, sdlg.realscreen->format-> Amask);

	// menu loop
	while (selection_made == -1) {
		SDL_BlitSurface(sdlg.screen, NULL, tmpscreen, NULL);

		tmprect.x = 4;
		tmprect.y = 4;
		tmprect.h = 148;
		tmprect.w = 110;
		dstrect.x = 49;
		dstrect.y = (sdlg.screen->h - tmprect.h) / 2;
		dstrect.h = tmprect.h;
		dstrect.w = tmprect.w;


		SDL_BlitSurface(sdlg.infobg, &tmprect, tmpscreen, &dstrect);

		tmprect.x = 199;
		tmprect.y = 4;
		tmprect.h = 148;
		tmprect.w = 120;
		dstrect.x = 159;
		dstrect.y = (sdlg.screen->h - tmprect.h) / 2;
		dstrect.h = tmprect.h;
		dstrect.w = tmprect.w;

		SDL_BlitSurface(sdlg.infobg, &tmprect, tmpscreen, &dstrect);

		SFont_Write(tmpscreen, sdlg.font_small, 75, 65, "RETURN TO GAME");
		if (cursor_pos == 0) {
			SFont_Write(tmpscreen, sdlg.font_small, 60, 65, ">");
		}

		SFont_Write(tmpscreen, sdlg.font_small, 75, 95, "RESTART LEVEL");
		if (cursor_pos == 1) {
			SFont_Write(tmpscreen, sdlg.font_small, 60, 95, ">");
		}

		SFont_Write(tmpscreen, sdlg.font_small, 75, 125, "SELECT NEW LEVEL");
		if (cursor_pos == 2) {
			SFont_Write(tmpscreen, sdlg.font_small, 60, 125, ">");
		}

		SFont_Write(tmpscreen, sdlg.font_small, 75, 155, "QUIT TO MAIN MENU");
		if (cursor_pos == 3) {
			SFont_Write(tmpscreen, sdlg.font_small, 60, 155, ">");
		}

		controlleddelay(0);

		SDL_BlitSurface(tmpscreen, NULL, sdlg.realscreen, NULL);            
		SDL_Flip(sdlg.realscreen);
		// myflip();

		switch (input(FALSE)) {
			case CmdSouth:
				controlleddelay(230);
				cursor_pos++;
				if (cursor_pos == 4) {
					cursor_pos = 3;
				}
				break;
			case CmdNorth:
				controlleddelay(230);
				cursor_pos--;
				if (cursor_pos == -1) {
					cursor_pos = 0;
				}
				break;
			case CmdProceed:
				selection_made = cursor_pos;
				controlleddelay(140);
				break;
			case CmdQuitLevel:
				// close menu and return
				selection_made = 0;
				break;
			default:
				break;
		}
	}

	SDL_FreeSurface(tmpscreen);
	setkeyboardinputmode(FALSE);

	setsoundeffects(1);

	switch (selection_made) {
		case 0:
			//user wants to return to game
			return 0;
		case 1:
			//user wants to restart level
			return 1;                
		case 2:
			//user wants to select new level
			return 2;
		case 3:
			//user wants to quit to main menu
			return -1;
		default:
			return 0;
	}				
}

//DKS - modified to add two parameters that this function updates:
// newbesttime is set to best time in seconds (negative values indicate
//		completion of level with no time limit and the absolute value
//		gives the actual time in seconds taken to complete it.
//	wasbesttime is set to 0 if a new record time was not set, 1 if was.
/* Play the current level, using firstcmd as the initial key command,
 * and returning when the level's play ends. The return value is FALSE
 * if play ended because the user restarted or changed the current
 * level (indicating that the program should not prompt the user
 * before continuing). If the return value is TRUE, the gamespec
 * structure's status field will contain the return value of the last
 * call to doturn() -- i.e., positive if the level was completed
 * successfully, negative if the level ended unsuccessfully. Likewise,
 * the gamespec structure will be updated if the user ended play by
 * changing the current level.
 */
static int playgame(gamespec *gs, int firstcmd, int *newbesttime, int *wasbesttime)
{

	int	render, lastrendered;
	int	cmd, n, i;

	cmd = firstcmd;
	if (cmd == CmdProceed)
		cmd = CmdNone;

	gs->status = 0;
	setgameplaymode(BeginPlay);
	render = lastrendered = TRUE;

	playgamesongs();

	for (;;) {

		n = doturn(cmd);
		drawscreen(render, TRUE);
		lastrendered = render;
		if (n)
			break;
		render = waitfortick() || noframeskip;
		cmd = input(FALSE);
		if (cmd == CmdQuitLevel) {

			setgameplaymode(SuspendPlayShuttered);
			drawscreen(TRUE, TRUE);
			i = ingamemenu();
			switch (i) {
				case -1:
					//user wants to quit to main menu
					//dks added 11/07/07:
					displaylevelselect = 1; // get runcurrentlevel ready for next plays
					quittomainmenu = 1;
					quitgamestate();
					setgameplaymode(EndPlay);
					return FALSE;
					break;
				case 0:
					//user exited menu wanting to continue
					cmd = CmdNone;
					setgameplaymode(ResumePlay);
					continue;
					break;
				case 1:
					//user wants to restart level
					cmd = CmdNone;
					quitgamestate();
					setgameplaymode(EndPlay);
					displaylevelselect = 0;
					return FALSE;
					//continue;				
					break;
				case 2: 
					//user wants to select new level
					cmd = CmdNone;
					//gs->status = 1;
					quitgamestate();
					setgameplaymode(EndPlay);
					displaylevelselect = 1;
					return FALSE;
					goto quitloop;
					break;
				default:
					continue;

			}
		}

		if (!(cmd >= CmdMoveFirst && cmd <= CmdMoveLast)) {
			switch (cmd) {
				case CmdPreserve:					break;
				case CmdPrevLevel:		n = -1;		goto quitloop;
				case CmdNextLevel:		n = +1;		goto quitloop;
				case CmdSameLevel:		n = 0;		goto quitloop;
				case CmdDebugCmd1:				break;
				case CmdDebugCmd2:				break;
				case CmdQuit:					exit(0);
				case CmdVolumeUp:
													changevolume(+2, TRUE);
													cmd = CmdNone;
													break;
				case CmdVolumeDown:
													changevolume(-2, TRUE);
													cmd = CmdNone;
													break;
				case CmdPauseGame:
													setgameplaymode(SuspendPlayShuttered);
													drawscreen(TRUE, TRUE);
													setdisplaymsg("(paused)", 1, 1);
													for (;;) {
														switch (input(TRUE)) {
															case CmdQuit:		exit(0);
															case CmdPauseGame:	break;
															default:			continue;
														}
														break;
													}
													setgameplaymode(ResumePlay);
													cmd = CmdNone;
													break;
				case CmdHelp:
													setgameplaymode(SuspendPlay);
													dohelp(Help_KeysDuringGame);
													setgameplaymode(ResumePlay);
													cmd = CmdNone;
													break;
				case CmdCheatNorth:     case CmdCheatWest:	break;
				case CmdCheatSouth:     case CmdCheatEast:	break;
				case CmdCheatHome:				break;
				case CmdCheatKeyRed:    case CmdCheatKeyBlue:	break;
				case CmdCheatKeyYellow: case CmdCheatKeyGreen:	break;
				case CmdCheatBootsIce:  case CmdCheatBootsSlide:	break;
				case CmdCheatBootsFire: case CmdCheatBootsWater:	break;
				case CmdCheatICChip:				break;
				default:
														cmd = CmdNone;
														break;
			}
		}
	}
	if (!lastrendered)
		drawscreen(TRUE, TRUE);
	setgameplaymode(EndPlay);

	if (n > 0) {

		//	if (replacesolution()) {
		//	    savesolutions(&gs->series);
		//	}

		//replacesolution now returns the new best time (in seconds remaining)
		// Also, newbesttime gets passed back to runcurrentlevel and it will be
		// picked up on by displayendmessage() and endinput()

		*wasbesttime = replacesolution(newbesttime);

		if (*wasbesttime) {
			savesolutions(&gs->series);
		}	

		//player successfully beat level, prompt for next level, though
		displaylevelselect = 1;
	} else {
		//player died, don't display level selection GUI
		displaylevelselect = 0;
	}

	gs->status = n;
	return TRUE;

quitloop:
	if (!lastrendered)
		drawscreen(TRUE, TRUE);
	quitgamestate();
	setgameplaymode(EndPlay);

	if (n)
		changecurrentgame(gs, n);	

	return FALSE;
}
/* Play back the user's best solution for the current level in real
 * time. Other than the fact that this function runs from a
 * prerecorded series of moves, it has the same behavior as
 * playgame().
 */
static int playbackgame(gamespec *gs)
{
	int	render, lastrendered, n;

	drawscreen(TRUE, TRUE);

	gs->status = 0;
	setgameplaymode(BeginPlay);
	render = lastrendered = TRUE;
	for (;;) {
		n = doturn(CmdNone);
		drawscreen(render, TRUE);
		lastrendered = render;
		if (n)
			break;
		render = waitfortick() || noframeskip;
		switch (input(FALSE)) {
			case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
			case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
			case CmdSameLevel:					goto quitloop;
			case CmdPlayback:					goto quitloop;
			case CmdQuitLevel:					goto quitloop;
			case CmdQuit:						exit(0);
			case CmdVolumeUp:
													changevolume(+2, TRUE);
													break;
			case CmdVolumeDown:
													changevolume(-2, TRUE);
													break;
			case CmdPauseGame:
													setgameplaymode(SuspendPlay);
													setdisplaymsg("(paused)", 1, 1);
													for (;;) {
														switch (input(TRUE)) {
															case CmdQuit:		exit(0);
															case CmdPauseGame:	break;
															default:		continue;
														}
														break;
													}
													setgameplaymode(ResumePlay);
													break;
			case CmdHelp:
													setgameplaymode(SuspendPlay);
													dohelp(Help_None);
													setgameplaymode(ResumePlay);
													break;
		}
	}
	if (!lastrendered)
		drawscreen(TRUE, TRUE);
	setgameplaymode(EndPlay);
	gs->playmode = Play_None;
	if (n < 0)
		replaceablesolution(gs, +1);
	if (n > 0) {
		if (checksolution())
			savesolutions(&gs->series);
		if (islastinseries(gs, gs->currentgame))
			n = 0;
	}
	gs->status = n;
	return TRUE;

quitloop:
	if (!lastrendered)
		drawscreen(TRUE, TRUE);
	quitgamestate();
	setgameplaymode(EndPlay);
	gs->playmode = Play_None;
	return FALSE;
}

/* Quickly play back the user's best solution for the current level
 * without rendering and without using the timer the keyboard. The
 * playback stops when the solution is finished or gameplay has
 * ended.
 */
static int verifyplayback(gamespec *gs)
{
	int	n;

	gs->status = 0;
	setdisplaymsg("Verifying ...", 1, 0);
	setgameplaymode(BeginVerify);
	for (;;) {
		n = doturn(CmdNone);
		if (n)
			break;
		advancetick();
		switch (input(FALSE)) {
			case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
			case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
			case CmdSameLevel:					goto quitloop;
			case CmdPlayback:					goto quitloop;
			case CmdQuitLevel:					goto quitloop;
			case CmdQuit:						exit(0);
		}
	}
	gs->playmode = Play_None;
	quitgamestate();
	drawscreen(TRUE, TRUE);
	setgameplaymode(EndVerify);
	if (n < 0) {
		setdisplaymsg("Invalid solution!", 1, 1);
		replaceablesolution(gs, +1);
	}
	if (n > 0) {
		if (checksolution())
			savesolutions(&gs->series);
		setdisplaymsg(NULL, 0, 0);
		if (islastinseries(gs, gs->currentgame))
			n = 0;
	}
	gs->status = n;
	return TRUE;

quitloop:
	setdisplaymsg(NULL, 0, 0);
	gs->playmode = Play_None;
	setgameplaymode(EndVerify);
	return FALSE;
}

//DKS - modified
static int runcurrentlevel(gamespec *gs)
{
	int ret = TRUE;
	int	cmd;
	int	valid, f;

	//dks new variable
	// wasbesttime is set to TRUE by playgame() if a new record time is recorded, FALSE otherwise
	// newbesttime is set to new best time recorded & when both passed and 
	// checked in endinput() and displayendmessage(), notifies the player visually.
	int newbesttime = 0;
	int wasbesttime = FALSE;

	sync();

	if (gs->enddisplay) {
		gs->enddisplay = FALSE;
		//	changesubtitle(NULL);
		setenddisplay();
		drawscreen(TRUE, TRUE);

		//DKS now needs extra 0s at end to say no, there's no new best time
		//	displayendmessage(0, 0, 0, 0);
		displayendmessage(0, 0, 0, 0, 0, 0);
		endgamestate();
		return finalinput(gs);
	}

	valid = initgamestate(gs->series.games + gs->currentgame,
			gs->series.ruleset);


	// changesubtitle(gs->series.games[gs->currentgame].name);
	passwordseen(gs, gs->currentgame);
	if (!islastinseries(gs, gs->currentgame))
		if (!valid || gs->series.games[gs->currentgame].unsolvable)
			passwordseen(gs, gs->currentgame + 1);

	if (displaylevelselect) {
		cmd = startinput(gs);
	} else {
		cmd = CmdProceed;
	}

	displaylevelselect = 1;

	if (cmd == CmdQuitLevel) {
		ret = FALSE;
	} else {
		if (cmd != CmdNone) {
			if (valid) {
				switch (gs->playmode) {

					//		  case Play_Normal:	f = playgame(gs, cmd);		break;
					case Play_Normal:	f = playgame(gs, cmd, &newbesttime, &wasbesttime);		break;
					case Play_Back:	f = playbackgame(gs);		break;
					case Play_Verify:	f = verifyplayback(gs);		break;
					default:		f = FALSE;			break;
				}
				if (f) {
					//f was positive, meaning player did not change levels or quit
					//DKS - now gets passed newbesttime
					//ret = endinput(gs);
					ret = endinput(gs, newbesttime, wasbesttime);

				} else {
					//bell();
					//DKS new 11/07/07:
					// user wants to either change levels or quit
					ret = !quittomainmenu;
				}
			}
		}
	}

	endgamestate();
	return ret;
}

static int batchverify(gameseries *series, int display)
{
	gamesetup  *game;
	int		valid = 0, invalid = 0;
	int		i, f;

	batchmode = TRUE;

	for (i = 0, game = series->games ; i < series->count ; ++i, ++game) {
		if (!hassolution(game))
			continue;
		if (initgamestate(game, series->ruleset) && prepareplayback()) {
			setgameplaymode(BeginVerify);
			while (!(f = doturn(CmdNone)))
				advancetick();
			setgameplaymode(EndVerify);
			if (f > 0) {
				++valid;
				checksolution();
			} else {
				++invalid;
				game->sgflags |= SGF_REPLACEABLE;
				if (display)
					printf("Solution for level %d is invalid\n", game->number);
			}
		}
		endgamestate();
	}

	if (display) {
		if (valid + invalid == 0) {
			printf("No solutions were found.\n");
		} else {
			printf("  Valid solutions:%4d\n", valid);
			printf("Invalid solutions:%4d\n", invalid);
		}
	}
	return invalid;
}

/*
 * Game selection functions
 */

/* Display the full selection of available series to the user as a
 * scrolling list, and permit one to be selected. When one is chosen,
 * pick one of levels to be the current level. All fields of the
 * gamespec structure are initiailzed. If autosel is TRUE, then the
 * function will skip the display if there is only one series
 * available. If defaultseries is not NULL, and matches the name of
 * one of the series in the array, then the scrolling list will be
 * initialized with that series selected. If defaultlevel is not zero,
 * and a level in the selected series that the user is permitted to
 * access matches it, then that level will be thhe initial current
 * level. The return value is zero if nothing was selected, negative
 * if an error occurred, or positive otherwise.
 */
static int selectseriesandlevel(gamespec *gs, seriesdata *series, int autosel,
		char const *defaultseries, int defaultlevel)
{
	int	okay, f, n;

	if (series->count < 1) {
		errmsg(NULL, "no level sets found");
		return -1;
	}

	okay = TRUE;
	if (series->count == 1 && autosel) {
		getseriesfromlist(&gs->series, series->list, 0);
	} else {
		n = 0;
		if (defaultseries) {
			n = series->count;
			while (n)
				if (!strcmp(series->list[--n].filebase, defaultseries))
					break;
		}
		for (;;) {
			f = displaylist("   Welcome to Tile World. Type ? or F1 for help.",
					&series->table, &n, scrollinputcallback);
			if (f == CmdProceed) {
				getseriesfromlist(&gs->series, series->list, n);
				okay = TRUE;
				break;
			} else if (f == CmdQuitLevel) {
				okay = FALSE;
				break;
			} else if (f == CmdHelp) {
				//		pushsubtitle("Help");
				dohelp(Help_First);
				//		popsubtitle();
			}
		}
	}
	freeserieslist(series->list, series->count, &series->table);
	if (!okay)
		return 0;

	if (!readseriesfile(&gs->series)) {
		errmsg(gs->series.filebase, "cannot read data file");
		freeseriesdata(&gs->series);
		return -1;
	}
	if (gs->series.count < 1) {
		errmsg(gs->series.filebase, "no levels found in data file");
		freeseriesdata(&gs->series);
		return -1;
	}

	gs->enddisplay = FALSE;
	gs->playmode = Play_None;
	gs->usepasswds = usepasswds && !(gs->series.gsflags & GSF_IGNOREPASSWDS);
	gs->currentgame = -1;
	gs->melindacount = 0;

	if (defaultlevel) {
		n = findlevelinseries(&gs->series, defaultlevel, NULL);
		if (n >= 0) {
			gs->currentgame = n;
			if (gs->usepasswds &&
					!(gs->series.games[n].sgflags & SGF_HASPASSWD))
				changecurrentgame(gs, -1);
		}
	}
	if (gs->currentgame < 0) {
		gs->currentgame = 0;
		for (n = 0 ; n < gs->series.count ; ++n) {
			if (!issolved(gs, n)) {
				gs->currentgame = n;
				break;
			}
		}
	}

	return +1;
}

/* Get the list of available series and permit the user to select one
 * to play. If lastseries is not NULL, use that series as the default.
 * The return value is zero if nothing was selected, negative if an
 * error occurred, or positive otherwise.
 */
static int choosegame(gamespec *gs, char const *lastseries)
{
	seriesdata	s;

	if (!createserieslist(NULL, &s.list, &s.count, &s.table))
		return -1;
	return selectseriesandlevel(gs, &s, FALSE, lastseries, 0);
}

//DKS - modified
/*
 * Initialization functions.
 */

/* Set the four directories that the program uses (the series
 * directory, the series data directory, the resource directory, and
 * the save directory).  Any or all of the arguments can be NULL,
 * indicating that the default value should be used. The environment
 * variables TWORLDDIR, TWORLDSAVEDIR, and HOME can define the default
 * values. If any or all of these are unset, the program will use the
 * default values it was compiled with.
 */
static void initdirs(char const *series, char const *seriesdat,
		char const *res, char const *save)
{
	unsigned int	maxpath;
	char const	       *root = NULL;
	char const	       *dir;

	maxpath = getpathbufferlen();
	if (series && strlen(series) >= maxpath) {
		errmsg(NULL, "Data (-D) directory name is too long;"
				" using default value instead");
		series = NULL;
	}
	if (seriesdat && strlen(seriesdat) >= maxpath) {
		errmsg(NULL, "Configured data (-C) directory name is too long;"
				" using default value instead");
		seriesdat = NULL;
	}
	if (res && strlen(res) >= maxpath) {
		errmsg(NULL, "Resource (-R) directory name is too long;"
				" using default value instead");
		res = NULL;
	}
	if (save && strlen(save) >= maxpath) {
		errmsg(NULL, "Save (-S) directory name is too long;"
				" using default value instead");
		save = NULL;
	}
	if (!save && (dir = getenv("TWORLDSAVEDIR")) && *dir) {
		if (strlen(dir) < maxpath)
			save = dir;
		else
			warn("Value of environment variable TWORLDSAVEDIR is too long");
	}

	if (!res || !series || !seriesdat) {
		if ((dir = getenv("TWORLDDIR")) && *dir) {
			if (strlen(dir) < maxpath - 8)
				root = dir;
			else
				warn("Value of environment variable TWORLDDIR is too long");
		}
		if (!root) {
#if defined(ROOTDIR) && !defined(PLATFORM_GP2X) && !defined(PLATFORM_GCW)
			root = ROOTDIR;
#else
			root = ".";
#endif
		}
	}

	resdir = getpathbuffer();
	if (res)
		strcpy(resdir, res);
	else
		combinepath(resdir, root, "res");

	seriesdir = getpathbuffer();
// #ifdef PLATFORM_GCW
	dir = getenv("HOME");
	combinepath(seriesdir, dir, ".tworld/sets");
	printf("Sets path: %s\n", seriesdir);
// #else
// 	if (series)
// 		strcpy(seriesdir, series);
// 	else
// 		combinepath(seriesdir, root, "sets");
// #endif

	seriesdatdir = getpathbuffer();
// #ifdef PLATFORM_GCW
	if ((dir = getenv("HOME")) && *dir && strlen(dir) < maxpath - 8)
		combinepath(seriesdatdir, dir, ".tworld/data");
	else
		combinepath(seriesdatdir, root, "data");
// #else
// 	if (seriesdat)
// 		strcpy(seriesdatdir, seriesdat);
// 	else
// 		combinepath(seriesdatdir, root, "data");
// #endif

	savedir = getpathbuffer();
	if ((dir = getenv("HOME")) && *dir && strlen(dir) < maxpath - 8)
		combinepath(savedir, dir, ".tworld/save");
	else
		combinepath(savedir, root, "save");

	//DKS - added new persistent settings file (ideally $HOME/.tworld/port_cfg) handled in oshw-sdl/cfg.c
	portcfgfilename = getpathbuffer();
	if ((dir = getenv("HOME")) && *dir && strlen(dir) < maxpath - 8)
		combinepath(portcfgfilename, dir, ".tworld/port_cfg");
	else
		combinepath(portcfgfilename, root, "port_cfg");

	//DKS
	//	printdirectories();
}

/* Parse the command-line options and arguments, and initialize the
 * user-controlled options.
 */
static int initoptionswithcmdline(int argc, char *argv[], startupdata *start)
{
	cmdlineinfo	opts;
	char const *optresdir = NULL;
	char const *optseriesdir = NULL;
	char const *optseriesdatdir = NULL;
	char const *optsavedir = NULL;
	char	buf[256];
	int		listdirs, pedantic;
	int		ch, n;
	char       *p;

	start->filename = getpathbuffer();
	*start->filename = '\0';
	start->savefilename = NULL;
	start->levelnum = 0;
	start->listseries = FALSE;
	start->listscores = FALSE;
	start->listtimes = FALSE;
	start->batchverify = FALSE;
	listdirs = FALSE;
	pedantic = FALSE;
	mudsucking = 1;
	soundbufsize = 0;
	volumelevel = -1;

	initoptions(&opts, argc - 1, argv + 1, "abD:dFfHhL:lm:n:PpqR:rS:stVv");
	while ((ch = readoption(&opts)) >= 0) {
		switch (ch) {
			case 0:
				if (start->savefilename && start->levelnum) {
					fprintf(stderr, "too many arguments: %s\n", opts.val);
					printtable(stderr, yowzitch);
					return FALSE;
				}
				if (!start->levelnum && (n = (int)strtol(opts.val, &p, 10)) > 0
						&& *p == '\0') {
					start->levelnum = n;
				} else if (*start->filename) {
					start->savefilename = getpathbuffer();
					sprintf(start->savefilename, "%.*s", getpathbufferlen(),
							opts.val);
				} else {
					sprintf(start->filename, "%.*s", getpathbufferlen(), opts.val);
				}
				break;
			case 'D':	optseriesdatdir = opts.val;			break;
			case 'L':	optseriesdir = opts.val;			break;
			case 'R':	optresdir = opts.val;				break;
			case 'S':	optsavedir = opts.val;				break;
			case 'H':	showhistogram = !showhistogram;			break;
			case 'f':	noframeskip = !noframeskip;			break;
			case 'F':	fullscreen = !fullscreen;			break;
			case 'p':	usepasswds = !usepasswds;			break;
			case 'q':	silence = !silence;				break;
			case 'r':	readonly = !readonly;				break;
			case 'P':	pedantic = !pedantic;				break;
			case 'a':	++soundbufsize;					break;
			case 'd':	listdirs = TRUE;				break;
			case 'l':	start->listseries = TRUE;			break;
			case 's':	start->listscores = TRUE;			break;
			case 't':	start->listtimes = TRUE;			break;
			case 'b':	start->batchverify = TRUE;			break;
			case 'm':	mudsucking = atoi(opts.val);			break;
			case 'n':	volumelevel = atoi(opts.val);			break;
			case 'h':	printtable(stdout, yowzitch); 	   exit(EXIT_SUCCESS);
			case 'v':	puts(VERSION);		 	   exit(EXIT_SUCCESS);
			case 'V':	printtable(stdout, vourzhon); 	   exit(EXIT_SUCCESS);
			case ':':
							fprintf(stderr, "option requires an argument: -%c\n", opts.opt);
							printtable(stderr, yowzitch);
							return FALSE;
			case '?':
							fprintf(stderr, "unrecognized option: -%c\n", opts.opt);
							printtable(stderr, yowzitch);
							return FALSE;
			default:
							printtable(stderr, yowzitch);
							return FALSE;
		}
	}

	if (pedantic)
		setpedanticmode();

	initdirs(optseriesdir, optseriesdatdir, optresdir, optsavedir);
	if (listdirs) {
		printdirectories();
		exit(EXIT_SUCCESS);
	}

	if (*start->filename && !start->savefilename) {
		if (loadsolutionsetname(start->filename, buf) > 0) {
			start->savefilename = getpathbuffer();
			strcpy(start->savefilename, start->filename);
			strcpy(start->filename, buf);
		}
	}

	if (start->listscores || start->listtimes || start->batchverify
			|| start->levelnum)
		if (!*start->filename)
			strcpy(start->filename, "chips.dat");

	return TRUE;
}

//DKS - modified
// most of this moved to oshw-sdl/sdlmenu.c and run from there
/* Run the initialization routines of oshw and the resource module.
 */
static int initializesystem(void)
{
#ifdef NDEBUG
	mudsucking = 1;
#endif
	setmudsuckingfactor(mudsucking);
	if (!oshwinitialize(silence, soundbufsize, showhistogram, fullscreen))
		return FALSE;
	if (!initresources())
		return FALSE;
	setkeyboardrepeat(TRUE);
	if (volumelevel >= 0)
		setvolume(volumelevel, FALSE);
	return TRUE;
}

//DKS - modified
// I wasn't happy with how SDL isn't shutdown explicitly, but through atexit() 
// calls in its code.  I was getting segfaults when audio samples were freed.
// I'm explicity shutting things down here.
/* Time for everyone to clean up and go home.
 */
static void shutdownsystem(void)
{

	shutdowngamestate();

	//    if(SDL_JoystickOpened(0))
	//        SDL_JoystickClose(sdlg.joy);

	setsoundeffects(-1);
	setaudiosystem(FALSE);
	//DKS - this is done explicitly by setaudiosystem now
	//	 if (SDL_WasInit(SDL_INIT_AUDIO))
	//	SDL_QuitSubSystem(SDL_INIT_AUDIO);


	freeallresources();
	free(resdir);
	free(seriesdir);
	free(seriesdatdir);
	free(savedir);

	//DKS NEW
	free(portcfgfilename);
	if (sdlg.screen) 
		SDL_FreeSurface(sdlg.screen);
	if (sdlg.playbg)
		SDL_FreeSurface(sdlg.playbg);
	if (sdlg.menubg)
		SDL_FreeSurface(sdlg.menubg);
	if (sdlg.infobg)
		SDL_FreeSurface(sdlg.infobg);
	if (sdlg.oopsbg)
		SDL_FreeSurface(sdlg.oopsbg);
	if (sdlg.sprites)
		SDL_FreeSurface(sdlg.sprites);

#if defined(PLATFORM_GP2X) || defined(PLATFORM_GCW)
	// Close josytick if opened
	if (sdlg.joy && SDL_JoystickOpened(0))
		SDL_JoystickClose(sdlg.joy);
#endif

	SDL_Quit();
}

//DKS - modified
/* Determine what to play. A list of available series is drawn up; if
 * only one is found, it is selected automatically. Otherwise, if the
 * listseries option is TRUE, the available series are displayed on
 * stdout and the program exits. Otherwise, if listscores or listtimes
 * is TRUE, the scores or times for a single series is display on
 * stdout and the program exits. (These options need to be checked for
 * before initializing the graphics subsystem.) Otherwise, the
 * selectseriesandlevel() function handles the rest of the work. Note
 * that this function is only called during the initial startup; if
 * the user returns to the series list later on, the choosegame()
 * function is called instead.
 */
static int choosegameatstartup(gamespec *gs, startupdata const *start)
{
	seriesdata	series;
	tablespec	table;
	int		n;

	if (!createserieslist(start->filename,
				&series.list, &series.count, &series.table))
		return -1;

	free(start->filename);

	if (series.count <= 0) {
		errmsg(NULL, "no level sets found");
		return -1;
	}

	if (start->listseries) {
		printtable(stdout, &series.table);
		if (!series.count)
			puts("(no files)");
		return 0;
	}

	if (series.count == 1) {
		if (start->savefilename)
			series.list[0].savefilename = start->savefilename;
		if (!readseriesfile(series.list)) {
			errmsg(series.list[0].filebase, "cannot read level set");
			return -1;
		}
		if (start->batchverify) {
			n = batchverify(series.list, !silence && !start->listtimes
					&& !start->listscores);
			if (silence)
				exit(n > 100 ? 100 : n);
			else if (!start->listtimes && !start->listscores)
				return 0;
		}
		if (start->listscores) {
			if (!createscorelist(series.list, usepasswds, '0',
						NULL, NULL, &table))
				return -1;
			freeserieslist(series.list, series.count, &series.table);
			printtable(stdout, &table);
			freescorelist(NULL, &table);
			return 0;
		}
		if (start->listtimes) {
			if (!createtimelist(series.list,
						series.list->ruleset == Ruleset_MS ? 10 : 100,
						'0', NULL, NULL, &table))
				return -1;
			freeserieslist(series.list, series.count, &series.table);
			printtable(stdout, &table);
			freetimelist(NULL, &table);
			return 0;
		}
	}


	if (!initializesystem()) {
		errmsg(NULL, "cannot initialize program due to previous errors");
		return -1;
	}

	return selectseriesandlevel(gs, &series, TRUE, NULL, start->levelnum);
}



//DKS - new menu stuff

//This function is called once when the game first loads.  It looks in the ./data/
//folder for levelset files.  For each file found, the game automatically
//generates two appropriate .dac files in ./sets/ directory, one called
//FILEBASE-ms.dac and the other called FILEBASE-lynx.dac, where FILEBASE
//is the filename stripped of its extension.  If these files already exist,
// they are overwritten.  Someone may make their own custom .dac files and
// put them in ./sets/ but they cannot be named in the above manner and not
// get overwritten.
//
//The FILEBASE-ms.dac file is filled with this text:
// file=FILENAME
// ruleset=ms
//
// And the other is filled with this:
// file=FILENAME
// ruleset=lynx
//
//This allows the GP2X user to simply copy a new levelset into ./data/ and then
//run the game, getting to choose between rulesets by selecting one or the other
//.dac file to play.  Saves are saved independently.  I did this business because
//many user-created levels are MS-only and there was no easy way to change between
//emulated rulesets in the original tileworld without extensive use of a text editor. 

//quick n' dirty error function passed to glob() to allow continuation on errors
int parsedatadir_errfn(const char *pathname, int theerr) {
	return 0;
}

//DKS - I wrote this function from scratch to help tileworld on handhelds, this is the original and the
//	version that comes after (uncommented) is modified to work with GCW Zero which disallows writing to 
//	any folder but $HOME.
//returns 0 on failure (couldn't find ./data/) , 1 otherwise
int parsedatadir(void)
{
	printdirectories();
	glob_t 	result;
	int x, y, i, rc, fd;
	long long filesize;
	struct stat stat_tmp;
	char newfilename[2048];	// will store the filenames as we make new .dac files
	char tmpstr[2048];
	char globstr[2048];
	int this_is_chips_dat_file = 0;

	strcpy(globstr,seriesdatdir);
	strcat(globstr,"/*");
	rc = glob(globstr, 0, parsedatadir_errfn, &result);

	if (rc == GLOB_ABORTED) {
		errmsg(NULL, "read error in parsedatadir()");
		return 0;
	}

	if (!result.gl_pathc) {
		// nothing found
		return 0;
	}

	for (i = 0; i < result.gl_pathc; i++) {

		if (!stat(result.gl_pathv[i], &stat_tmp)) {
			if (S_ISREG(stat_tmp.st_mode)) {
				//This is a normal file, not a dir or anything

				// now we must make new .DAC file names to create

				//copy only the filename into newfilename, not any leading dirs
				//ex: "./data/CCLP2.dat"  becomes "CCLP2.dat"

				strcpy(tmpstr, skippathname(result.gl_pathv[i]));

				// Automatically detect filename of chips.dat (case insensitive match)
				//	so the "fixlynx" option will automatically be inserted
				// into that levelset's lynx .DAC file we generate.
				this_is_chips_dat_file = !strcasecmp(tmpstr,"chips.dat");

				// y will hold the cut-off point for our file name sans extension..
				// begin it all the way at the end
				y = strlen(tmpstr);
				newfilename[0] = '\0';

				for (x = strlen(tmpstr) - 1; x >= 0; --x) {
					if (tmpstr[x] == '.') {
						y = x;					
					}

				}

				strncat(newfilename, tmpstr, y);
				//write two .DAC config files into ./sets/ for this levelset file
				//first, the MS one
				strcpy(tmpstr, seriesdir);
				strcat(tmpstr, "/");
				strcat(tmpstr, newfilename);
				strcat(tmpstr, "-ms.dac");

				//check if file already exists
				fd = open(tmpstr, O_RDONLY);
				if (fd != -1) {
					// ok ,if the file already exists, get info on it so we can see how big it is
					y = fstat(fd, &stat_tmp);
				}

				if (y != -1) {
					filesize = (long long) stat_tmp.st_size;
				} else {
					// there was an error getting file info
					filesize = 0;
				}

				// if the file doesn't exist, or it's only 0 bytes long (invalid), create a fresh copy
				if ((fd == -1) || (filesize == 0)) {
					if (fd != -1) {
						close(fd);
					}
					// file didn't already exist, create it 
					fd = open(tmpstr, O_TRUNC | O_CREAT | O_WRONLY, 0644);
					if ( fd < 0) {
						errmsg(NULL, "write error in parsedatadir()");	
					} else {
						strcpy(tmpstr, "file=");
						strcat(tmpstr, skippathname(result.gl_pathv[i])); 
						strcat(tmpstr, "\n");
						write(fd, tmpstr, strlen(tmpstr));
						strcpy(tmpstr, "ruleset=ms\n"); 
						write(fd, tmpstr, strlen(tmpstr));
						close(fd);
					}
				} else {
					if (fd != -1) {
						close(fd);
					}
				}


				#ifndef MIYOO_MODE
				strcpy(tmpstr, seriesdir);
				strcat(tmpstr, "/");
				strcat(tmpstr, newfilename);
				strcat(tmpstr, "-lynx.dac");

				//check if file already exists
				fd = open(tmpstr, O_RDONLY);
				if (fd != -1) {
					// ok ,if the file already exists, get info on it so we can see how big it is
					y = fstat(fd, &stat_tmp);
				}

				if (y != -1) {
					filesize = (long long) stat_tmp.st_size;
				} else {
					// there was an error getting file info
					filesize = 0;
				}

				// if the file doesn't exist, or it's only 0 bytes long (invalid), create a fresh copy
				if ((fd == -1) || (filesize == 0)) {
					if (fd != -1) {
						close(fd);
					}
					fd = open(tmpstr, O_TRUNC | O_CREAT | O_WRONLY, 0644);
					if ( fd < 0) {
						errmsg(NULL, "write error in parsedatadir()");	
					} else {
						strcpy(tmpstr, "file=");
						strcat(tmpstr, skippathname(result.gl_pathv[i])); 
						strcat(tmpstr, "\n");
						write(fd, tmpstr, strlen(tmpstr));
						strcpy(tmpstr, "ruleset=lynx\n"); 
						write(fd, tmpstr, strlen(tmpstr));
						if (this_is_chips_dat_file)
						{
							strcpy(tmpstr, "fixlynx=y\n");
							write(fd, tmpstr, strlen(tmpstr));
						}
						close(fd);
					}
				} else {
					if (fd != -1) {
						close(fd);
					}
				}
				#endif

			}
		}
	}

	globfree(&result);

	return 1;
}	



//Called from main menu
//"gs" is the gamespec whose series member we will be assigning a new level series
//"series" is the series of levels to display in menu
//"defaultseries" is the filebase of the level series to display as initial selection
//"menusurface" is the main menu's surface we'll display underneath the new menu

// seriesmenu returns negative if error occurred or if user quit menu,
// otherwise it returns the new seriesindex
int seriesmenu(seriesdata *series , int seriesindex,
		SDL_Surface *menusurface)
{
	const int seriesmenuw = 290;
	const int seriesmenuh = 180;

	SDL_Surface *tmpsurface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCCOLORKEY,
			seriesmenuw, seriesmenuh, sdlg.realscreen->format->BitsPerPixel,
			sdlg.realscreen->format->Rmask, sdlg.realscreen->format->Gmask,
			sdlg.realscreen->format->Bmask, sdlg.realscreen->format-> Amask);

	//series menu's position on full screen
	SDL_Rect menurect;
	menurect.w = 290;
	menurect.h = 180;
	menurect.x = 15;
	menurect.y = 30;

	SDL_Rect tmprect;
	//paint dark background for series menu
	SDL_FillRect(tmpsurface, NULL,
			SDL_MapRGB(tmpsurface->format, 0, 0, 0));

	//draw a white border
	rectangleRGBA(tmpsurface, 0, 0,
			menurect.w-1, menurect.h-1,
			255, 255, 255, 255);

	// position of scrolling levelset names
	SDL_Rect textrect;
	textrect.w = 281;
	textrect.h = 168;
	textrect.x = 21;
	textrect.y = 36;

	SDL_BlitSurface(menusurface, NULL, sdlg.realscreen, NULL);

	if (series->count < 1) {
		errmsg(NULL, "no level sets found");
// #ifdef PLATFORM_GCW
// 		drawmultilinetext(tmpsurface, &textrect, "Sorry, no level packs were found. "
// 				"Please place level packs in /home/.tworld/data/ subfolder or reinstall."
// 				"(Press any button to continue)",
// 				-1, PT_UPDATERECT, sdlg.font_small, 1);
// #else
		drawmultilinetext(tmpsurface, &textrect, "Sorry, no level packs were found. "
				"Please place level packs in $HOME/.tworld/data/ folder."
				"(Press any button to continue)",
				-1, PT_UPDATERECT, sdlg.font_small, 1);

// #endif
		anykey();
		return -1;
	}

	//how many pixels high is a line of filenames?
	int textheight = SFont_TextHeight(sdlg.font_tiny) + 1;
	int numlines = (textrect.h - 2) / textheight; // number of lines to display in selector, must be odd
	if (numlines % 2)
		numlines--;

	int n;
	char tmpstr[2048]; // used to display lines in menu

	//marker is permanently in middle of lines and lines scroll while marker
	//  remains stationary.
	SDL_Rect markerrect;
	markerrect.w = textrect.w+1;
	markerrect.h = textheight+1;
	markerrect.x = 4;
	markerrect.y =  ((numlines/2) * textheight) + ((textheight+1) / 2) - 1;

	int command = CmdNone;
	int should_exit = 0;

	SDL_Surface *seriesmenusurface = SDL_DisplayFormat(tmpsurface);
	SDL_FreeSurface(tmpsurface);

	//paint lighter marker rectangle for current file
	SDL_FillRect(seriesmenusurface, &markerrect,
			SDL_MapRGB(seriesmenusurface->format, 30, 30, 30));

	while (!should_exit) {
		SDL_BlitSurface(seriesmenusurface, NULL, sdlg.realscreen, &menurect);
		tmprect.x = textrect.x;
		tmprect.y = textrect.y;
		tmprect.h = textheight;
		tmprect.w = textrect.w;

		for (n = -(numlines / 2); n <= (numlines / 2); ++n) {
			if ((seriesindex + n >= 0) && (seriesindex + n < series->count)) {
				// this is a valid entry into the list of levelsets
				char *seriesfname = series->list[seriesindex + n].name;
				int maxlength = textrect.w-3;
				if (strlen(seriesfname) > strlen("-ms.dac")) {
					char *cmpstr = "-ms.dac";
					int cmpstrlen = strlen(cmpstr);
					char appendstr[15];
					strcpy(appendstr, "-MS");
					if ( !strcmp(cmpstr, &seriesfname[(strlen(seriesfname) - cmpstrlen)])) {
						strupr(tmpstr, seriesfname);
						char *ins_point = &tmpstr[(strlen(tmpstr) - cmpstrlen)];
						strcpy(ins_point, appendstr);
						while ((SFont_TextWidth(sdlg.font_tiny, tmpstr) > maxlength) && (ins_point >= tmpstr))
						{
							// crop filename displayed to end neatly
							appendstr[0] = '~'; 
							strcpy(ins_point, appendstr);
							--ins_point;
						}
					}
				}

				if (strlen(seriesfname) > strlen("-lynx.dac")) {
					char *cmpstr = "-lynx.dac";
					int cmpstrlen = strlen(cmpstr);
					char appendstr[15];
					strcpy(appendstr, "-LYNX");

					if ( !strcmp(cmpstr, &seriesfname[(strlen(seriesfname) - cmpstrlen)])) {
						// we have have found "-ms.dac" at end of series filename, replace substring with "-MS\0"
						strupr(tmpstr, seriesfname);
						char *ins_point = &tmpstr[(strlen(tmpstr) - cmpstrlen)];
						strcpy(ins_point, appendstr);
						while ((SFont_TextWidth(sdlg.font_tiny, tmpstr) > maxlength) && (ins_point >= tmpstr))
						{
							// crop filename displayed to end neatly
							appendstr[0] = '~'; 
							strcpy(ins_point, appendstr);
							--ins_point;
						}
					}
				} 
				if (strlen(tmpstr) == 0) {
					//if name was blank, print a space
					tmpstr[0] = ' ';
					tmpstr[1] = '\0';
				}
			} else {
				//on lines that don't represent valid entries in series print space
				tmpstr[0] = ' ';
				tmpstr[1] = '\0';
			}
			drawtext(sdlg.realscreen, &tmprect, tmpstr,
					-1, PT_UPDATERECT, sdlg.font_tiny, 1);
		}

		SFont_WriteCenter(sdlg.realscreen, sdlg.font_tiny, 12, "Press SELECT to Cancel"); 
		SDL_Flip(sdlg.realscreen);
		// myflip();

		controlleddelay(10); // DKS - why be a CPU hog?
		command = input(FALSE);

		if (command == CmdQuitLevel) {
			should_exit = 1;
		} else if (command == CmdProceed) {
			should_exit = 1;
		} else if (command == CmdSouth) {
			if (seriesindex < (series->count - 1)) {
				++seriesindex;
				controlleddelay(180);
			}
		} else if (command == CmdNorth) {
			if (seriesindex > 0) {
				--seriesindex;
				controlleddelay(180);
			}
		} else if ((command == CmdPrev10) || (command == CmdPrev)) {
			seriesindex -= numlines;
			if (seriesindex < 0) {
				seriesindex = 0;
			}
			controlleddelay(180);
		} else if ((command == CmdNext10) || (command == CmdNext)) {
			seriesindex += numlines;
			if (seriesindex >= series->count) {
				seriesindex = series->count - 1;
			}
			controlleddelay(180);
		}
	}
	SDL_FreeSurface(seriesmenusurface);

	if (command == CmdProceed) {
		return seriesindex;
	} else {
		//user cancelled menu
		return -1;
	}
}

//DKS - new
//strupr takes an input string and converts it to all-upper-case. 
// it is user's responsibility to make sure buffer doesn't overrun.
// This is for printing LEDs on the screen, so any spaces are converted to 
// lower-case r's (which map to blank LEDs in the font)
// UPDATE: OK, I ended up not printing LEDs with this, I actually use this in 
// series.c to make the series comparison function work with upper and lower
// case filenames as if they were all uppercase.
void strupr(char *dest, char *src) {
	while (*src) {
		*dest = toupper(*src);

		++dest;
		++src;
	}
	*dest = '\0'; 
}

//my custom main menu - now the main game lopp
// return FALSE if error
int mainmenu(startupdata *start)
{
	seriesdata	series;
	//brought in from original tworld() main function
	gamespec	spec;
	//brought in from choosegameatstartup(), which we'll not be calling anymore
	int		n;
	SDL_Rect tmprect;	

	if (!parsedatadir()) {
		errmsg(NULL, "read error in parsedatadir()\n");
	}

	if (!initializesystem()) {
		errmsg(NULL, "cannot initialize program due to previous errors");
		return -1;
	}

	// contains the main menu's contents
	SDL_Surface *menusurface;
	menusurface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCCOLORKEY,
			sdlg.realscreen->w, sdlg.screen->h, sdlg.realscreen->format->BitsPerPixel,
			sdlg.realscreen->format->Rmask, sdlg.realscreen->format->Gmask,
			sdlg.realscreen->format->Bmask, sdlg.realscreen->format-> Amask);

	SDL_FillRect(menusurface, NULL, SDL_MapRGB(menusurface->format, 0, 0, 0));

	SFont_WriteCenter(menusurface, sdlg.font_tiny, 10, "-TILE WORLD FOR HANDHELDS-");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 30, "GNU GPL License, see COPYING file");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 55, "LOADING...");

	SFont_WriteCenter(menusurface, sdlg.font_tiny, 80, "-CREDITS-");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 90, "Original game & concept: ");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 100, "Chuck Sommerville");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 120, "Programming:");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 130, "Tile World: Brian Raiter");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 140, "Handheld port: Dan Silsby (Senor Quack)");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 160, "Music:");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 170, "Chaozz from gp32x.com (menu music),");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 180, "Am-Fm (under Creative Commons license)");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 200, "Fonts:");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 210, "Geekabyte by Jakob Fischer,");
	SFont_WriteCenter(menusurface, sdlg.font_tiny, 220, "Subatomic by Kevin Meinert");



	SDL_BlitSurface(menusurface, NULL, sdlg.realscreen, NULL);
	SDL_Flip(sdlg.realscreen);
	// myflip();

	//DKS - preload the game's tileset sprites, sounds, etc.
	loadgameresources(Ruleset_None);

	//DKS - new code to support remembering the last levelset file loaded, along with last played level:
	// (ONLY GETS CALLED ONCE AT LOAD OF GAME)
	set_port_cfg_defaults();
	read_port_cfg_file();
	if (!port_cfg_settings.music_enabled)
		disablemusic();

	SDL_Delay(500);



	// tmprect.x = 0;
	// tmprect.y = 55;
	// tmprect.h = 10;
	// tmprect.w = 320;
	// SDL_FillRect(menusurface, &tmprect, SDL_MapRGB(menusurface->format, 0, 0, 0));
	// // SFont_WriteCenter(menusurface, sdlg.font_tiny, 55, "PRESS ANY BUTTON");
	// SDL_BlitSurface(menusurface, NULL, sdlg.realscreen, NULL);	
	// SDL_Flip(sdlg.realscreen);
	// // myflip();

	playmenusong();
	setkeyboardinputmode(TRUE);
	// anykey();

	int seriesindex = 0; // stores the index into series where we can find the currently loaded series

	int quit_game = 0;

	if (!createserieslist(NULL,
				&series.list, &series.count, &series.table))
		return -1;

	free (start->filename);

	if (series.count <= 0) {
		errmsg(NULL, "no level sets found");
		return -1;
	}

	//    //Load the first level series the game found for now
	int tmpseriesctr;
	for (tmpseriesctr = 0; tmpseriesctr < series.count; tmpseriesctr++)
	{
		if (!strcmp(port_cfg_settings.last_levelset_played_filename, series.list[tmpseriesctr].name))
		{
			// We have located the series file matching the one played previously mentioned in port_cfg settings file 
			seriesindex=tmpseriesctr;
			break;
		} else {
			if (tmpseriesctr == (series.count-1)) {
				port_cfg_settings.last_level_in_levelset_played = 0;
			}

		}
	}

	getseriesfromlist(&(spec.series), series.list, seriesindex);
	freeserieslist(series.list, series.count, &(series.table));

	if (!readseriesfile(&(spec.series))) {
		errmsg(spec.series.filebase, "cannot read default data file");
	}

	int selection_made = 0;
	int cursor_pos = 0;

	// port_cfg_settings.music_enabled = 1;
	// enablemusic();

	while (!quit_game) {
		selection_made = -1;
		setkeyboardinputmode(TRUE);

		//main menu loop
		while (selection_made == -1) {
			SDL_BlitSurface(sdlg.menubg, NULL, menusurface, NULL);
			//            
#ifdef PLATFORM_GP2X
			SFont_WriteCenter(menusurface, sdlg.font_big, 30, "TileWorld2X");
#else
			SFont_WriteCenter(menusurface, sdlg.font_big, 30, "Tile World");
#endif

			char tmpstr[1024];
			strcpy(tmpstr, "Current level pack: ");
			strcat(tmpstr, spec.series.filebase);
			//if filename was huge, chop it nicely to a reasonable size		

			int cutoffctr = 150;		//sensible default for now, for 320 max width screens, but stop before something crazy happens
			while ((SFont_TextWidth(sdlg.font_tiny, tmpstr) > 310) && cutoffctr > 10)
			{
				tmpstr[cutoffctr-3] = '.';
				tmpstr[cutoffctr-2] = '.';
				tmpstr[cutoffctr-1] = '.';
				tmpstr[cutoffctr] = '\0';
				cutoffctr--;
			}

			SFont_Write(menusurface, sdlg.font_tiny, 3, 220, tmpstr);
			int num_total = spec.series.count;
			int num_solved = 0;
			int level_ctr;
			for (level_ctr = 0 ; level_ctr < spec.series.count ; ++level_ctr) {
				if (issolved(&spec, level_ctr))
					++num_solved;
			}

			sprintf(tmpstr, " Levels solved: %d / %d", num_solved, num_total);
			SFont_Write(menusurface, sdlg.font_tiny, 3, 230, tmpstr);

			int basescore, timescore;
			long totalscore;
			getscoresforlevel(&spec.series, spec.currentgame,
					&basescore, &timescore, &totalscore);
			sprintf(tmpstr, "Total score: %ld", totalscore);
			SFont_Write(menusurface, sdlg.font_tiny, (319 - SFont_TextWidth(sdlg.font_tiny, tmpstr) - 4), 230, tmpstr);

#define MMENU_LINEX	50
#define MMENU_CURSORX (MMENU_LINEX - 15)
#define MMENU_LINE1Y 80
#define MMENU_LINE2Y (MMENU_LINE1Y + 25)
#define MMENU_LINE3Y (MMENU_LINE2Y + 25)
#define MMENU_LINE4Y (MMENU_LINE3Y + 25)
#define MMENU_LINE5Y (MMENU_LINE4Y + 25)
			SFont_Write(menusurface, sdlg.font_small, MMENU_LINEX, MMENU_LINE1Y, "Play Level Pack");
			if (cursor_pos == 0)
				SFont_Write(menusurface, sdlg.font_small, MMENU_CURSORX, MMENU_LINE1Y, ">");
			SFont_Write(menusurface, sdlg.font_small, MMENU_LINEX, MMENU_LINE2Y, "Select New Level Pack");
			if (cursor_pos == 1)
				SFont_Write(menusurface, sdlg.font_small, MMENU_CURSORX, MMENU_LINE2Y, ">");
			SFont_Write(menusurface, sdlg.font_small, MMENU_LINEX, MMENU_LINE3Y, 
					ismusicenabled() ? "Disable Music" : "Enable Music");
			if (cursor_pos == 2)
				SFont_Write(menusurface, sdlg.font_small, MMENU_CURSORX, MMENU_LINE3Y, ">");
			SFont_Write(menusurface, sdlg.font_small, MMENU_LINEX, MMENU_LINE4Y, 
					port_cfg_settings.analog_enabled ? "Disable analog stick" : "Enable analog stick");
			if (cursor_pos == 3) 
				SFont_Write(menusurface, sdlg.font_small, MMENU_CURSORX, MMENU_LINE4Y, ">");
			SFont_Write(menusurface, sdlg.font_small, MMENU_LINEX, MMENU_LINE5Y, "Quit");
			if (cursor_pos == 4) {
				SFont_Write(menusurface, sdlg.font_small, MMENU_CURSORX, MMENU_LINE5Y, ">");
			}

			//cleardisplay();
			SDL_BlitSurface(menusurface, NULL, sdlg.screen, NULL);
			SDL_BlitSurface(sdlg.screen, NULL, sdlg.realscreen, NULL);            
			SDL_Flip(sdlg.realscreen);
			// myflip();

			switch (input(FALSE)) {
				case CmdSouth:
					// controlleddelay(250);
					if (cursor_pos < 4) {
						cursor_pos++;
					}
					break;
				case CmdNorth:
					// controlleddelay(250);
					if (cursor_pos > 0) {
						cursor_pos--;
					}
					break;
				case CmdProceed:
					selection_made = cursor_pos;
					// controlleddelay(140);
					break;
				default:
					break;
			}
			controlleddelay(100);
		}

		switch (selection_made) {
			case 0:
				// user wants to play selected levelpack
				spec.enddisplay = FALSE;
				spec.playmode = Play_None;
				spec.usepasswds = FALSE;
				spec.melindacount = 0;

				if (spec.series.count < 1) {
					errmsg(spec.series.filebase, "no levels found in data file");
					break;
				}

				if ((port_cfg_settings.last_level_in_levelset_played < spec.series.count) &&
						(port_cfg_settings.last_level_in_levelset_played > 0))
				{
					spec.currentgame = port_cfg_settings.last_level_in_levelset_played;
					printf("Loading last level played in levelset (in ~/.tworld/port_cfg): %d\n", port_cfg_settings.last_level_in_levelset_played+1);
				}
				else
				{
					printf("No valid saved last-played level for levelset (in ~/.tworld/port_cfg): %d\n", port_cfg_settings.last_level_in_levelset_played+1);
					// if we can't find a levelset played previously, load the one at top of list and
					//		go to the first unsolved level
					spec.currentgame = 0;
					for (n = 0 ; n < spec.series.count ; ++n) {
						if (!issolved(&spec, n)) {
							spec.currentgame = n;
							break;
						}
					}
					printf("Loaded first unsolved level in levelset instead.\n");
				}
				setkeyboardinputmode(FALSE);

				SDL_FreeSurface(menusurface); 

				quittomainmenu = 0;  // playgame will set this to 1 if user wants to exit.
				displaylevelselect = 1; 	//we want runcurrentlevel to show level selection initially

				//start the game
				while (runcurrentlevel(&spec));
				port_cfg_settings.last_level_in_levelset_played = spec.currentgame;
				strcpy(port_cfg_settings.last_levelset_played_filename, spec.series.filebase);

				quittomainmenu = 0;			// re-initialize both
				displaylevelselect = 1; 	

				//OK, we're back to the main menu
				playmenusong();

				//recreate the menu surface before looping    						
				menusurface = SDL_CreateRGBSurface(SDL_HWSURFACE | SDL_SRCCOLORKEY,
						sdlg.realscreen->w, sdlg.screen->h, sdlg.realscreen->format->BitsPerPixel,
						sdlg.realscreen->format->Rmask, sdlg.realscreen->format->Gmask,
						sdlg.realscreen->format->Bmask, sdlg.realscreen->format-> Amask);
				break;
			case 1:
				//User wants to select new levelpack
				if (!createserieslist(NULL,
							&series.list, &series.count, &series.table))
					return -1;

				int newseriesindex = seriesmenu(&series, seriesindex, menusurface);

				if ( newseriesindex >= 0 ) {
					// new level series was selected
					// load new level series into current gamespec
					seriesindex=newseriesindex;
					freeseriesdata(&(spec.series));
					getseriesfromlist(&(spec.series), series.list, newseriesindex);
					freeserieslist(series.list, series.count, &(series.table));
					strcpy(port_cfg_settings.last_levelset_played_filename, spec.series.filebase);
					port_cfg_settings.last_level_in_levelset_played = 0;


					if (!readseriesfile(&(spec.series))) {
						errmsg(spec.series.filebase, "cannot read data file");
						// level pack was bad, reset to first in list
					}

				}
				cursor_pos = 0;
				break;
			case 2:
				//user wants to enable/disable music
				if (ismusicenabled()) {
					disablemusic();
					port_cfg_settings.music_enabled = 0;
				} else {
					enablemusic();
					port_cfg_settings.music_enabled = 1;
				}
				break;
			case 3:
#ifdef PLATFORM_GCW
				//user wants to enabled/disable analog input
				port_cfg_settings.analog_enabled = !port_cfg_settings.analog_enabled;
#endif
				//DKS-NOTE: on other platforms without analog input, this part of the menu can be filled in
				//		later on with some other trivial option in its place.. too busy to make this nicer now.
				break;
			case 4:
				//user wants to quit
				quit_game = TRUE;
				write_port_cfg_file();
				break;
			default:
				break;
		}
	}
	SDL_FreeSurface(menusurface);
	freeseriesdata(&(spec.series));
	return TRUE;

}

//DKS - modified
int tworld(int argc, char *argv[])
{
	//DKS - added for GCW-Zero 
// #ifdef PLATFORM_GCW
	// Since GCW uses a read-only squashfs .OPK file for running applications, this makes sense:
	char hdir[512];// = getenv("HOME");
	snprintf(hdir, sizeof(hdir), "%s/.tworld", getenv("HOME")); mkdir(hdir, 0777);
	snprintf(hdir, sizeof(hdir), "%s/.tworld/save", getenv("HOME")); mkdir(hdir, 0777);
	snprintf(hdir, sizeof(hdir), "%s/.tworld/data", getenv("HOME")); mkdir(hdir, 0777);
	snprintf(hdir, sizeof(hdir), "%s/.tworld/sets", getenv("HOME")); mkdir(hdir, 0777);
	snprintf(hdir, sizeof(hdir), "[ ! -f %s/.tworld/data/CCLP1.ccx ] && cp data/* %s/.tworld/data/", getenv("HOME"), getenv("HOME"));
	system(hdir);
// #endif

	startupdata	start;
	int		f = 0;
	if (!initoptionswithcmdline(argc, argv, &start))
		return EXIT_FAILURE;
	f = mainmenu(&start);
	shutdownsystem();
	//dks added:
	sync();
	//return f == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
	return f >= 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
