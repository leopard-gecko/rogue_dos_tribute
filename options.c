/*
 * This file has all the code for the option command.  I would rather
 * this command were not necessary, but it is the only way to keep the
 * wolves off of my back.
 *
 * @(#)options.c	4.24 (Berkeley) 05/10/83
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include "rogue.h"
#ifdef _WIN32
#include "windows/rogue_windows_console.h"
#endif

#define	EQSTR(a, b, c)	(strncmp(a, b, c) == 0)

#define	NUM_OPTS	(sizeof optlist / sizeof (OPTION))
#define OPT_REDRAW	3

/*
 * description of an option and what to do with it
 */
struct optstruct {
    const char	*o_name;	/* option name */
    const char	*o_prompt;	/* prompt for interactive entry */
    void 	*o_opt;		/* pointer to thing to set */
				/* function to print value */
    void 	(*o_putfunc)(void *opt);
				/* function to get value interactively */
    int		(*o_getfunc)(void *opt, WINDOW *win);
};

typedef struct optstruct	OPTION;

void	pr_optname(const OPTION *op);
static void put_lang(void *vp);
static int get_lang(void *vp, WINDOW *win);
static int get_macro(void *vp, WINDOW *win);
static int set_menu_option(const char *value, size_t len);
static void option_redraw(void);
static void option_refresh(WINDOW *win);

static OPTION	optlist[] = {
    {"terse",	 JSTR_STATIC("簡潔な表示", "Terse output"),
		 &terse,	put_bool,	get_bool	},
    {"jump",	 JSTR_STATIC("移動中の表示を行わない", "Show position only at end of run"),
		 &jump,		put_bool,	get_bool	},
    {"name",	 JSTR_STATIC("プレイヤー名", "Name"),
		 whoami,	put_str,	get_str		},
    {"fruit",	 JSTR_STATIC("好きな果物", "Fruit"),
		 fruit,		put_str,	get_str		},
    {"macro",	 JSTR_STATIC("マクロ", "Macro"),
		 dos_macro,	put_str,	get_macro	},
    {"color", JSTR_STATIC("カラーで表示", "Use color mode"),
		 &color_mode,	put_bool,	get_bool	},
    {"extchars", JSTR_STATIC("拡張文字", "Extended chars"),
		 &jrogue_char_mode_pc,	put_bool,	get_bool	},
    {"cursor", JSTR_STATIC("プレイ中のカーソルを表示", "Show cursor during play"),
		 &show_cursor,	put_bool,	get_bool	},
    {"language", JSTR_STATIC("表示する言語", "Language"),
		 &jrogue_lang_is_japanese,	put_lang,	get_lang	},
    {"file",	 JSTR_STATIC("セーブファイル名", "Save file"),
		 file_name,	put_str,	get_str		}
};
void
jrogue_apply_options_language(void)
{
    optlist[0].o_prompt = JSTR("簡潔な表示", "Terse output");
    optlist[1].o_prompt = JSTR("移動中の表示を行わない", "Show position only at end of run");
    optlist[2].o_prompt = JSTR("プレイヤー名", "Name");
    optlist[3].o_prompt = JSTR("好きな果物", "Fruit");
    optlist[4].o_prompt = JSTR("マクロ", "Macro");
    optlist[5].o_prompt = JSTR("カラーで表示", "Use color mode");
    optlist[6].o_prompt = JSTR("拡張文字", "Extended chars");
    optlist[7].o_prompt = JSTR("プレイ中のカーソルを表示", "Show cursor during play");
    optlist[8].o_prompt = JSTR("表示する言語", "Language");
    optlist[9].o_prompt = JSTR("セーブファイル名", "Save file");
}




/* PDCursesMod's VT backend may render a supplementary-plane character
 * using only its low 16 bits on the initial options display.  The editor
 * already repairs these cells after refresh; use that same refresh path for
 * the entire options screen, including its first paint and later redraws.
 * On macOS/Linux this remains an ordinary wrefresh(). */
