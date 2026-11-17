/*
 * save and restore routines
 *
 * @(#)save.c	4.33 (Berkeley) 06/01/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <curses.h>
#include <locale.h>
#include "rogue.h"
#include "score.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#include "windows/rogue_windows_console.h"
#define JROGUE_SAVE_OPEN_READ(path) jrogue_windows_fopen_utf8((path), L"rb")
#define JROGUE_SAVE_OPEN_WRITE(path) jrogue_windows_fopen_utf8((path), L"wb")
#define JROGUE_SAVE_STAT(path, st) jrogue_windows_stat_utf8((path), (st))
#else
#define JROGUE_SAVE_OPEN_READ(path) fopen((path), "rb")
#define JROGUE_SAVE_OPEN_WRITE(path) fopen((path), "wb")
#define JROGUE_SAVE_STAT(path, st) stat((path), (st))
#endif


typedef struct stat STAT;
static STAT sbuf;

#define SAVE_MAGIC_SIZE 32
#define SAVE_VERSION_SIZE 80
/* Historical on-disk signature: keep unchanged across the project rename,
 * so saved adventures from JEC.091 remain readable. */
static const char save_magic[] = "Rogue DOS148 JEC save";


static void
restore_welcome_message(void)
{
    char welcome[MAXSTR];

    if (terse)
        snprintf(welcome, sizeof welcome, JSTR("%s、おかえりなさい！", "%s, Welcome back!"), whoami);
    else
        snprintf(welcome, sizeof welcome, JSTR("やあ、%s。運命の洞窟へおかえり！", "Hello %s, Welcome back to the Dungeons of Doom!"), whoami);
    welcome[sizeof welcome - 1] = '\0';

    /*
     * The saved stdscr image can contain an old Japanese message line.
     * mvwinch()/mvwaddch() is not reliable for reconstructing multibyte
     * text in that transient line on every curses implementation, so clear
     * it explicitly and print the new DOS-style restore message with
     * printw(), the same path used by the Japanese status line.
     */
    jrogue_set_normal_attr();
    move(0, 0);
    clrtoeol();
    printw("%s", welcome);
    clrtoeol();
    jrogue_strlcpy(huh, welcome, MAXSTR);
    mpos = mbstrlen(welcome);
    refresh();
}

/*
 * save_game:
 *	Implement the "save game" command
 */

