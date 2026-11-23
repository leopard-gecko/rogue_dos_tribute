/*
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 *
 * @(#)main.c	4.22 (Berkeley) 02/05/99
 */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
#include <conio.h> /* MinGW getch declaration before PDCurses macro */
#include "windows/rogue_windows_console.h"
#include "windows/rogue_windows.h"
#endif
#include <curses.h>
#include <locale.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"


#define START_CURTAIN_PAIR_FRAME 52
#define START_CURTAIN_PAIR_FILL 53
#define START_CURTAIN_DELAY_MS 35

/* The DOS original redirects dungeon drawing away from the visible screen
 * while the startup curtain is down.  curses has one logical stdscr, so
 * endmsg() must not flush it until the curtain has been restored there.
 * Keep message composition and the dungeon snapshot unchanged. */
static int start_curtain_covers_screen = FALSE;

int
jrogue_start_curtain_is_covered(void)
{
    return start_curtain_covers_screen;
}

static int
jrogue_start_curtain_enabled(void)
{
    return _LINES >= 20 && NUMCOLS >= 40;
}

/* The curtain uses one UTF-8 glyph table and one draw path on every OS.
 * The shared drawing helper translates a scalar only at the curses boundary. */
enum start_curtain_glyph {
    START_CURTAIN_TL, START_CURTAIN_TR, START_CURTAIN_BL, START_CURTAIN_BR,
    START_CURTAIN_H, START_CURTAIN_V, START_CURTAIN_FILL,
    START_CURTAIN_GLYPH_COUNT
};

static const char *const start_curtain_chars_extended[START_CURTAIN_GLYPH_COUNT] = {
    "┌", "┐", "└", "┘", "─", "│", "▒"
};

static const char *const start_curtain_chars_basic[START_CURTAIN_GLYPH_COUNT] = {
    "+", "+", "+", "+", "-", "|", "#"
};

static const char *const *
jrogue_start_curtain_get_chars(void)
{
    /*
     * Use the UTF-8 curtain/tombstone glyphs whenever the terminal can
     * display multibyte characters, even if extchars is disabled.
     * noextchars only affects the map glyph set.
     */
    if (MB_CUR_MAX > 1)
        return start_curtain_chars_extended;
    return start_curtain_chars_basic;
}

static void
jrogue_start_curtain_set_attr(int pair)
{
    if (color_mode2 != FALSE)
        attrset(COLOR_PAIR(pair) | A_NORMAL);
    else
        attrset(A_NORMAL);
}

static void
jrogue_start_curtain_normal_attr(void)
{
    if (color_mode2 != FALSE)
        jrogue_set_normal_attr();
    else
        attrset(A_NORMAL);
}

static void
jrogue_start_curtain_put(const char *const *chars,
                         enum start_curtain_glyph glyph)
{
    (void)jrogue_add_utf8_glyph(chars[glyph]);
}

static void
jrogue_start_curtain_draw_frame(void)
{
    int row, col;

    const char *const *cc;

    cc = jrogue_start_curtain_get_chars();
    jrogue_start_curtain_set_attr(START_CURTAIN_PAIR_FRAME);
    move(0, 0);
    jrogue_start_curtain_put(cc, START_CURTAIN_TL);
    for (col = 1; col < NUMCOLS - 1; col++)
        jrogue_start_curtain_put(cc, START_CURTAIN_H);
    jrogue_start_curtain_put(cc, START_CURTAIN_TR);

    for (row = 1; row < _LINES - 1; row++) {
        move(row, 0);
        jrogue_start_curtain_put(cc, START_CURTAIN_V);
        move(row, NUMCOLS - 1);
        jrogue_start_curtain_put(cc, START_CURTAIN_V);
    }

    move(_LINES - 1, 0);
    jrogue_start_curtain_put(cc, START_CURTAIN_BL);
    for (col = 1; col < NUMCOLS - 1; col++)
        jrogue_start_curtain_put(cc, START_CURTAIN_H);
    jrogue_start_curtain_put(cc, START_CURTAIN_BR);
    jrogue_start_curtain_normal_attr();
}

static void
jrogue_start_curtain_fill(int animate)
{
    int row, col;
    const char *const *cc;

    cc = jrogue_start_curtain_get_chars();
    jrogue_start_curtain_draw_frame();
    if (animate) {
        (void)jrogue_curtain_update(FALSE);
        msleep(START_CURTAIN_DELAY_MS);
    }

    for (row = 1; row < _LINES - 1; row++) {
        jrogue_start_curtain_set_attr(START_CURTAIN_PAIR_FILL);
        move(row, 1);
        for (col = 1; col < NUMCOLS - 1; col++)
            jrogue_start_curtain_put(cc, START_CURTAIN_FILL);
        jrogue_start_curtain_normal_attr();
        if (animate) {
            (void)jrogue_curtain_update(FALSE);
            msleep(START_CURTAIN_DELAY_MS);
        }
    }
    jrogue_start_curtain_normal_attr();
}