static void
option_refresh(WINDOW *win)
{
    wrefresh(win);
#ifdef _WIN32
    jrogue_windows_option_vt_restore_nonbmp(win);
#endif
}

static void
option_redraw(void)
{
    const OPTION *op;

    wclear(hw);
    jrogue_wset_normal_attr(hw);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	pr_optname(op);
	(*op->o_putfunc)(op->o_opt);
	waddch2(hw, '\n');
    }
}


/*
 * option:
 *	Print and then set options from the terminal
 */

static int get_str_use_initial = FALSE;
static int get_str_allow_empty = FALSE;

void
option(void)
{
    const OPTION *op;
    int		retval;


    /*
     * Display current values of options
     */
    option_redraw();
    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
    {
	int old_color;

	pr_optname(op);
	get_str_use_initial = TRUE;
	old_color = color_mode;
	retval = (*op->o_getfunc)(op->o_opt, hw);
	get_str_use_initial = FALSE;
	if (op->o_opt == &color_mode && color_mode != old_color)
	{
	    /*
	     * Apply the terminal foreground/background policy immediately.
	     * In particular, switching color off from this screen must restore
	     * the terminal's own default colors instead of leaving the temporary
	     * black color-mode background in effect until the next command turn.
	     */
	    jrogue_apply_color_mode();
	    werase(hw);
	    option_redraw();
	    wmove(hw, (int)(op - optlist) + 1, 0);
	}
	if (retval)
	{
	    if (retval == QUIT)
		break;
	    else if (retval == OPT_REDRAW)
	    {
		option_redraw();
		wmove(hw, (int)(op - optlist) + 1, 0);
	    }
	    else if (op > optlist) {	/* MINUS */
		wmove(hw, (int)(op - optlist) - 1, 0);
		op -= 2;
	    }
	    else	/* trying to back up beyond the top */
	    {
		putchar('\007');
		wmove(hw, 0, 0);
		op--;
	    }
	}
    }
    /*
     * Switch back to original screen
     */
    wmove(hw, _LINES - 1, 0);
    waddstr(hw, JSTR("--スペースを押してください--", "--Press space to continue--"));
    option_refresh(hw);
    jrogue_show_cursor(TRUE);
    wait_for(hw, ' ');
    jrogue_restore_cursor();
    clearok(curscr, TRUE);
    touchwin(stdscr);
    refresh2();

    after = FALSE;
}

/*
 * pr_optname:
 *	Print out the option name prompt
 */

void
pr_optname(const OPTION *op)
{
    wprintw(hw, "%s (\"%s\"): ", op->o_prompt, op->o_name);
}

/*
 * put_bool
 *	Put out a boolean
 */

void
put_bool(void *b)
{
    waddstr(hw, *(int *) b ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
}

/*
 * put_str:
 *	Put out a string
 */

void
put_str(void *str)
{
#ifdef _WIN32
    jrogue_windows_waddstr_utf8(hw, (char *)str);
#else
    waddstr(hw, (char *)str);
#endif
}

static int
set_menu_option(const char *value, size_t len)
{
    char buf[MAXSTR];
    size_t i;

    if (len >= sizeof buf)
	len = sizeof buf - 1;
    for (i = 0; i < len; i++)
	buf[i] = (char)tolower((unsigned char)value[i]);
    buf[len] = '\0';

    if (strncmp(buf, "on", 2) == 0)
	jrogue_strlcpy(inventory_menu, "on", MAXSTR);
    else if (strncmp(buf, "sel", 3) == 0)
	jrogue_strlcpy(inventory_menu, "sel", MAXSTR);
    else if (strncmp(buf, "off", 3) == 0)
	jrogue_strlcpy(inventory_menu, "off", MAXSTR);
    else
	return FALSE;
    return TRUE;
}

static void
put_lang(void *vp)
{
    int *jp = (int *) vp;

    if (*jp)
	waddstr(hw, JSTR("日本語(J)", "Japanese(J)"));
    else
	waddstr(hw, JSTR("英語(E)", "English(E)"));
}

static int
get_lang(void *vp, WINDOW *win)
{
    int *jp = (int *) vp;
    int old_lang;
    int oy, ox;

    old_lang = *jp;
    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    put_lang(vp);

    for (;;)
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case 'j':
	    case 'J':
		*jp = TRUE;
		break;
	    case 'e':
	    case 'E':
		*jp = FALSE;
		break;
	    case '\n':
	    case '\r':
		wmove(win, oy, ox);
		put_lang(vp);
		wclrtoeol(win);
		waddch2(win, '\n');
		return NORM;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 15);
		waddstr(win, JSTR("［日本語(J)、英語(E)］", "(J or E)"));
		continue;
	}
	break;
    }

    jrogue_apply_language();
    wmove(win, oy, ox);
    put_lang(vp);
    wclrtoeol(win);
    waddch2(win, '\n');

    return (*jp != old_lang) ? OPT_REDRAW : NORM;
}