void
save_game(void)
{
    FILE *savef;
    int c;
    auto char buf[MAXSTR];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    msg(JSTR("ファイル名（%s）でセーブしますか？", "save file (%s)?"), file_name);
	jrogue_show_cursor(TRUE);
	    c = readchar();
	jrogue_restore_cursor();
	    mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg(JSTR("'Y'か'N'で答えてください。（ESCでキャンセル）", "please answer Y or N"));
	jrogue_show_cursor(TRUE);
	}
	if (c == 'y' || c == 'Y')
	{
	    addstr("Yes\n");
	    refresh();
	    jrogue_strlcpy(buf, file_name, sizeof buf);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg(JSTR("ファイル名：", "file name: "));
	jrogue_show_cursor(TRUE);
	buf[0] = '\0';
	jrogue_set_normal_attr();
	if (get_str(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	jrogue_restore_cursor();
	    return;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (JROGUE_SAVE_STAT(buf, &sbuf) >= 0)
	{
	    for (;;)
	    {
		msg(JSTR("そのファイルは存在します。上書きしますか？", "File exists.  Do you wish to overwrite it?"));
	jrogue_show_cursor(TRUE);
		mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit_it;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg(JSTR("'Y'か'N'で答えてください。", "Please answer Y or N"));
	jrogue_show_cursor(TRUE);
	    }
	    msg(JSTR("ファイル名： %s", "file name: %s"), buf);
	    md_unlink(buf);
	}
	jrogue_strlcpy(file_name, buf, MAXSTR);
	if ((savef = JROGUE_SAVE_OPEN_WRITE(file_name)) == NULL)
	    msg("%s", strerror(errno));
    } while (savef == NULL);
    msg("");
    save_file(savef, TRUE);
	jrogue_restore_cursor();
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	received
 */

void
auto_save(int sig)
{
    FILE *savef = NULL;
    (void)sig;

    md_ignoreallsignals();
    if (file_name[0] != '\0' && ((savef = JROGUE_SAVE_OPEN_WRITE(file_name)) != NULL ||
	(md_unlink_open_file(file_name, savef) >= 0 && (savef = JROGUE_SAVE_OPEN_WRITE(file_name)) != NULL)))
	    save_file(savef, FALSE);
    exit(0);
}

/*
 * save_file:
 *	Write the saved game on the file
 */

void
save_file(FILE *savef, int manual)
{
    char buf[80];
    char magic[SAVE_MAGIC_SIZE];
    char save_version[SAVE_VERSION_SIZE];
    int save_err = 0;
    int err = 0;

    mvcur(0, _COLS - 1, _LINES - 1, 0); 
    putchar('\n');
    restore_terminal_background();
    endwin();
    resetltchars();
    md_chmod(file_name, 0400);

    memset(magic, 0, sizeof magic);
    jrogue_strlcpy(magic, save_magic, sizeof magic);
    if (encwrite(magic, sizeof magic, savef) != sizeof magic && save_err == 0)
	save_err = encerror() ? encerror() : EIO;

    memset(save_version, 0, sizeof save_version);
    jrogue_strlcpy(save_version, version, sizeof save_version);
    if (encwrite(save_version, sizeof save_version, savef) != sizeof save_version && save_err == 0)
	save_err = encerror() ? encerror() : EIO;

    snprintf(buf, sizeof buf, "%d x %d lang %s\n", _LINES, _COLS, jrogue_language_code());
    if (encwrite(buf, 80, savef) != 80 && save_err == 0)
	save_err = encerror() ? encerror() : EIO;

    err = rs_save_file(savef);
    if (err != 0 && save_err == 0)
	save_err = err;

    if (fflush(savef) == EOF && save_err == 0)
	save_err = errno ? errno : EIO;

    if (fclose(savef) == EOF && save_err == 0)
	save_err = errno ? errno : EIO;

    if (manual && save_err == 0)
	printf(JSTR("ゲームを %s にセーブしました。\n", "Game saved as %s.\n"), file_name);

    if (save_err != 0)
    {
	errno = save_err;
	printf("%s", JSTR("セーブファイルを書き込めませんでした。\n", "Could not write save file.\n"));
	if (errno != 0)
	    printf("%s\n", strerror(errno));
	md_unlink(file_name);
#ifdef _WIN32
        if (manual)
            jrogue_windows_wait_for_direct_exit();
#endif
	exit(1);
    }

#ifdef _WIN32
    if (manual)
        jrogue_windows_wait_for_direct_exit();
#endif
    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
int
restore(const char *file)
{
    FILE *inf;
    int syml;
    auto char buf[MAXSTR];
    auto STAT sbuf2;
    int lines, cols;
    char saved_lang[8];
    char magic[SAVE_MAGIC_SIZE];
    int restore_err;

    if (strcmp(file, "-r") == 0)
	file = file_name;

    md_tstphold();

    if ((inf = JROGUE_SAVE_OPEN_READ(file)) == NULL)
    {
	printf(JSTR("セーブファイル %s を開けません。\n", "Cannot open save file %s.\n"), file);
	if (errno != 0)
	    printf("%s\n", strerror(errno));
	md_tstpresume();
	return FALSE;
    }
    if (JROGUE_SAVE_STAT(file, &sbuf2) < 0)
    {
	printf(JSTR("セーブファイル %s の情報を取得できません。\n", "Cannot stat save file %s.\n"), file);
	if (errno != 0)
	    printf("%s\n", strerror(errno));
	fclose(inf);
	md_tstpresume();
	return FALSE;
    }
    syml = is_symlink(file);

    printf(JSTR("%s から再開しています...\n", "Restoring %s\n"), file);
    fflush(stdout);
    md_sleep(1);

    memset(magic, 0, sizeof magic);
    jrogue_strlcpy(magic, save_magic, sizeof magic);

    encclearerr();
    memset(buf, 0, sizeof buf);
    encread(buf, SAVE_MAGIC_SIZE, inf);
    if (encerror() || memcmp(buf, magic, SAVE_MAGIC_SIZE) != 0)
    {
	printf("%s", JSTR("これはセーブファイルではありません。\n", "Not a savefile.\n"));
	fclose(inf);
	md_tstpresume();
	return FALSE;
    }

    memset(buf, 0, sizeof buf);
    encread(buf, SAVE_VERSION_SIZE, inf);
    if (encerror())
    {
	printf("%s", JSTR("セーブファイルを読み込めませんでした。\n", "Could not read save file.\n"));
	fclose(inf);
	md_tstpresume();
	return FALSE;
    }

    memset(buf, 0, sizeof buf);
    encread(buf,80,inf);
    if (encerror() || sscanf(buf,"%d x %d lang %7s\n", &lines, &cols, saved_lang) < 2)
    {
	printf("%s", JSTR("セーブファイルを読み込めませんでした。\n", "Could not read save file.\n"));
	fclose(inf);
	md_tstpresume();
	return FALSE;
    }

    /* saved_lang records the language used when the file was saved.
     * The saved game itself is language-independent: object types and
     * discoveries are restored by internal IDs, while user-entered names
     * and labels remain raw UTF-8 strings.  A language mismatch is allowed.
     */

    /* The external reset program is Unix-specific and must not be
     * invoked in a native Windows console.  Keep the UTF-8 locale after
     * a restore, just as at initial startup. */
#ifndef _WIN32
    system("reset");
#endif
    jrogue_init_locale();
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(1);
#endif
    initscr();                          /* Start up cursor package */
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize(0);
    if (jrogue_windows_sync_direct_launch_screen() == ERR) {
        endwin();
        fputs(JSTR("ゲーム画面を80×24に調整できませんでした。\n",
                   "Could not adjust the game screen to 80x24.\n"), stderr);
        fclose(inf);
        md_tstpresume();
        return FALSE;
    }
#endif
    start_custom_color();
    keypad(stdscr, 1);

#ifdef _WIN32
    /* Match the 80x24 logical screen used at new-game startup. */
    _COLS = NUMCOLS;
    _LINES = NUMLINES;
#else
    _COLS  = (COLS > MAXCOLS) ? MAXCOLS : COLS;
    _LINES = (LINES > MAXLINES) ? MAXLINES : LINES;
#endif

    if (lines > _LINES)
    {
        restore_terminal_background();
        endwin();
        printf(JSTR("セーブ元のゲームは%d行でプレイされていたようです。\n", "The saved game appears to have been played with %d lines.\n"),lines);
        printf(JSTR("このスクリーンは%d行なので、そのファイルを使うことはできません。\n", "This screen has %d lines, so that file cannot be used.\n"),_LINES);
	fclose(inf);
	md_tstpresume();
        return(FALSE);
    }
    if (cols > _COLS)
    {
        restore_terminal_background();
        endwin();
        printf(JSTR("セーブ元のゲームは%d列でプレイされていたようです。\n", "The saved game appears to have been played with %d columns.\n"),cols);
        printf(JSTR("このスクリーンは%d列なので、そのファイルを使うことはできません。\n", "This screen has %d columns, so that file cannot be used.\n"),_COLS);
	fclose(inf);
	md_tstpresume();
        return(FALSE);
    }

    hw = newwin(_LINES, _COLS, 0, 0);
    jrogue_wset_normal_attr(hw);
    if (color_mode)
    {
	bkgd(' ' | jrogue_normal_attr());
	attrset(jrogue_normal_attr());
	erase();
	refresh();
	wbkgd(hw, ' ' | jrogue_normal_attr());
	wattrset(hw, jrogue_normal_attr());
    }
    setup();

    restore_err = rs_restore_file(inf);
    if (restore_err != 0)
    {
	restore_terminal_background();
	endwin();
	printf("%s", JSTR("セーブファイルを読み込めませんでした。\n", "Could not read save file.\n"));
	fclose(inf);
	md_tstpresume();
	return FALSE;
    }
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    mpos = 0;
/*    printw(0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime)); */
/*
    printw("%s: %s", file, ctime(&sbuf2.st_mtime));
*/
    clearok(stdscr,TRUE);
    /*
     * defeat multiple restarting from the same place
     */
#ifdef MASTER
    if (!wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    restore_terminal_background();
	    endwin();
	    printf("%s", JSTR("\nそのファイルから再開することはできません。\n", "\nCannot restore from a linked file\n"));
	    fclose(inf);
	    md_tstpresume();
	    return FALSE;
	}

#ifdef MASTER
    if (!wizard)
#endif
    {
	int unlink_result = md_unlink_open_file(file, inf);
#ifdef _WIN32
	inf = NULL;
#endif
	if (unlink_result < 0)
	{
	    restore_terminal_background();
	    endwin();
	    printf("Cannot unlink file\n");
	    if (inf != NULL)
		fclose(inf);
	    md_tstpresume();
	    return FALSE;
	}
    }

    if (pstats.s_hpt <= 0)
    {
	restore_terminal_background();
	endwin();
	printf("%s", JSTR("\n\"彼は死んだんだよ、ジム。\"\n", "\n\"He's dead, Jim\"\n"));
	if (inf != NULL)
	    fclose(inf);
	md_tstpresume();
	return FALSE;
    }

    md_tstpresume();

    jrogue_strlcpy(file_name, file, MAXSTR);
    clearok(curscr, TRUE);
    srand(md_getpid());
/*    msg("file name: %s", file); ### */
    if (inf != NULL)
	fclose(inf);

    oldpos = hero;
    oldrp = roomin(&hero);
    proom = oldrp;

    jrogue_restore_cursor();
    restore_welcome_message();
    playit();
    /*NOTREACHED*/
    return(0);
}

static int encerrno = 0;

int
encerror()
{
    return encerrno;
}

void
encseterr(int err)
{
    encerrno = err;
}

int
encclearerr()
{
    int n = encerrno;

    encerrno = 0;

    return(n);
}

/*
 * encwrite:
 *	Perform an encrypted write
 */

size_t
encwrite(const char *start, size_t size, FILE *outf)
{
    const char *e1, *e2;
    char fb;
    int temp;
    size_t o_size = size;
    e1 = encstr;
    e2 = statlist;
    fb = 0;

    if (encerrno) {
	errno = encerrno;
	return 0;
    }

    while(size)
    {
	if (putc(*start++ ^ *e1 ^ *e2 ^ fb, outf) == EOF)
	{
	    encerrno = errno ? errno : EIO;
            break;
	}

	temp = *e1++;
	fb = fb + ((char) (temp * *e2++));
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
	size--;
    }

    return(o_size - size);
}

/*
 * encread:
 *	Perform an encrypted read
 */
size_t
encread(char *start, size_t size, FILE *inf)
{
    const char *e1, *e2;
    char fb;
    int temp;
    size_t read_size;
    size_t items;
    fb = 0;

    if (encerrno) {
	errno = encerrno;
	return 0;
    }

    items = read_size = fread(start,1,size,inf);

    e1 = encstr;
    e2 = statlist;

    while (read_size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb = fb + (char)(temp * *e2++);
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }

    if (items != size)
	encerrno = errno ? errno : EIO;

    return(items);
}

static void
clear_score_entry(SCORE *scp)
{
    memset(scp, 0, sizeof *scp);
}

/*
 * read_score
 *	Read in the score file.  A missing or short score file is not a fatal
 *	encryption error; it just means the remaining score entries are empty.
 */
void
rd_score(SCORE *top_ten)
{
    char scoreline[100];
    int i;

    if (scoreboard == NULL)
	return;

    rewind(scoreboard);
    encclearerr();

    for(i = 0; i < numscores; i++)
    {
        if (encread(top_ten[i].sc_name, MAXSTR, scoreboard) != MAXSTR)
        {
            clear_score_entry(&top_ten[i]);
            encclearerr();
            break;
        }
	scoreline[0] = '\0';
        if (encread(scoreline, 100, scoreboard) != 100)
        {
            clear_score_entry(&top_ten[i]);
            encclearerr();
            break;
        }
        if (sscanf(scoreline, " %u %d %u %d %d %x \n",
            &top_ten[i].sc_uid, &top_ten[i].sc_score,
            &top_ten[i].sc_death_id, &top_ten[i].sc_killer_id,
            &top_ten[i].sc_level, &top_ten[i].sc_rank) != 6)
            clear_score_entry(&top_ten[i]);
    }

    rewind(scoreboard);
    encclearerr();
}

/*
 * write_score
 *	Write the score file.
 */
void
wr_score(SCORE *top_ten)
{
    char scoreline[100];
    int i;

    if (scoreboard == NULL)
	return;

    rewind(scoreboard);
    encclearerr();

    for(i = 0; i < numscores; i++)
    {
          memset(scoreline,0,100);
          if (encwrite(top_ten[i].sc_name, MAXSTR, scoreboard) != MAXSTR)
              break;
          snprintf(scoreline, sizeof scoreline, " %u %d %u %d %d %x \n",
              top_ten[i].sc_uid, top_ten[i].sc_score,
              top_ten[i].sc_death_id, top_ten[i].sc_killer_id,
              top_ten[i].sc_level, top_ten[i].sc_rank);
          if (encwrite(scoreline,100,scoreboard) != 100)
              break;
    }

    fflush(scoreboard);
    rewind(scoreboard);
    encclearerr();
}
