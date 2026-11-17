/*
 * Various installation dependent routines
 *
 * @(#)mach_dep.c	4.37 (Berkeley) 05/23/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * The various tuneable defines are:
 *
 *	SCOREFILE	Where/if the score file should live.
 *	NUMSCORES	Number of scores in the score file (default 10).
 *	NUMNAME		String version of NUMSCORES (first character
 *			should be capitalized) (default "Ten").
 *	MAXLOAD		What (if any) the maximum load average should be
 *			when people are playing.  Since it is divided
 *			by 10, to specify a load limit of 4.0, MAXLOAD
 *			should be "40".	 If defined, then
 *      LOADAV		Should it use it's own routine to get
 *		        the load average?
 *      NAMELIST	If so, where does the system namelist
 *		        hide?
 *	MAXUSERS	What (if any) the maximum user count should be
 *	                when people are playing.  If defined, then
 *      UCOUNT		Should it use it's own routine to count
 *		        users?
 *      UTMP		If so, where does the user list hide?
 *	CHECKTIME	How often/if it should check during the game
 *			for high load average.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>       /* _open, _close and other MinGW CRT file functions */
#else
#include <unistd.h>
#endif
#include <curses.h>
#include "extern.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#define JROGUE_SCORE_STAT(path, st) jrogue_windows_stat_utf8((path), (st))
#else
#define JROGUE_SCORE_STAT(path, st) stat((path), (st))
#endif



# ifndef NUMSCORES
#	define	NUMSCORES	10
#	define	NUMNAME		JSTR("テン", "Ten")
# endif

int numscores = NUMSCORES;
const char *Numname = "Ten";


#ifdef CHECKTIME
static int num_checks = 0;		/* times we've gone over in checkout() */
#endif /* CHECKTIME */

/*
 * init_check:
 *	Check out too see if it is proper to play the game now
 */

void
init_check(void)
{
#if defined(MAXLOAD) || defined(MAXUSERS)
    if (too_much())
    {
	printf("Sorry, %s, but the system is too loaded now.\n", whoami);
	printf("Try again later.  Meanwhile, why not enjoy a%s %s?\n",
	    vowelstr(fruit), fruit);
	if (author())
	    printf("However, since you're a good guy, it's up to you\n");
	else
	    exit(1);
    }
#endif
}

/*
 * open_score:
 *	Open up the score file for future use
 */

void
open_score(void)
{
#ifdef SCOREFILE
#ifdef _WIN32
    /* SCOREFILE may be a long UTF-8 pathname. Do not truncate to PATH_MAX. */
    const char *scorefile = SCOREFILE;
#else
    char scorefile_buf[PATH_MAX];
    char *scorefile = scorefile_buf;
    jrogue_strlcpy(scorefile_buf, SCOREFILE, sizeof scorefile_buf);
#ifdef __CYGWIN__
    cygwin_conv_to_posix_path(scorefile_buf, scorefile_buf);
#endif
#endif

//    numscores = NUMSCORES;

     /* 
      * We drop setgid privileges after opening the score file, so subsequent 
      * open()'s will fail.  Just reuse the earlier filehandle. 
      */

    if (scoreboard != NULL) { 
        rewind(scoreboard); 
        return; 
    } 

#ifdef _WIN32
    scoreboard = jrogue_windows_fopen_utf8(scorefile, L"r+b");
#else
    scoreboard = fopen(scorefile, "r+b");
#endif

    if ((scoreboard == NULL) && (errno == ENOENT))
    {
#ifdef _WIN32
        int created = FALSE;
        /* The first writer must not truncate a scorefile just created
         * by another running instance. */
        scoreboard = jrogue_windows_create_exclusive_utf8(scorefile, 0666, L"w+b");
        if (scoreboard != NULL)
            created = TRUE;
        else if (errno == EEXIST)
            scoreboard = jrogue_windows_fopen_utf8(scorefile, L"r+b");
        if (created)
            md_chmod(scorefile, 0664);
#else
        /* Match Windows' exclusive-first-creator rule.  Two games that
         * start together must not both truncate the new score file.
         * Reopen the winner's file if another process created it first. */
        int fd = open(scorefile, O_RDWR | O_CREAT | O_EXCL, 0666);
        if (fd >= 0)
        {
            scoreboard = fdopen(fd, "r+b");
            if (scoreboard == NULL)
            {
                int saved_errno = errno;
                close(fd);
                (void)unlink(scorefile); /* Only our new file. */
                errno = saved_errno;
            }
            else
                md_chmod(scorefile, 0664);
        }
        else if (errno == EEXIST)
            scoreboard = fopen(scorefile, "r+b");
#endif
    }

    if (scoreboard == NULL) { 
         fprintf(stderr, "Could not open %s for writing: %s\n", scorefile, strerror(errno)); 
         fflush(stderr); 
    } 
#else
    scoreboard = NULL;
#endif
}