/*
 * get_bool:
 *	Allow changing a boolean option and print it out
 */
int
get_bool(void *vp, WINDOW *win)
{
    int *bp = (int *) vp;
    int oy, ox;
    int op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    jrogue_wset_normal_attr(win);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    while (op_bad)	
    {
	wmove(win, oy, ox);
	option_refresh(win);
	jrogue_show_cursor(TRUE);
	switch (wreadchar(win))
	{
	    case 't':
	    case 'T':
		*bp = TRUE;
		op_bad = FALSE;
		break;
	    case 'f':
	    case 'F':
		*bp = FALSE;
		op_bad = FALSE;
		break;
	    case '\n':
	    case '\r':
		op_bad = FALSE;
		break;
	    case ESCAPE:
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		wmove(win, oy, ox + 10);
		waddstr(win, JSTR("［有効(T)、無効(F)］", "(T or F)"));
	}
    }
    wmove(win, oy, ox);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    waddch2(win, '\n');
    return NORM;
}


/*
 * get_str:
 *	Set a string option
 */
#define MAXINP	48	/* max string to read from terminal or environment */


static int
str_read_utf8_char(WINDOW *win, int first, char *out)
{
    int i, need, c;

#ifdef _WIN32
    /* Native Windows input may already be a complete Unicode scalar.
     * The portable editor stores UTF-8 bytes, never a truncated wchar_t. */
    if (first > 255)
        return jrogue_windows_encode_unicode((unsigned int)first, out);
#endif
    if (first < 0 || first > 255)
	return 0;
    need = jrogue_utf8_sequence_bytes((unsigned char)first);
    out[0] = (char)first;

    for (i = 1; i < need; i++)
    {
	jrogue_prepare_input_attr(win);
	c = wgetch(win);
	if (c < 0 || c > 255 || (((unsigned char)c & 0xc0) != 0x80))
	    return i;
	out[i] = (char)c;
    }
    return need;
}

static void
str_redraw_edit(WINDOW *win, int y, int x, const char *buf, int pos)
{
    jrogue_wset_normal_attr(win);
    wmove(win, y, x);
#ifdef _WIN32
    jrogue_windows_waddstr_utf8(win, buf);
#else
    waddstr(win, buf);
#endif
    wclrtoeol(win);
    wmove(win, y, x + jrogue_text_width_n(buf, (size_t)pos));
    option_refresh(win);
}


/* Read a CSI sequence to its final byte, including optional modifiers.
 * The first numeric parameter distinguishes Delete (CSI 3~) from other
 * editing keys.  Unknown sequences must not be mistaken for a bare ESC:
 * that would close the entire options screen. */
static int
str_read_csi_edit_key(WINDOW *win)
{
    int c, i, first = -1, modifiers = FALSE;

    for (i = 0; i < 16; i++)
    {
	c = wgetch(win);
	if (c == ERR)
	    return ERR;
	if (c >= 0x40 && c <= 0x7e)  /* CSI final byte */
	{
	    if (c == 'D') return KEY_LEFT;
	    if (c == 'C') return KEY_RIGHT;
	    if (c == '~' && first == 3) return KEY_DC;
	    return ERR;
	}
	if (c == ';')
	    modifiers = TRUE;
	else if (!modifiers && c >= '0' && c <= '9')
	{
	    if (first < 0)
		first = 0;
	    if (first < 100)
		first = first * 10 + c - '0';
	}
    }
    return ERR;
}