static void
jrogue_start_drop_curtain(void)
{
    jrogue_start_curtain_fill(TRUE);
}

static void
jrogue_start_restore_curtain_virtual(void)
{
    jrogue_start_curtain_fill(FALSE);
}

static void
jrogue_start_raise_curtain(WINDOW *dungeon)
{
    int row;

    if (dungeon == NULL)
        return;

    for (row = _LINES - 1; row >= 0; row--) {
        copywin(dungeon, stdscr, row, 0, row, 0, row, NUMCOLS - 1, FALSE);
        wnoutrefresh(stdscr);
        (void)jrogue_curtain_update(TRUE);
        msleep(START_CURTAIN_DELAY_MS);
    }
    jrogue_start_curtain_normal_attr();
}

/*
 * main:
 *	The main program, of course
 */
int
main(int argc, char **argv)
{
    char *env;
    time_t lowtime;

    int start_curtain = FALSE;
    WINDOW *start_snapshot = NULL;
#ifdef _WIN32
    char *windows_save_path = NULL;
    int windows_argument_index = 1;
#endif

    jrogue_init_locale();
#ifdef _WIN32
    /* A Windows UTF-16 environment value cannot safely be read via the
     * locale-dependent narrow getenv() when it contains Japanese text. */
    env = jrogue_rogueopts_utf8();
#else
    env = getenv("ROGUEOPTS");
#endif
    /* Language detection and option parsing must use the SAME UTF-8 value. */
    jrogue_detect_language(env);
    jrogue_apply_language();
#ifdef __CYGWIN__
    setenv("TERMINFO", "terminfo", 1);
#endif

#ifdef _WIN32
    jrogue_windows_prepare_console();
#endif
    md_init();
#ifdef _WIN32
    jrogue_windows_resize_direct_launch();
#endif

#ifdef MASTER
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, md_crypt(md_getpass(JSTR("魔法使いの合言葉は？", "Wizard password?")), "mT")) == 0)
	{
	    wizard = TRUE;
	    player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
#ifdef _WIN32
            ++windows_argument_index;
#endif
	}

#endif

    /*
     * get home and options from environment
     */

#ifdef __CYGWIN__
    jrogue_strlcpy(file_name, DEFAULT_SAVEFILE, MAXSTR);
#else
    jrogue_strlcpy(home, md_gethomedir(), MAXSTR);

	if (strlen(home) > MAXSTR - strlen(DEFAULT_SAVEFILE) - 1)
		*home = 0;
//    strcpy(file_name, home);
//    strcat(file_name, DEFAULT_SAVEFILE);
    jrogue_strlcpy(file_name, DEFAULT_SAVEFILE, MAXSTR);

#endif

    if (env != NULL)
	parse_opts(env);
    jrogue_apply_language();
    if (env == NULL || whoami[0] == '\0')
        strucpy(whoami, md_getusername(), strlen(md_getusername()));
#ifdef _WIN32
    free(env); /* parse_opts() has already copied all string options. */
#endif
    lowtime = time(NULL);
    dnum = (unsigned int) lowtime + md_getpid();
#ifdef MASTER
    if (wizard && getenv("SEED") != NULL)
    {
	dnum = atoi(getenv("SEED"));
	noscore = 1;
    }
#endif
    seed = dnum;

    open_score();

	/* 
     * Drop setuid/setgid after opening the scoreboard file. 
     */ 

    md_normaluser();

    /*
     * check for print-score option
     */

    if (argc == 2)
    {
	if (strcmp(argv[1], "-s") == 0)
	{
	    noscore = TRUE;
	    score(0, -1, 0);
#ifdef _WIN32
            jrogue_windows_wait_for_direct_exit();
#endif
	    exit(0);
	}
    }
#ifdef _WIN32
    /* Explorer double-click with no argument: offer a menu only when the
     * resolved default save exists.  Explicit -r/-s and dragged filenames
     * bypass the menu, and shell launches never display it. */
    if (argc == 1)
        windows_save_path = jrogue_windows_launch_save(file_name);
    else if (argc == 2)
        windows_save_path = jrogue_windows_argument_utf8(windows_argument_index);
#endif
    init_check();			/* check for legal startup */
