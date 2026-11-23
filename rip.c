/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	4.57 (Berkeley) 02/05/99
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <curses.h>
#include <locale.h>
#include "rogue.h"
#include "score.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

int jrogue_score_color_output = FALSE;
int jrogue_score_waited_for_exit = FALSE;

static const char *rip[] = {
"                       __________  \n",
"                      /          \\\n",
"                     /            \\\n",
JSTR_STATIC("                    /   安らかに   \\\n", "                    /              \\\n"),
JSTR_STATIC("                   /      眠れ      \\\n", "                   /                \\\n"),
"                  /                  \\\n",
"                  |                  |\n",
"                  |                  |\n",
"                  |                  |\n",
JSTR_STATIC("                  |    により死す    |\n", "                  |                  |\n"),
"                  |       1980       |\n",
"                 *|     *  *  *      | *\n",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    0
};
void
jrogue_apply_rip_language(void)
{
    rip[3] = JSTR("                    /   安らかに   \\\n", "                    /              \\\n");
    rip[4] = JSTR("                   /      眠れ      \\\n", "                   /                \\\n");
    rip[9] = JSTR("                  |    により死す    |\n", "                  |                  |\n");
}




static void
score_clear_plain_screen(void)
{
    /*
     * Score lists are printed after endwin() on the terminal's normal
     * screen.  Clear only the visible terminal contents before a game-over
     * score so prompts left by wrappers or launch scripts do not remain
     * above the hall of fame.  Do not force a foreground/background color
     * here; in monochrome mode the user's terminal colors should be kept.
     */
#ifdef _WIN32
    jrogue_windows_score_clear_plain_screen();
#else
    printf("\033[0m\033[H\033[2J");
    fflush(stdout);
#endif
}

static void
score_reset_attrs(void)
{
#ifdef _WIN32
    if (!jrogue_windows_score_uses_vt())
        return;
#endif
    if (jrogue_score_color_output)
        printf("\033[0m\033[37m%s", jrogue_black_background_ansi());
    else
        printf("\033[0m");
}

static void
score_set_header_color(void)
{
    if (jrogue_score_color_output)
        printf("\033[38;5;229m");
}

static void
score_set_line_color(int highlight)
{
    if (!jrogue_score_color_output)
        return;
    if (highlight)
        printf("\033[38;5;229m");
    else
        printf("\033[38;5;180m");
}

static void
score_set_name_color(void)
{
    if (jrogue_score_color_output)
        printf("\033[38;5;167m");
}

static int
score_rank_index(const SCORE *scp)
{
    int rank = (int) scp->sc_rank;
    int max_rank = 21;

    if (rank < 1 || rank > max_rank)
        return -1;
    return rank - 1;
}

static const char *
score_rank_name(const SCORE *scp)
{
    int idx = score_rank_index(scp);

    if (idx < 0)
        return NULL;
    return JSTR(rank_names_ja[idx], rank_names_en[idx]);
}

static const char *
score_guild_honor(void)
{
    return JSTR("ギルドに讃えられる", "Honored by the Guild");
}

static int
score_should_show_rank(const SCORE *scp, const char *rank)
{
    int name_len, rank_len;

    if (score_rank_index(scp) <= 0 || scp->sc_name[0] == '\0' || rank == NULL)
        return FALSE;

    if (jrogue_lang_is_japanese) {
        name_len = (int) mbstrlen(scp->sc_name);
        rank_len = (int) mbstrlen(rank);
    }
    else {
        name_len = (int) strlen(scp->sc_name);
        rank_len = (int) strlen(rank);
    }

    return (name_len + 10 + rank_len) < _COLS;
}

static int
score_death_id(const SCORE *scp)
{
    /* sc_death_id is unsigned, and SC_DEATH_KILLED is zero. */
    if (scp->sc_death_id > SC_DEATH_WINNER)
        return SC_DEATH_KILLED;
    return (int) scp->sc_death_id;
}