static int
str_read_edit_key(WINDOW *win)
{
    int c, c2, c3;

#ifdef _WIN32
    /* A real Windows console stores typed characters as UTF-16, not UTF-8.
     * On mintty/pipe handles, retain the existing curses/ESC decoder. */
    if (jrogue_windows_read_option_key(&c))
        return c;
#endif
    c = wgetch(win);
    if (c != ESCAPE)
	return c;

    /* Allow the next byte a short time to arrive: escape-prefixed keypad
     * keys are sometimes delivered in separate terminal reads. */
    wtimeout(win, 100);
    c2 = wgetch(win);
    wtimeout(win, -1);       /* Return to the normal blocking input mode. */
    if (c2 == ERR)
	return ESCAPE;       /* Escape pressed by itself */

    /* Some terminals send a Meta-prefixed digit instead of plain ASCII. */
    if (c2 >= '0' && c2 <= '9')
	return c2;

    if (c2 == 'O')
    {
	c3 = wgetch(win);
	/* Application-keypad digit keys: SS3 p through SS3 y = 0..9. */
	if (c3 >= 'p' && c3 <= 'y')
	    return '0' + (c3 - 'p');
	if (c3 == 'D') return KEY_LEFT;
	if (c3 == 'C') return KEY_RIGHT;
	return ERR;
    }
    if (c2 == '[')
	return str_read_csi_edit_key(win);

    /* Any other escape-prefixed key is not an explicit cancel request. */
    return ERR;
}