#ifdef _WIN32
    if (windows_save_path != NULL)
    {
        if (strlen(windows_save_path) >= MAXSTR)
        {
            fputs(JSTR("セーブファイルのパスが長すぎます。\n",
                       "Save file path is too long.\n"), stderr);
            free(windows_save_path);
            jrogue_windows_wait_for_direct_exit();
            exit(1);
        }
        if (!restore(windows_save_path))
        {
            free(windows_save_path);
            /* A failed restore may be pre-curses. Do not call my_exit(). */
            jrogue_windows_wait_for_direct_exit();
            exit(1);
        }
        free(windows_save_path); /* restore() enters playit() on success */
    }
    else if (argc == 2)
    {
        fputs(JSTR("再開するファイル名を読み取れませんでした。\n",
                   "Could not decode saved-game filename.\n"), stderr);
        jrogue_windows_wait_for_direct_exit();
        exit(1);
    }
#else
    if (argc == 2)
    {
        if (!restore(argv[1]))
            exit(1); /* early restore failures have no curses screen */
    }
#endif
    initscr();
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(0);
    if (jrogue_windows_sync_direct_launch_screen() == ERR) {
        jrogue_endwin();
        fputs(JSTR("ゲーム画面を80×24に調整できませんでした。\n",
                   "Could not adjust the game screen to 80x24.\n"), stderr);
        jrogue_windows_wait_for_direct_exit();
        exit(1);
    }
#endif				/* Start up cursor package */
    start_custom_color();
    init_probs();			/* Set up prob tables for objects */
    init_player();			/* Set up initial player stats */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();

    /*
     * The screen must be at least NUMLINES x NUMCOLS
     */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
	jrogue_endwin();
	printf(JSTR("\nごめん！ 画面の大きさは最低 %dx%d なんだ。\n", "\nSorry, the screen must be at least %dx%d.\n"), NUMLINES, NUMCOLS);
/*	endwin();	*/
	my_exit(1);
    }

#ifdef _WIN32
    /* Fixed 80x24 logical game for ConsoleHost, Windows Terminal and UCRT64.
     * Automatic physical window resizing is a later, separate stage. */
    _COLS = NUMCOLS;
    _LINES = NUMLINES;
#else
    if (COLS > MAXCOLS)
        _COLS = MAXCOLS;
    else
        _COLS  = COLS;
    if (LINES > MAXLINES)
        _LINES = MAXLINES;
    else
        _LINES = LINES;
#endif

    /*
     * Make the active drawing color state available before the first
     * animation.  refresh2() normally synchronizes color_mode2 with
     * color_mode, but the start curtain is drawn before refresh2() is
     * ever called.
     */
    color_mode2 = color_mode ? TRUE : FALSE;

    /*
     * Set up windows
     */
    hw = newwin(_LINES, _COLS, 0, 0);
    jrogue_wset_normal_attr(hw);
    if (color_mode)
    {
	bkgd(' ' | jrogue_normal_attr());
	attrset(jrogue_normal_attr());
	erase();
	jrogue_refresh_window(stdscr);
	wbkgd(hw, ' ' | jrogue_normal_attr());
	wattrset(hw, jrogue_normal_attr());
    }
    idlok(stdscr, TRUE);
    idlok(hw, TRUE);
    jrogue_restore_cursor();
#ifdef MASTER
    noscore = wizard;
#endif
    start_curtain = jrogue_start_curtain_enabled();
    if (start_curtain) {
        /* Allocate the snapshot before closing the curtain.  If this
         * fails, start normally instead of leaving a closed curtain. */
        start_snapshot = newwin(_LINES, NUMCOLS, 0, 0);
        if (start_snapshot == NULL)
            start_curtain = FALSE;
        else {
            jrogue_show_cursor(FALSE);
            jrogue_start_drop_curtain();
            start_curtain_covers_screen = TRUE;
        }
    }

    new_level();			/* Draw current level */

    /*
     * Start up daemons and fuses in the same order as the DOS 1.48 version.
     */
    start_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    start_daemon(stomach, 0, AFTER);
    start_daemon(runners, 0, AFTER);

    if (jrogue_lang_is_japanese)
	msg(terse ? "やあ、%s。" : "やあ、%s。運命の洞窟へようこそ！", whoami);
    else
	msg(terse ? "Hello %s." : "Hello %s.  Welcome to the Dungeons of Doom.", whoami);

    if (start_curtain) {
        /* stdscr now contains the complete dungeon and welcome message.
         * Save that image without displaying it, then restore the closed
         * curtain in stdscr before any physical screen update. */
        overwrite(stdscr, start_snapshot);
        jrogue_start_restore_curtain_virtual();
        start_curtain_covers_screen = FALSE;
        jrogue_start_raise_curtain(start_snapshot);
        delwin(start_snapshot);
        start_snapshot = NULL;
        jrogue_restore_cursor();
    }
    playit();
    return(0);
}