static void
score_add_tail(char *tail, size_t size, const char *fmt, ...)
{
    va_list ap;
    size_t len;

    len = strlen(tail);
    if (len >= size)
        return;
    va_start(ap, fmt);
    vsnprintf(tail + len, size - len, fmt, ap);
    va_end(ap);
}

static void
score_format_tail(const SCORE *scp, char *tail, size_t size,
    const char **reason)
{
    const char *rank = score_rank_name(scp);
    const char *honor = (scp->sc_level >= 26) ? score_guild_honor() : NULL;
    int death_id = score_death_id(scp);
    int show_rank = score_should_show_rank(scp, rank);

    tail[0] = '\0';
    if (show_rank)
        score_add_tail(tail, size, " \"%s\"", rank);

    if (jrogue_lang_is_japanese) {
        if (death_id == SC_DEATH_WINNER)
            score_add_tail(tail, size, " %s！", reason[death_id]);
        else if (honor != NULL)
            score_add_tail(tail, size, " %s", honor);
        else if (death_id == SC_DEATH_KILLED) {
            const char *killer = killname(scp->sc_killer_id, FALSE);

            score_add_tail(tail, size, " 地下%d階にて%s%s", scp->sc_level,
                killer, isupper(scp->sc_killer_id) ? "と戦いて死す" : reason[death_id]);
        }
        else
            score_add_tail(tail, size, " 地下%d階にて%s", scp->sc_level,
                reason[death_id]);
    }
    else {
        if (death_id == SC_DEATH_WINNER)
            score_add_tail(tail, size, " %s!", reason[death_id]);
        else if (honor != NULL)
            score_add_tail(tail, size, " %s", honor);
        else if (death_id == SC_DEATH_KILLED)
            score_add_tail(tail, size, " %s by %s on level %d",
                reason[death_id], killname(scp->sc_killer_id, TRUE),
                scp->sc_level);
        else
            score_add_tail(tail, size, " %s on level %d", reason[death_id],
                scp->sc_level);
    }
}

static void
score_print_record_colored(const SCORE *scp, const char *tail, int highlight)
{
    score_set_line_color(highlight);
    printf("%-5d ", scp->sc_score);

    if (highlight)
        printf("%s", scp->sc_name);
    else {
        score_set_name_color();
        printf("%s", scp->sc_name);
        score_set_line_color(FALSE);
    }

    printf("%s", tail);
}

static void
score_print_record_plain(const SCORE *scp, const char *tail)
{
    printf("%-5d %s%s", scp->sc_score, scp->sc_name, tail);
}

static void
score_print_list(SCORE *top_ten, SCORE *endp, SCORE *highlight,
    int flags, int prflags, const char **reason)
{
    SCORE *scp, *sc2;
    char tail[240];
#ifdef MASTER
    char buf[240];
    int i;
#endif

#ifndef MASTER
    (void) prflags;
#endif

    (void) flags;
    sc2 = highlight;
    score_reset_attrs();
    printf("%s\n\n", JSTR("ギルドマスターの栄誉殿堂:", "Guildmaster's Hall Of Fame:"));
    score_set_header_color();
    printf("%s", JSTR("金塊\n\n", "Gold\n\n"));
    score_reset_attrs();
    for (scp = top_ten; scp < endp; scp++)
    {
	if (scp->sc_score) {
            score_format_tail(scp, tail, sizeof tail, reason);

            if (jrogue_score_color_output)
                score_print_record_colored(scp, tail, sc2 == scp);
	    else {
                if (sc2 == scp)
	            md_raw_standout();
                score_print_record_plain(scp, tail);
            }

#ifdef MASTER
	    if (prflags == 1)
	    {
                snprintf(buf, sizeof buf, " (%s)", md_getrealname(scp->sc_uid));
                if (jrogue_score_color_output)
                    score_set_line_color(sc2 == scp);
	        printf("%s", buf);
	    }
	    else if (prflags == 2)
	    {
		fflush(stdout);
		(void) fgets(prbuf,10,stdin);
		if (prbuf[0] == 'd')
		{
		    for (sc2 = scp; sc2 < endp - 1; sc2++)
			*sc2 = *(sc2 + 1);
		    sc2 = endp - 1;
		    sc2->sc_score = 0;
		    for (i = 0; i < MAXSTR; i++)
			sc2->sc_name[i] = (char) rnd(255);
		    sc2->sc_death_id = RN;
		    sc2->sc_level = RN;
		    sc2->sc_killer_id = RN;
		    sc2->sc_rank = 0;
		    scp--;
		}
	    }
#endif /* MASTER */
	    if (highlight == scp)
            {
                if (jrogue_score_color_output)
                    score_reset_attrs();
                else
		    md_raw_standend();
            }
            else if (jrogue_score_color_output)
                score_reset_attrs();
            putchar('\n');
	}
	else
	    break;
    }
    score_reset_attrs();
}