int
get_str(void *vopt, WINDOW *win)
{
    char *opt = (char *) vopt;
    int oy, ox;
    int c, len, pos, next, clen;
    char chbuf[5];
    static char buf[MAXSTR];

    getyx(win, oy, ox);
    /* Use raw character input while editing.  With keypad enabled,
     * curses can translate numeric-keypad digits into KEY_C1, KEY_A1,
     * etc., which are not text.  We decode arrows/Delete locally. */
    keypad(win, FALSE);
    if (get_str_use_initial)
    {
	/*
	 * Only the option screen should edit the current value in-place.
	 * Prompts on the main screen, such as scroll/ring/stick naming,
	 * start with an empty input field.
	 */
	/* Keep a whole UTF-8 grapheme when an old value exceeds MAXINP. */
	len = (int)jrogue_text_prefix(opt, MAXINP);
	memcpy(buf, opt, (size_t)len);
	buf[len] = '\0';
    }
    else
    {
	buf[0] = '\0';
	len = 0;
    }
    pos = len;
    str_redraw_edit(win, oy, ox, buf, pos);
    jrogue_show_cursor(TRUE);

    /*
     * Loop reading in the string, and put it in a temporary buffer.
     * Prepare the terminal attribute immediately before wgetch(); this is
     * important for IME/preedit text, which may be drawn by the terminal
     * before curses receives the confirmed characters.
     */
    for (;;)
    {
	jrogue_prepare_input_attr(win);
	c = str_read_edit_key(win);

	if (c == '\n' || c == '\r' || c == ESCAPE)
	    break;
	if (c == -1)
	    continue;
	if (c == KEY_LEFT)
	{
	    pos = (int)jrogue_text_prev(buf, (size_t)pos);
	    str_redraw_edit(win, oy, ox, buf, pos);
	    continue;
	}
	if (c == KEY_RIGHT)
	{
	    pos = (int)jrogue_text_next(buf, (size_t)pos);
	    str_redraw_edit(win, oy, ox, buf, pos);
	    continue;
	}
	if (c == KEY_BACKSPACE || c == erasechar() || c == '\b' || c == 0177)
	{
	    if (pos > 0)
	    {
		int prev = (int)jrogue_text_prev(buf, (size_t)pos);
		memmove(&buf[prev], &buf[pos], (size_t)(len - pos + 1));
		len -= pos - prev;
		pos = prev;
	    }
	    str_redraw_edit(win, oy, ox, buf, pos);
	    continue;
	}
	if (c == KEY_DC)
	{
	    if (pos < len)
	    {
		next = (int)jrogue_text_next(buf, (size_t)pos);
		memmove(&buf[pos], &buf[next], (size_t)(len - next + 1));
		len -= next - pos;
	    }
	    str_redraw_edit(win, oy, ox, buf, pos);
	    continue;
	}
	if (c == killchar())	/* process kill character */
	{
	    len = 0;
	    pos = 0;
	    buf[0] = '\0';
	    str_redraw_edit(win, oy, ox, buf, pos);
	    continue;
	}
	if (len == 0 && pos == 0)
	{
	    if (c == '-' && win != stdscr)
		break;
	    else if (c == '~')
	    {
		len = (int)jrogue_text_prefix(home, MAXINP);
		memcpy(buf, home, (size_t)len);
		buf[len] = '\0';
		pos = len;
		str_redraw_edit(win, oy, ox, buf, pos);
		continue;
	    }
	}
#ifdef _WIN32
        /* Unicode scalars from the Windows console can exceed one byte. */
        if (c < 0 || c > 0x10ffff)
#else
	if (c < 0 || c > 255)
#endif
	{
	    putchar(CTRL('G'));
	    continue;
	}
	clen = str_read_utf8_char(win, c, chbuf);
	if (clen <= 0)
	{
	    putchar(CTRL('G'));
	    continue;
	}
	if (len + clen > MAXINP)
	    putchar(CTRL('G'));
	else
	{
	    memmove(&buf[pos + clen], &buf[pos], (size_t)(len - pos + 1));
	    memcpy(&buf[pos], chbuf, (size_t)clen);
	    len += clen;
	    pos += clen;
	    str_redraw_edit(win, oy, ox, buf, pos);
	}
    }
    keypad(win, TRUE);	/* Restore command/menu special-key decoding. */
    if (len > 0 || get_str_allow_empty)	/* usually only change if typed */
	jrogue_strlcpy(opt, buf, MAXSTR);
    jrogue_wset_normal_attr(win);
#ifdef _WIN32
    wmove(win, oy, ox);
    jrogue_windows_waddstr_utf8(win, opt);
    waddch(win, '\n');
#else
    mvwprintw(win, oy, ox, "%s\n", opt);
#endif
    option_refresh(win);
    if (win == stdscr)
	mpos += jrogue_text_width_n(buf, (size_t)len);
    if (c == '-')
	return MINUS;
    else if (c == ESCAPE)
	return QUIT;
    else
	return NORM;
}


static int
get_macro(void *vp, WINDOW *win)
{
    char buf[MAXSTR];
    int retval;
    int oy, ox;

    getyx(win, oy, ox);
    jrogue_strlcpy(buf, (char *)vp, sizeof buf);
    get_str_allow_empty = TRUE;
    retval = get_str(buf, win);
    get_str_allow_empty = FALSE;
    if (retval == NORM)
    {
	jrogue_set_macro(buf);
	wmove(win, oy, ox);
	waddstr(win, dos_macro);
	wclrtoeol(win);
	waddch2(win, '\n');
	option_refresh(win);
    }
    return retval;
}


#ifdef MASTER
/*
 * get_num:
 *	Get a numeric option
 */
int
get_num(void *vp, WINDOW *win)
{
    int *opt = (int *) vp;
    int i;
    static char buf[MAXSTR];

    if ((i = get_str(buf, win)) == NORM)
	*opt = atoi(buf);
    return i;
}
#endif

/*
 * parse_opts:
 *	Parse options from string, usually taken from the environment.
 *	The string is a series of comma seperated values, with booleans
 *	being stated as "name" (true) or "noname" (false), and strings
 *	being "name=....", with the string being defined up to a comma
 *	or the end of the entire option string.
 */

