/*
 * Various input/output functions
 *
 * @(#)io.c	4.32 (Berkeley) 02/05/99
 */

#include <stdarg.h>
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"

/*
 * msg:
 *	Display a message at the top of the screen.
 */
#define MORE_MSG_WIDTH 9
#define MAXMSG	(NUMCOLS - MORE_MSG_WIDTH - 1)

static char msgbuf[MAXSTR];
static int newpos = 0;
static int force_status_redraw = FALSE;

void
force_status(void)
{
    force_status_redraw = TRUE;
}


static void
msg_append_bytes(const char *s, int bytes)
{
    size_t used = strlen(msgbuf);

    /* The caller passes a complete grapheme.  Never split its UTF-8
     * bytes to satisfy the buffer size limit. */
    if (bytes <= 0 || (size_t)bytes >= sizeof msgbuf - used)
	return;
    memcpy(msgbuf + used, s, (size_t)bytes);
    msgbuf[used + (size_t)bytes] = '\0';
    newpos = (int)mbstrlen(msgbuf);
}

/* VARARGS1 */
int
msg(const char *fmt, ...)
{
    va_list args;

    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	jrogue_set_normal_attr();
	move(0, 0);
	clrtoeol();
	mpos = 0;
	return ~ESCAPE;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
    return endmsg();
}

/*
 * addmsg:
 *	Add things to the current message
 */
/* VARARGS1 */
void
addmsg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the --More--)
 */
int
endmsg(void)
{
    int ch;

    if (save_msg)
	jrogue_strlcpy(huh, msgbuf, MAXSTR);
    if (mpos)
    {
	/*
	 * Keep the play cursor state while redrawing the screen.  Only show
	 * the terminal cursor for the actual --More-- input wait; otherwise a
	 * hidden play cursor can briefly appear at the hero position.
	 */
	jrogue_restore_cursor();
	look(FALSE);
	jrogue_set_normal_attr();
	standout2();
	mvaddstr(0, mpos, JSTR(" --続く--", " --More--"));
	standend2();
	jrogue_show_cursor(TRUE);
	/* The startup curtain is still visible; do not expose the
	 * off-screen dungeon or message before the raising animation. */
	if (!jrogue_start_curtain_is_covered())
	    refresh();
	if (!msg_esc)
	    wait_for(stdscr, ' ');
	else
	{
	    while ((ch = readchar()) != ' ')
		if (ch == ESCAPE)
		{
		    jrogue_restore_cursor();
		    msgbuf[0] = '\0';
		    mpos = 0;
		    newpos = 0;
		    return ESCAPE;
		}
	}
	jrogue_restore_cursor();
    }
    /*
     * All messages should start with uppercase, except ones that
     * start with a pack addressing character
     */
    if (!jrogue_lang_is_japanese && islower((unsigned char)msgbuf[0])
	&& !lower_msg && msgbuf[1] != ')')
	msgbuf[0] = (char) toupper((unsigned char)msgbuf[0]);
    jrogue_set_normal_attr();
    mvaddstr(0, 0, msgbuf);
    clrtoeol();
    mpos = newpos;
    newpos = 0;
    msgbuf[0] = '\0';
    jrogue_restore_cursor();
    /* Preserve the DOS curtain's separate drawing buffer: the caller
     * snapshots this composed screen, then reveals it one row at a time. */
    if (!jrogue_start_curtain_is_covered())
        refresh();
    return ~ESCAPE;
}

/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
doadd(const char *fmt, va_list args)
{
    static char buf[MAXSTR];
    const char *bp;
    int len, bytes, width;
    size_t next;

    /*
     * Build the text safely before appending it to the message buffer.
     * MAXMSG is a one-line display-width limit.  Split long UTF-8
     * messages into Rogue-style --More-- chunks before the terminal can
     * auto-wrap them and leave stray text on the map line below.
     */
    len = vsnprintf(buf, sizeof buf, fmt, args);
    if (len < 0)
	buf[0] = '\0';
    else
	buf[sizeof buf - 1] = '\0';

    bp = buf;
    while (*bp != '\0')
    {
	if (newpos == 0 && *bp == ' ')
	{
	    bp++;
	    continue;
	}
	next = jrogue_text_next(bp, 0);
	bytes = (int)next;
	width = jrogue_text_width_n(bp, next);
	if (newpos > 0 && newpos + width > MAXMSG)
	{
	    endmsg();
	    continue;
	}
	msg_append_bytes(bp, bytes);
	bp += bytes;
	if (newpos >= MAXMSG && *bp != '\0')
	    endmsg();
    }
}