/*
 * getltchars:
 *	Get the local tty chars for later use
 */

void
getltchars(void)
{
    got_ltc = TRUE;
    orig_dsusp = md_dsuspchar();
    md_setdsuspchar( md_suspchar() );
}

/*
 * setup:
 *	Get starting setup for all games
 */

void
setup(void)
{
#ifdef DUMP
    md_onsignal_autosave();
#else
    md_onsignal_default();
#endif

#ifdef CHECKTIME
    md_start_checkout_timer(CHECKTIME*60);
    num_checks = 0;
#endif

    raw();				/* Raw mode */
    noecho();				/* Echo off */
    keypad(stdscr,1);
    getltchars();			/* get the local tty chars */
}

/* 
 * resetltchars: 
 *      Reset the local tty chars to original values. 
 */ 
void 
resetltchars(void) 
{ 
    if (got_ltc) {
        md_setdsuspchar(orig_dsusp);
    } 
} 
  
/* 
 * playltchars: 
 *      Set local tty chars to the values we use when playing. 
 */ 
void 
playltchars(void) 
{ 
    if (got_ltc) { 
        md_setdsuspchar( md_suspchar() );
    } 
} 

/*
 * start_score:
 *	Start the scoring sequence
 */

void
start_score(void)
{
#ifdef CHECKTIME
    md_stop_checkout_timer();
#endif
}

/* 	 	 
 * is_symlink: 	 	 
 *      See if the file has a symbolic link 	 	 
  */ 	 	 
int	 	 
is_symlink(char *sp) 	 	 
{ 	 	 
#if defined(S_IFLNK) && !defined(_WIN32) 	 	 
    struct stat sbuf2; 	 	 
 	 	 
    if (lstat(sp, &sbuf2) < 0) 	 	 
        return FALSE; 	 	 
    else 	 	 
        return ((sbuf2.st_mode & S_IFMT) != S_IFREG); 	 	 
#else
	(void)sp;
    return FALSE; 	 	 
#endif 
} 

#if defined(MAXLOAD) || defined(MAXUSERS)
/*
 * too_much:
 *	See if the system is being used too much for this game
 */
int
too_much(void)
{
#ifdef MAXLOAD
    double avec[3];
#else
    int cnt;
#endif

#ifdef MAXLOAD
    md_loadav(avec);
    if (avec[1] > (MAXLOAD / 10.0))
	return TRUE;
#endif
#ifdef MAXUSERS
    if (ucount() > MAXUSERS)
	return TRUE;
#endif
    return FALSE;
}

/*
 * author:
 *	See if a user is an author of the program
 */
int
author(void)
{
#ifdef MASTER
    if (wizard)
	return TRUE;
#endif
    switch (md_getuid())
    {
	case -1:
	    return TRUE;
	default:
	    return FALSE;
    }
}
#endif

#ifdef CHECKTIME
/*
 * checkout:
 *	Check each CHECKTIME seconds to see if the load is too high
 */

checkout(int sig)
{
    char *msgs[] = {
	"The load is too high to be playing.  Please leave in %0.1f minutes",
	"Please save your game.  You have %0.1f minutes",
	"Last warning.  You have %0.1f minutes to leave",
    };
    int checktime;

    if (too_much())
    {
	if (author())
	{
	    num_checks = 1;
	    chmsg("The load is rather high, O exaulted one");
	}
	else if (num_checks++ == 3)
	    fatal("Sorry.  You took too long.  You are dead\n");
	checktime = (CHECKTIME * 60) / num_checks;
	chmsg(msgs[num_checks - 1], ((double) checktime / 60.0));
    }
    else
    {
	if (num_checks)
	{
	    num_checks = 0;
	    chmsg("The load has dropped back down.  You have a reprieve");
	}
	checktime = (CHECKTIME * 60);
    }

	md_start_checkout_timer(checktime);
}