static void
score_show_colored_then_record(SCORE *top_ten, SCORE *endp, SCORE *highlight,
    int flags, int prflags, const char **reason)
{
    /*
     * Show the colored score on the terminal's alternate screen.  When the
     * player leaves that screen, print the same score once in monochrome on
     * the normal screen so the terminal scrollback is readable.
     */
    jrogue_score_color_output = TRUE;
    printf("\033[?1049h%s\033[H\033[2J", jrogue_black_background_ansi());
    score_print_list(top_ten, endp, highlight, flags, prflags, reason);
    printf("\n%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
    fflush(stdout);
    (void) fgets(prbuf, 80, stdin);
    jrogue_score_waited_for_exit = TRUE;
    score_reset_attrs();
    printf("\033[?1049l");
    fflush(stdout);

    jrogue_score_color_output = FALSE;
    score_clear_plain_screen();
    score_print_list(top_ten, endp, highlight, flags, prflags, reason);
}


/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */

void
score(int amount, int flags, int monst)
{
    SCORE *scp;
    int i;
    SCORE *sc2;
    SCORE *top_ten, *endp;
    int prflags = 0;
    int score_locked = FALSE;
    void (*fp)(int);
    jrogue_uid_t uid;
    const char *reason[] = {
	JSTR("により死す", "killed"),
	JSTR("逃亡す", "quit"),
	JSTR("完全勝利者", "A total winner"),
	JSTR("により死す", "killed")
    };
    jrogue_score_waited_for_exit = FALSE;
    start_score();

 if (flags >= 0
#ifdef MASTER
            || wizard
#endif
        )
    {
	mvaddstr(_LINES - 1, 0 , JSTR("［リターンを押してランキングへ］", "[Press Enter to see rankings]"));
        jrogue_prepare_input_attr(stdscr);
        jrogue_refresh_window(stdscr);
        wgetnstr(stdscr,prbuf,80);
 	jrogue_endwin();
        resetltchars();
	/*
	 * free up space to "guarantee" there is space for the top_ten
	 */
	delwin(stdscr);
	delwin(curscr);
	if (hw != NULL)
	    delwin(hw);
    }

    top_ten = malloc(numscores * sizeof (SCORE));

	if (top_ten == NULL)
		return;

    endp = &top_ten[numscores];
    for (scp = top_ten; scp < endp; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = (char) rnd(255);
	scp->sc_death_id = RN;
	scp->sc_level = RN;
	scp->sc_killer_id = RN;
	scp->sc_rank = 0;
	scp->sc_uid = RN;
    }

    signal(SIGINT, SIG_DFL);

#ifdef MASTER
    if (wizard)
    {
	if (strcmp(prbuf, "names") == 0)
	    prflags = 1;
	else if (strcmp(prbuf, "edit") == 0)
	    prflags = 2;
    }
#endif
    /*
     * Lock the score file before reading it so two simultaneous game exits
     * cannot read the same old table and overwrite each other's scores.
     */
    fp = signal(SIGINT, SIG_IGN);
    score_locked = lock_sc();

    rd_score(top_ten);
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (!noscore)
    {
	uid = md_getuid();
	for (scp = top_ten; scp < endp; scp++)
	    if (amount > scp->sc_score)
		break;
	if (scp < endp)
	{
	    sc2 = endp - 1;
	    while (sc2 > scp)
	    {
		*sc2 = sc2[-1];
		sc2--;
	    }
	    scp->sc_score = amount;
	    jrogue_strlcpy(scp->sc_name, whoami, sizeof scp->sc_name);
	    scp->sc_death_id = flags;
	    scp->sc_level = max_level;
	    scp->sc_killer_id = monst;
	    scp->sc_rank = pstats.s_lvl;
	    scp->sc_uid = uid;
	    sc2 = scp;
	}
    }

    if (sc2 != NULL && score_locked)
	wr_score(top_ten);
    if (score_locked)
	unlock_sc();
    signal(SIGINT, fp);
#ifdef _WIN32
    /* curses is finished.  Enable raw-output VT when possible; fallback
     * consoles still show a plain score, without literal escape codes. */
    jrogue_windows_score_prepare();
#endif
    /*
     * Print the list.  If color is enabled during a normal game-over score
     * display, show the colored score on the alternate screen first, then
     * leave a monochrome copy on the normal terminal screen.
     */
#ifdef MASTER
    if (color_mode == TRUE && flags != -1 && prflags != 2
#ifdef _WIN32
        && jrogue_windows_score_uses_vt()
#endif
        )
#else
    if (color_mode == TRUE && flags != -1
#ifdef _WIN32
        && jrogue_windows_score_uses_vt()
#endif
        )
#endif
        score_show_colored_then_record(top_ten, endp, sc2, flags, prflags, reason);
    else
    {
#ifdef MASTER
        jrogue_score_color_output = (color_mode == TRUE && flags != -1 && prflags != 2);
#ifdef _WIN32
        jrogue_score_color_output &= jrogue_windows_score_uses_vt();
#endif
#else
        jrogue_score_color_output = (color_mode == TRUE && flags != -1);
#ifdef _WIN32
        jrogue_score_color_output &= jrogue_windows_score_uses_vt();
#endif
#endif
        if (flags != -1)
            score_clear_plain_screen();
        score_print_list(top_ten, endp, sc2, flags, prflags, reason);
        jrogue_score_color_output = FALSE;
    }
}


#define RIP_BOX_WIDTH 29
#define RIP_BOX_HEIGHT 16
#define RIP_PAIR_BORDER 50
#define RIP_PAIR_FLOWER 51
#define RIP_PAIR_GRASS 52
#define RIP_PAIR_CURTAIN 53
#define RIP_ANIM_DELAY_MS 35

static void
rip_wset_attr(WINDOW *wn, int pair)
{
    if (color_mode == TRUE)
        wattrset(wn, COLOR_PAIR(pair) | A_NORMAL);
    else
        wattrset(wn, A_NORMAL);
}

static int
rip_border_pair(void)
{
    if (!jrogue_char_mode_pc)
        return JROGUE_PAIR_ASCII_WALL;
    return RIP_PAIR_BORDER;
}

static void
rip_wnormal_attr(WINDOW *wn)
{
    if (color_mode == TRUE)
        jrogue_wset_normal_attr(wn);
    else
        wattrset(wn, A_NORMAL);
}

struct rip_chars {
    const char *tl, *tr, *bl, *br;
    const char *h, *v;
    const char *single_tl, *single_tr, *single_bl, *single_br;
    const char *single_h, *single_v;
    const char *flower_line;
    const char *grass_line;
    const char *curtain_fill;
};

static const struct rip_chars rip_chars_extended = {
    "╔", "╗", "╚", "╝", "═", "║",
    "┌", "┐", "└", "┘", "─", "│",
    "  *    *      * ",
    "___\\/(\\/)/(\\/ \\\\(//)\\)\\/(//)\\\\)//(\\__",
    "▒"
};

static const struct rip_chars rip_chars_basic = {
    "+", "+", "+", "+", "-", "|",
    "+", "+", "+", "+", "-", "|",
    "  *    *      * ",
    "___\\/(\\/)/(\\/ \\\\(//)\\)\\/(//)\\\\)//(\\__",
    "#"
};

static const struct rip_chars *
rip_get_chars(void)
{
    /*
     * Keep the grave/curtain Unicode line art in UTF-8 environments even
     * when extchars is disabled.  noextchars only switches the dungeon map
     * back to ASCII-style glyphs.
     */
    if (MB_CUR_MAX > 1)
        return &rip_chars_extended;
    return &rip_chars_basic;
}

static int
rip_animation_enabled(void)
{
    return _LINES >= 20 && NUMCOLS >= 40;
}

static void
rip_wadd_border(WINDOW *wn, const char *str)
{
    (void)jrogue_wadd_utf8_glyph(wn, str);
}

static void
rip_mvwadd_border(WINDOW *wn, int row, int col, const char *str)
{
    wmove(wn, row, col);
    rip_wadd_border(wn, str);
}

static void
rip_wadd_curtain_fill(WINDOW *wn)
{
    (void)jrogue_wadd_utf8_glyph(wn, rip_get_chars()->curtain_fill);
}

static void
rip_wbox_center_text_attr(WINDOW *wn, int row, int left, int width,
    const char *text, int mono_underline)
{
    int inner_width, len, col, i;

    inner_width = width - 2;
    len = (int) mbstrlen(text);
    col = left + 1 + (inner_width - len) / 2;
    if (col < left + 1)
        col = left + 1;

    rip_wnormal_attr(wn);
    wmove(wn, row, left + 1);
    for (i = 0; i < inner_width; i++)
        waddch(wn, ' ');

    if (mono_underline && color_mode != TRUE)
        wattron(wn, A_UNDERLINE);
    mvwaddstr(wn, row, col, text);
    if (mono_underline && color_mode != TRUE)
        wattroff(wn, A_UNDERLINE);
    rip_wnormal_attr(wn);
}

static void
rip_wbox_center_text(WINDOW *wn, int row, int left, int width,
    const char *text)
{
    rip_wbox_center_text_attr(wn, row, left, width, text, FALSE);
}

static void
rip_wbox_center_name(WINDOW *wn, int row, int left, int width,
    const char *text)
{
    /*
     * The DOS 1.48 version used an underlined player name on monochrome
     * displays, while color displays distinguished it by color instead.
     * Keep the current color grave unchanged and underline only in mono.
     */
    rip_wbox_center_text_attr(wn, row, left, width, text, TRUE);
}

static void
rip_wdraw_double_box(WINDOW *wn, int top, int left, int width, int height)
{
    const struct rip_chars *rc;
    int row, col, right, bottom;

    rc = rip_get_chars();
    right = left + width - 1;
    bottom = top + height - 1;

    rip_wset_attr(wn, rip_border_pair());
    rip_mvwadd_border(wn, top, left, rc->tl);
    for (col = left + 1; col < right; col++)
        rip_mvwadd_border(wn, top, col, rc->h);
    rip_mvwadd_border(wn, top, right, rc->tr);

    for (row = top + 1; row < bottom; row++) {
        rip_mvwadd_border(wn, row, left, rc->v);
        rip_wnormal_attr(wn);
        wmove(wn, row, left + 1);
        for (col = left + 1; col < right; col++)
            waddch(wn, ' ');
        rip_wset_attr(wn, rip_border_pair());
        rip_mvwadd_border(wn, row, right, rc->v);
    }

    rip_mvwadd_border(wn, bottom, left, rc->bl);
    for (col = left + 1; col < right; col++)
        rip_mvwadd_border(wn, bottom, col, rc->h);
    rip_mvwadd_border(wn, bottom, right, rc->br);
    rip_wnormal_attr(wn);
}

static void
rip_wdraw_single_box(WINDOW *wn, int top, int left, int width, int height,
    int pair)
{
    const struct rip_chars *rc;
    int row, col, right, bottom;

    rc = rip_get_chars();
    right = left + width - 1;
    bottom = top + height - 1;

    rip_wset_attr(wn, pair);
    rip_mvwadd_border(wn, top, left, rc->single_tl);
    for (col = left + 1; col < right; col++)
        rip_mvwadd_border(wn, top, col, rc->single_h);
    rip_mvwadd_border(wn, top, right, rc->single_tr);

    for (row = top + 1; row < bottom; row++) {
        rip_mvwadd_border(wn, row, left, rc->single_v);
        rip_mvwadd_border(wn, row, right, rc->single_v);
    }

    rip_mvwadd_border(wn, bottom, left, rc->single_bl);
    for (col = left + 1; col < right; col++)
        rip_mvwadd_border(wn, bottom, col, rc->single_h);
    rip_mvwadd_border(wn, bottom, right, rc->single_br);
    rip_wnormal_attr(wn);
}

static void
rip_wcenter_text(WINDOW *wn, int row, const char *text)
{
    int col;

    col = (NUMCOLS - (int) mbstrlen(text)) / 2;
    if (col < 0)
        col = 0;
    mvwaddstr(wn, row, col, text);
}

static void
rip_wdraw_plant_line(WINDOW *wn, int row)
{
    const struct rip_chars *rc;

    /*
     * Match the DOS 1.48 layout: the flower and grass lines are
     * centered on the full screen and intentionally overwrite the box
     * bottom line.
     */
    rc = rip_get_chars();
    rip_wset_attr(wn, RIP_PAIR_FLOWER);
    rip_wcenter_text(wn, row, rc->flower_line);
    rip_wset_attr(wn, RIP_PAIR_GRASS);
    rip_wcenter_text(wn, row + 1, rc->grass_line);
    rip_wnormal_attr(wn);
}

static void
rip_render_grave(WINDOW *wn, int monst, const struct tm *lt)
{
    const char *killer;
    char killerbuf[PRBUF_SIZE];
    int top, left;

    if (color_mode == TRUE)
        wbkgd(wn, ' ' | jrogue_normal_attr());
    else
        wbkgd(wn, ' ' | A_NORMAL);
    werase(wn);
    rip_wnormal_attr(wn);

    top = 6;
    left = (NUMCOLS - 28) / 2;
    rip_wdraw_double_box(wn, top, left, RIP_BOX_WIDTH, RIP_BOX_HEIGHT);

    if (jrogue_lang_is_japanese) {
        killer = killname(monst, FALSE);
        jrogue_strlcpy(killerbuf, killer, sizeof killerbuf);
        rip_wbox_center_text(wn, top + 3, left, RIP_BOX_WIDTH, "安らかに");
        rip_wbox_center_text(wn, top + 4, left, RIP_BOX_WIDTH, "眠れ");
        rip_wbox_center_name(wn, top + 7, left, RIP_BOX_WIDTH, whoami);
        rip_wbox_center_text(wn, top + 8, left, RIP_BOX_WIDTH, killerbuf);
        if (isupper(monst))
            rip_wbox_center_text(wn, top + 9, left, RIP_BOX_WIDTH, "と戦いて死す");
        else
            rip_wbox_center_text(wn, top + 9, left, RIP_BOX_WIDTH, "により死す");
        snprintf(prbuf, PRBUF_SIZE, "金塊 %d個", purse);
        rip_wbox_center_text(wn, top + 11, left, RIP_BOX_WIDTH, prbuf);
        snprintf(prbuf, PRBUF_SIZE, "%4d", 1900 + lt->tm_year);
        rip_wbox_center_text(wn, top + 12, left, RIP_BOX_WIDTH, prbuf);
    }
    else {
        rip_wbox_center_text(wn, top + 3, left, RIP_BOX_WIDTH, "REST");
        rip_wbox_center_text(wn, top + 4, left, RIP_BOX_WIDTH, "IN");
        rip_wbox_center_text(wn, top + 5, left, RIP_BOX_WIDTH, "PEACE");
        rip_wbox_center_name(wn, top + 7, left, RIP_BOX_WIDTH, whoami);
        rip_wbox_center_text(wn, top + 8, left, RIP_BOX_WIDTH, "killed by");
        killer = killname(monst, TRUE);
        jrogue_strlcpy(killerbuf, killer, sizeof killerbuf);
        rip_wbox_center_text(wn, top + 9, left, RIP_BOX_WIDTH, killerbuf);
        snprintf(prbuf, PRBUF_SIZE, "%d Au", purse);
        rip_wbox_center_text(wn, top + 11, left, RIP_BOX_WIDTH, prbuf);
        snprintf(prbuf, PRBUF_SIZE, "%4d", 1900 + lt->tm_year);
        rip_wbox_center_text(wn, top + 12, left, RIP_BOX_WIDTH, prbuf);
    }

    rip_wdraw_plant_line(wn, top + 14);
}

static void
rip_drop_curtain(void)
{
    int row, col;

    rip_wdraw_single_box(stdscr, 0, 0, NUMCOLS, _LINES, RIP_PAIR_GRASS);
    wnoutrefresh(stdscr);
    (void)jrogue_curtain_update(TRUE);
    msleep(RIP_ANIM_DELAY_MS);

    for (row = 1; row < _LINES - 1; row++) {
        rip_wset_attr(stdscr, RIP_PAIR_CURTAIN);
        wmove(stdscr, row, 1);
        for (col = 1; col < NUMCOLS - 1; col++)
            rip_wadd_curtain_fill(stdscr);
        rip_wnormal_attr(stdscr);
        wnoutrefresh(stdscr);
        (void)jrogue_curtain_update(TRUE);
        msleep(RIP_ANIM_DELAY_MS);
    }
}

static void
rip_raise_curtain(WINDOW *grave)
{
    int row;

    for (row = _LINES - 1; row >= 0; row--) {
        copywin(grave, stdscr, row, 0, row, 0, row, NUMCOLS - 1, FALSE);
        wnoutrefresh(stdscr);
        (void)jrogue_curtain_update(TRUE);
        msleep(RIP_ANIM_DELAY_MS);
    }
    rip_wnormal_attr(stdscr);
}

static void
rip_show_grave(WINDOW *grave)
{
    overwrite(grave, stdscr);
    wnoutrefresh(stdscr);
    doupdate();
    rip_wnormal_attr(stdscr);
}

/*
 * death:
 *	Do something really fun when he dies
 */

void
death(int monst)
{
    struct tm *lt;
    time_t date;
    WINDOW *grave;
    int animate;

    signal(SIGINT, SIG_IGN);
    purse -= purse / 10;
    signal(SIGINT, leave);
    time(&date);
    lt = localtime(&date);
    animate = rip_animation_enabled();

    if (animate) {
        jrogue_show_cursor(FALSE);
        rip_drop_curtain();
    }

    grave = newwin(_LINES, NUMCOLS, 0, 0);
    if (grave != NULL) {
        rip_render_grave(grave, monst, lt);
        if (animate)
            rip_raise_curtain(grave);
        else
            rip_show_grave(grave);
        delwin(grave);
    }
    else {
        erase();
        rip_render_grave(stdscr, monst, lt);
        jrogue_refresh_window(stdscr);
    }

    if (animate)
        jrogue_restore_cursor();
    move(_LINES - 1, 0);
    jrogue_refresh_window(stdscr);
    score(purse, SC_DEATH_KILLED, monst);
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
    my_exit(0);
}

/*
 * center:
 *	Return the index to center the given string
 */
int
center(const char *str)
{
    return 28 - (((int)mbstrlen(str) + 1) / 2);
}

/*
 * total_winner:
 *	Code for a winner
 */

void
total_winner(void)
{
    THING *obj;
    struct obj_info *op;
    int worth = 0;
    int oldpurse;
    char c;

    clear();
    jrogue_set_normal_attr();
    addstr(JSTR("おめでとう！\n\n君はようやく太陽の光のもとにたどりついた。\n\n\n\n",
        "Congratulations!\n\nYou have made it to the light of day!\n\n\n\n"));
    addstr(JSTR("君は故郷へ帰り、すべての\n",
        "You journey home and sell all your\n"));
    addstr(JSTR("戦利品を売って大金を手に入れ、\n",
        "loot at a great profit and are\n"));
    addstr(JSTR("戦士ギルドに迎え入れられた。\n\n\n",
        "admitted to the fighters guild.\n\n\n"));
    jrogue_set_normal_attr();
    mvaddstr(_LINES - 1, 0, JSTR("--スペースを押してください--", "--Press space to continue--"));
    jrogue_refresh_window(stdscr);
    jrogue_show_cursor(TRUE);
    wait_for(stdscr, ' ');
    jrogue_restore_cursor();
    clear();
    jrogue_set_normal_attr();
    mvaddstr(0, 0, JSTR("    価値  持ちもの\n", "   Worth  Item\n"));
    oldpurse = purse;
    for (c = 'a', obj = pack; obj != NULL; c++, obj = next(obj))
    {
	switch (obj->o_type)
	{
	    case FOOD:
		worth = 2 * obj->o_count;
	    when WEAPON:
		worth = weap_info[obj->o_which].oi_worth;
		worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		worth = arm_info[obj->o_which].oi_worth;
		worth += (9 - obj->o_arm) * 100;
		worth += (10 * (a_class[obj->o_which] - obj->o_arm));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		worth = scr_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &scr_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when POTION:
		worth = pot_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &pot_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when RING:
		op = &ring_info[obj->o_which];
		worth = op->oi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
		{
			if (obj->o_arm > 0)
			    worth += obj->o_arm * 100;
			else
			    worth = 10;
		}
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when STICK:
		op = &ws_info[obj->o_which];
		worth = op->oi_worth;
		worth += 20 * obj->o_charges;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when AMULET:
		worth = 1000;
	}
	if (worth < 0)
	    worth = 0;
	printw("%c) %5d  %s\n", c, worth, inv_name(obj, FALSE));
	purse += worth;
    }
    printw(JSTR("   %5d  金塊          ", "   %5d  Gold Pieces          "), oldpurse);
    jrogue_refresh_window(stdscr);
    score(purse, SC_DEATH_WINNER, ' ');
    if (!jrogue_score_waited_for_exit)
    {
        printf("%s", JSTR("［リターンを押してください］", "[Press return to exit]\n"));
        fflush(stdout);
        (void) fgets(prbuf,10,stdin);
    }
    my_exit(0);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
const char *
killname(int monst, int doart)
{
    struct h_list *hp;
    const char *sp;
    int article;
    struct h_list nlist[] = {
	{'a',	JSTR("矢", "arrow"),		TRUE},
	{'b',	JSTR("ボルト", "bolt"),			TRUE},
	{'d',	JSTR("毒投げ矢", "dart"),			TRUE},
	{'f',	JSTR("落下", "fall"),			FALSE},
	{'s',	JSTR("飢え", "starvation"),		FALSE},
	{'\0'}
    };

    if (isupper(monst))
    {
	sp = monsters[monst-'A'].m_name;
	article = TRUE;
    }
    else
    {
	sp = JSTR("神", "God");
	article = FALSE;
	for (hp = nlist; hp->h_ch; hp++)
	    if (hp->h_ch == monst)
	    {
		sp = hp->h_desc;
		article = hp->h_print;
		break;
	    }
    }
    if (doart && article)
	snprintf(prbuf, PRBUF_SIZE, "a%s ", vowelstr(sp));
    else
	prbuf[0] = '\0';
    jrogue_strlcat(prbuf, sp, PRBUF_SIZE);
    return prbuf;
}