static void
str_attr_end_mode(int *mode)
{
    if (*mode == 1)
    {
	attroff(A_UNDERLINE);
	*mode = 0;
    }
    else if (*mode == 2)
    {
	standend2();
	*mode = 0;
    }
}

static void
str_attr_start_mode(int *mode, int newmode)
{
    str_attr_end_mode(mode);
    if (newmode == 1)
	attron(A_UNDERLINE);
    else if (newmode == 2)
	standout2();
    *mode = newmode;
}

static void
str_attr_putc(int ch)
{
    if (ch == '\n' || ch == '\r')
	printw("\n");
    else if (ch != '\0')
	addch((unsigned char)ch);
}

/*
 * str_attr:
 *	Print a string to the current screen position, interpreting the DOS 1.48
 *	attribute escapes.  In the original DOS source, strings such as
 *	"%Yes/%No" highlight only the character immediately following '%'.
 *	The documented %i/%I, %u/%U and %$ escapes are also accepted.
 */
void
str_attr(const char *str)
{
    int mode = 0;
    const unsigned char *s = (const unsigned char *)str;

    while (*s != '\0')
    {
	if (*s == '%' && s[1] != '\0')
	{
	    s++;
	    switch (*s)
	    {
	    case 'u':
		s++;
		if (*s != '\0')
		{
		    attron(A_UNDERLINE);
		    str_attr_putc(*s++);
		    attroff(A_UNDERLINE);
		}
		continue;
	    case 'U':
		str_attr_start_mode(&mode, 1);
		s++;
		continue;
	    case 'i':
		s++;
		if (*s != '\0')
		{
		    standout2();
		    str_attr_putc(*s++);
		    standend2();
		}
		continue;
	    case 'I':
		str_attr_start_mode(&mode, 2);
		s++;
		continue;
	    case '$':
		str_attr_end_mode(&mode);
		s++;
		continue;
	    default:
		standout2();
		str_attr_putc(*s++);
		standend2();
		continue;
	    }
	}
	str_attr_putc(*s++);
    }
    str_attr_end_mode(&mode);
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
int
step_ok(int ch)
{
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	default:
	    return (!isalpha(ch));
    }
}


static int
read_command_char(WINDOW *win)
{
    int ch, i, len, next;

    jrogue_prepare_input_attr(win);
    ch = md_readchar(win);

    if (ch >= 0x80 && ch <= 0xff)
    {
	len = jrogue_utf8_sequence_bytes((unsigned char)ch);
	for (i = 1; i < len; i++)
	{
	    next = md_readchar(win);
	    if (!(next >= 0x80 && next <= 0xff)
	    || (((unsigned char)next & 0xc0) != 0x80))
		break;
	}
	return INVALID_INPUT;
    }
    return ch;
}


/*
 * readchar:
 *	Reads and returns a character, checking for gross input errors
 */

int
readchar(void)
{
    int ch;

if (dos_typeahead != NULL && *dos_typeahead != '\0')
	return (unsigned char)*dos_typeahead++;

    ch = read_command_char(stdscr);

    if (ch == 3)
    {
		quit(0);
        return(27);
    }

    return(ch);
}

int
wreadchar(WINDOW *win)
{
    int ch;

if (dos_typeahead != NULL && *dos_typeahead != '\0')
	return (unsigned char)*dos_typeahead++;

    ch = read_command_char(win);

    if (ch == 3)
    {
		quit(0);
        return(27);
    }

    return(ch);
}




static const char *
status_rank_name(void)
{
    int idx = pstats.s_lvl - 1;

    if (idx < 0)
        idx = 0;
    else if (idx > 20)
        idx = 20;
    return JSTR(rank_names_ja[idx], rank_names_en[idx]);
}