/*
 * chmsg:
 *	checkout()'s version of msg.  If we are in the middle of a
 *	shell, do a printf instead of a msg to a the refresh.
 */
/* VARARGS1 */

chmsg(char *fmt, int arg)
{
    if (!in_shell)
	msg(fmt, arg);
    else
    {
	printf(fmt, arg);
	putchar('\n');
	fflush(stdout);
    }
}
#endif

#ifdef UCOUNT
/*
 * ucount:
 *	count number of users on the system
 */
#include <utmp.h>

struct utmp buf;

int
ucount(void)
{
    struct utmp *up;
    FILE *utmp;
    int count;

    if ((utmp = fopen(UTMP, "r")) == NULL)
	return 0;

    up = &buf;
    count = 0;

    while (fread(up, 1, sizeof (*up), utmp) > 0)
	if (buf.ut_name[0] != '\0')
	    count++;
    fclose(utmp);
    return count;
}
#endif

/*
 * lock_sc:
 *	lock the score file.  If it takes too long, ask the user if
 *	they care to wait.  Return TRUE if the lock is successful.
 */
static FILE *lfd = NULL;
static const char *score_lockfile = NULL;

static FILE *
try_lock_file(const char *lockfile)
{
#ifndef _WIN32
    FILE *fp;
    int fd;
#endif

#ifdef _WIN32
    /* UTF-8 lock path, same O_EXCL protocol as macOS/Linux. */
    return jrogue_windows_create_exclusive_utf8(lockfile, 0666, L"w+");
#else
    fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0)
        return NULL;
    fp = fdopen(fd, "w");
    if (fp == NULL)
    {
        close(fd);
        md_unlink(lockfile);
    }
    return fp;
#endif
}

int
lock_sc(void)
{
#if defined(SCOREFILE) && defined(LOCKFILE)
    int cnt;
    struct stat sbuf;
    const char *lockfile = LOCKFILE;

    if (lfd != NULL)
	return TRUE;

over:
    if ((lfd = try_lock_file(lockfile)) != NULL)
    {
	score_lockfile = lockfile;
	return TRUE;
    }
    for (cnt = 0; cnt < 5; cnt++)
    {
	md_sleep(1);
	if ((lfd = try_lock_file(lockfile)) != NULL)
	{
	    score_lockfile = lockfile;
	    return TRUE;
	}
    }
    if (JROGUE_SCORE_STAT(lockfile, &sbuf) < 0)
    {
	if ((lfd = try_lock_file(lockfile)) != NULL)
	{
	    score_lockfile = lockfile;
	    return TRUE;
	}
	return FALSE;
    }
    if (time(NULL) - sbuf.st_mtime > 10)
    {
	if (md_unlink(lockfile) < 0)
	    return FALSE;
	goto over;
    }
    else
    {
	printf("The score file is very busy.  Do you want to wait longer\n");
	printf("for it to become free so your score can get posted?\n");
	printf("If so, type \"y\"\n");
	(void) fgets(prbuf, MAXSTR, stdin);
	if (prbuf[0] == 'y')
	    for (;;)
	    {
		if ((lfd = try_lock_file(lockfile)) != NULL)
		{
		    score_lockfile = lockfile;
		    return TRUE;
		}
		if (JROGUE_SCORE_STAT(lockfile, &sbuf) < 0)
		{
		    if ((lfd = try_lock_file(lockfile)) != NULL)
		    {
			score_lockfile = lockfile;
			return TRUE;
		    }
		    return FALSE;
		}
		if (time(NULL) - sbuf.st_mtime > 10)
		{
		    if (md_unlink(lockfile) < 0)
			return FALSE;
		}
		md_sleep(1);
	    }
	else
	    return FALSE;
    }
#else
    return TRUE;
#endif
}

/*
 * unlock_sc:
 *	Unlock the score file
 */

void
unlock_sc(void)
{
#if defined(SCOREFILE) && defined(LOCKFILE)
    if (lfd != NULL)
        fclose(lfd);
    lfd = NULL;
    if (score_lockfile != NULL)
        md_unlink(score_lockfile);
    score_lockfile = NULL;
#endif
}

/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */

void
flush_type(void)
{
    flushinp();
}