void
parse_opts(char *str)
{
    char *sp;
    const OPTION *op;
    int len;
    char *start;

    while (*str)
    {
	/*
	 * Get option name
	 */
	for (sp = str; isalpha((int)*sp); sp++)
	    continue;
	len = (int)(sp - str);
	/*
	 * ``doskeys'' is intentionally hidden from the interactive option
	 * screen.  It selects the DOS-style key bindings at startup.
	 */
	if (len == 7 && EQSTR(str, "doskeys", 7))
	{
	    dos_keys = TRUE;
	}
	else if (len == 9 && EQSTR(str, "nodoskeys", 9))
	{
	    dos_keys = FALSE;
	}
	/*
	 * ``menu'' is intentionally hidden from the interactive option
	 * screen, but remains available through ROGUEOPTS.
	 */
	else if (len == 4 && EQSTR(str, "menu", 4) && *sp == '=')
	{
	    char save_ch;

	    for (str = sp + 1; *str == '='; str++)
		continue;
	    for (sp = str; *sp && *sp != ','; sp++)
		continue;
	    save_ch = *sp;
	    *sp = '\0';
	    set_menu_option(str, strlen(str));
	    *sp = save_ch;
	}
	/*
	 * ``macro'' initializes the command macro.  The same value is also
	 * editable from the interactive option screen.
	 */
	else if (len == 5 && EQSTR(str, "macro", 5) && *sp == '=')
	{
	    char save_ch;

	    for (str = sp + 1; *str == '='; str++)
		continue;
	    for (sp = str; *sp && *sp != ','; sp++)
		continue;

	    save_ch = *sp;
	    *sp = '\0';
	    jrogue_set_macro(str);
	    *sp = save_ch;
	}
	/*
	 * Look it up and deal with it
	 */
	else for (op = optlist; op <= &optlist[NUM_OPTS-1]; op++)
	    if (EQSTR(str, op->o_name, len))
	    {
		if (op->o_putfunc == put_bool)	/* if option is a boolean */
		    *(int *)op->o_opt = TRUE;	/* NOSTRICT */
		else				/* string option */
		{
		    /*
		     * Skip to start of string value
		     */
		    for (str = sp + 1; *str == '='; str++)
			continue;
		    if (*str == '~')
		    {
			jrogue_strlcpy((char *) op->o_opt, home, MAXSTR);	  /* NOSTRICT */
			start = (char *) op->o_opt + strlen(home);/* NOSTRICT */
			while (*++str == '/')
			    continue;
		    }
		    else
			start = (char *) op->o_opt;	/* NOSTRICT */
		    /*
		     * Skip to end of string value
		     */
		    for (sp = str + 1; *sp && *sp != ','; sp++)
			continue;
		    /*
		     * handle options with special string values
		     */
		    if (op->o_putfunc == put_lang)
		    {
			char save_ch;

			save_ch = *sp;
			*sp = '\0';
			if (jrogue_set_language(str))
			    jrogue_apply_language();
			*sp = save_ch;
		    }
		    else
			strucpy2(start, str, (size_t)(sp - str));
		}
		break;
	    }
	    /*
	     * check for "noname" for booleans
	     */
	    else if (op->o_putfunc == put_bool
	      && EQSTR(str, "no", 2) && EQSTR(str + 2, op->o_name, len - 2))
	    {
		*(int *)op->o_opt = FALSE;	/* NOSTRICT */
		break;
	    }

	/*
	 * skip to start of next option name
	 */
	while (*sp && !isalpha((int)*sp))
	    sp++;
	str = sp;
    }
}

/*
 * strucpy:
 *	Copy string using unctrl for things
 */

void
strucpy(char *s1, const char *s2, size_t len)
{
    if (len > MAXINP)
	len = MAXINP;
    while (len--)
    {
	if (isprint((int)*s2) || *s2 == ' ')
	    *s1++ = *s2;
	s2++;
    }
    *s1 = '\0';
}

void
strucpy2(char *s1, const char *s2, size_t len)
{
    if (len > MAXINP)
	len = MAXINP;
    while (len--)
    {
	*s1++ = *s2;
	s2++;
    }
    *s1 = '\0';
}