static void
status_set_attr(int highlight)
{
    if (color_mode2 != FALSE)
        attrset(COLOR_PAIR(JROGUE_PAIR_STATUS) | (highlight ? A_BOLD : A_NORMAL));
    else
        attrset(highlight ? A_BOLD : A_NORMAL);
}

static int
status_hunger_col(void)
{
    /*
     * The original DOS 1.48 display kept the hunger state in a fixed
     * right-side field instead of appending it to the status text.  This
     * port still uses a single 24-line terminal status row, so reserve the
     * last six columns for Hungry/Weak/Faint rather than using a second row.
     */
    return NUMCOLS - 6;
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status(void)
{
    int oy, ox, temp;
    static int hpwidth = 0;
    static int s_hungry = 0;
    static int s_lvl = 0;
    static int s_pur = -1;
    static int s_hp = 0;
    static int s_arm = 0;
    static int s_str = 0;
    static int s_player_lvl = 0;
    static int s_color_mode = -1;
    static int s_lang_is_japanese = -1;
    const char *state_name[] =
    {"", JSTR("空腹", "Hungry"), JSTR("飢餓", "Weak"), JSTR("瀕死", "Faint") };
    
    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
    temp = (cur_armor != NULL ? cur_armor->o_arm : pstats.s_arm);
    if (s_hp == pstats.s_hpt && s_player_lvl == pstats.s_lvl && s_pur == purse
	&& s_arm == temp && s_str == pstats.s_str && s_lvl == level
	&& s_hungry == hungry_state
	&& s_color_mode == color_mode2
	&& s_lang_is_japanese == jrogue_lang_is_japanese
	&& !stat_msg
	&& !force_status_redraw
	)
	    return;

    force_status_redraw = FALSE;

    s_arm = temp;

    getyx(stdscr, oy, ox);
    if (s_hp != max_hp)
    {
	temp = max_hp;
	s_hp = max_hp;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }

    /*
     * Save current status
     */
    s_lvl = level;
    s_pur = purse;
    s_hp = pstats.s_hpt;
    s_str = pstats.s_str;
    s_player_lvl = pstats.s_lvl;
    s_hungry = hungry_state;
    s_color_mode = color_mode2;
    s_lang_is_japanese = jrogue_lang_is_japanese;

    jrogue_set_normal_attr();

    if (stat_msg)
    {
        move(0, 0);
        msg(JSTR("階: %d  体力: %*d(%*d)  強さ: %2d(%d)  金塊: %-5d  守備: %-2d  %s  %s", "Level: %d  Hits: %*d(%*d)  Str: %2d(%d)  Gold: %-5d  Armor: %-2d  %s  %s"),
        level, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
        max_stats.s_str, purse, 11 - s_arm,
        status_rank_name(),
        state_name[hungry_state]);
    }
    else
    {
	move(STATLINE, 0);
	status_set_attr(FALSE);

        printw(JSTR("階: %d  体力: %*d(%*d)  強さ: %2d(%d)  金塊: %-5d  守備: %-2d  %s", "Level: %d  Hits: %*d(%*d)  Str: %2d(%d)  Gold: %-5d  Armor: %-2d  %s"),
	    level, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
	    max_stats.s_str, purse, 11 - s_arm,
            status_rank_name());

	clrtoeol();
	if (hungry_state != 0)
	{
	    move(STATLINE, status_hunger_col());
	    status_set_attr(TRUE);
	    printw("%s", state_name[hungry_state]);
	    status_set_attr(FALSE);
	}
    }
    jrogue_set_normal_attr();
    move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(WINDOW *win, int ch)
{
    int c;

    if (ch == '\n')
        while ((c = wreadchar(win)) != '\n' && c != '\r')
	    continue;
    else
        while (wreadchar(win) != ch)
	    continue;
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 */
void
show_win(const char *message)
{
    WINDOW *win;

    win = hw;
    jrogue_wset_normal_attr(win);
    wmove(win, 0, 0);
    waddstr(win, message);
    touchwin(win);
    wrefresh(win);
    wait_for(win, ' ');
    clearok(curscr, TRUE);
    touchwin(stdscr);
}