/*
 * endit:
 *	Exit the program abnormally.
 */

void
endit(int sig)
{
    (void)sig;
    fatal(JSTR("それでは、またね。\n", "Okay, bye.\n"));
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */

void
fatal(const char *s)
{
    mvaddstr(_LINES - 2, 0, s);
    jrogue_refresh_window(stdscr);
    jrogue_endwin();
    my_exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(int range)
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int 
roll(int number, int sides)
{
    int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

/*
 * tstp:
 *	Handle stop and start signals
 */

void
tstp(int ignored)
{
    int y, x;
    int oy, ox;

	(void)ignored;

    /*
     * leave nicely
     */
    getyx(curscr, oy, ox);
    mvcur(0, _COLS - 1, _LINES - 1, 0);
    jrogue_endwin();
    resetltchars();
    fflush(stdout);
	md_tstpsignal();

    /*
     * start back up again
     */
	md_tstpresume();
    raw();
    noecho();
    keypad(stdscr,1);
    playltchars();
    clearok(curscr, TRUE);
#ifdef _WIN32
    /* As after a shell escape, reinstate the saved game visibility before
     * the first redraw, so suspend/resume never exposes an underline. */
    jrogue_restore_cursor();
#endif
    jrogue_refresh_window(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    /* Modern curses/ncurses (including macOS) keeps WINDOW opaque. */
    wmove(curscr, oy, ox);
}

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */

void
playit(void)
{
    /*
     * ROGUEOPTS is read once in main(), before a new game is created.
     * Do not parse it again here: restore() also calls playit(), and
     * saved options (including terse and jump) must not be overwritten.
     * The original slow-terminal defaults are unnecessary for JEC's
     * fixed 80-column screen.
     */
    oldpos = hero;
    oldrp = roomin(&hero);
    while (playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */

void
quit(int sig)
{
    int oy, ox;
    int ch;

    (void)sig;

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (!q_comm)
	mpos = 0;
    getyx(curscr, oy, ox);
    jrogue_set_normal_attr();
    move(0, 0);
    clrtoeol();
    move(0, 0);
    if (!terse)
	addstr(JSTR("冒険を", "Do you wish to "));
    if (jrogue_lang_is_japanese)
    {
	addstr("終わりにしますか？（");
	str_attr("%Yes/%No");
	addstr("）?");
    }
    else
	str_attr("end your quest now (%Yes/%No) ?");
    look(FALSE);
    jrogue_refresh_window(stdscr);
    /*
     * The quit prompt is a single-key confirmation.  Keep the cursor hidden
     * here even when the user has enabled cursor display, matching the DOS
     * presentation and avoiding a stray cursor on the player position.
     */
    jrogue_show_cursor(FALSE);
    ch = readchar();
    jrogue_restore_cursor();
    if (ch == 'y' || ch == 'Y')
    {
	signal(SIGINT, leave);
	clear();
	jrogue_clear_screen_chars();
	mvprintw(_LINES - 2, 0, JSTR("所持金は＄%d", "You quit with %d gold pieces"), purse);
	move(_LINES - 1, 0);
	jrogue_refresh_window(stdscr);
	score(purse, 1, 0);
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
	my_exit(0);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	jrogue_refresh_window(stdscr);
	mpos = 0;
	count = 0;
	to_death = FALSE;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */

void
leave(int sig)
{
    static char buf[BUFSIZ];

    (void)sig;

    setbuf(stdout, buf);	/* throw away pending output */

    if (!isendwin())
    {
	mvcur(0, _COLS - 1, _LINES - 1, 0);
	jrogue_endwin();
    }

    putchar('\n');
    my_exit(0);
}

/*
 * shell:
 *	Let them escape for a while
 */

void
shell(void)
{
    /*
     * Set the terminal back to original mode
     */
    move(_LINES-1, 0);
    jrogue_refresh_window(stdscr);
    restore_terminal_background();
    jrogue_endwin();
    resetltchars();
    putchar('\n');
    in_shell = TRUE;
    after = FALSE;
    fflush(stdout);
    /*
     * Fork and do a shell
     */
    md_shellescape();

    noecho();
    raw();
    keypad(stdscr,1);
    playltchars();
    refresh2();
    in_shell = FALSE;
    clearok(stdscr, TRUE);
}

/*
 * my_exit:
 *	Leave the process properly
 */

void
my_exit(int st)
{
    restore_terminal_background();
    resetltchars();
    exit(st);
}

