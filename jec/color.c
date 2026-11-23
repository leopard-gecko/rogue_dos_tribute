/*
 * JEC color palette, DOS-style screen glyphs, and curses drawing helpers.
 * Moved from extern.c in JEC.091; visual behavior is unchanged.
 * See LICENSE.TXT for copyright and licensing information.
 */
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "../rogue.h"
#ifdef _WIN32
#include "../windows/rogue_windows_console.h"
#endif

int	color_mode = TRUE;	/* Color Char Mode */
int	jrogue_char_mode_pc = TRUE;	/* Use extended characters */
int	color_mode2 = FALSE;
int	ccolor[0x60] =
{
/*      !  "  #  $  %  &  '  (  )  *  +  ,  -  .  /  */
     0, 13, 13, 42, 13, 48, 13, 13, 13, 13, 49, 12, 13, 12, 41, 13,
/*   0  1  2  3  4  5  6  7  8  9  :  ;  <  =  >  ?  */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 43, 13, 13, 13, 13, 13,
/*   @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  */
    49, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
/*   P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _  */
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 13, 13, 13, 14, 13,
/*   `  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  */
    13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*   p  q  r  s  t  u  v  w  x  y  z  {  |  }  ~     */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 13, 12, 13, 13, 13

};

/*		          0   1   2   3   4	  5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  */
int fcolor[] = {   0,   1,   2,   3,   4,   5,   6,   7,   2,   3, 255, 248, 130,  75, 161,  14, 144, 223,  46, 130,  70, 252, 108, 117, 112, 166,  49, 207, 123, 153,  20, 189, 214,  75, 131, 178,  33,  59, 160, 117,  99,  65, 242, 167, 210, 117, 222, 173,  16, 222, 137, 167, 108, 186, 229, 178 };
int hcolor[] = {   0,   1,   1,   0,   1,   1,   1,   0,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1 };
int bcolor[] = {  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,  -1,  -1,  17,  17,   0,   0,   0,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,   0,   0,   0,  17,  17,  17,   0,  22,   0,   0,   0,   0,   0, 244,   0 };

int mystout = FALSE;
static int jrogue_screen_chars[NUMLINES][NUMCOLS];
static int jrogue_screen_chars_inited = FALSE;
#ifndef _WIN32
static int color_background_changed = FALSE;
#endif
static void apply_color_background(void);
void restore_terminal_background(void);


#ifndef _WIN32
static void
write_ansi(const char *seq)
{
    fputs(seq, stdout);
    fflush(stdout);
}
#endif

/* Draw one UTF-8 scalar at the current position in any curses window.
 * A common interface is used by the startup curtain, floor transition,
 * grave border and ending curtain.  Only the curses encoding boundary is
 * platform-specific; color attributes and cursor placement belong to callers. */
int
jrogue_wadd_utf8_glyph(WINDOW *win, const char *glyph)
{
    unsigned int scalar;

    if (win == NULL || !jrogue_utf8_single_scalar(glyph, &scalar))
        return ERR;
#ifdef _WIN32
    /* PDCursesMod accepts a BMP box-drawing/shading scalar as one chtype.
     * Never send the bytes of a multibyte glyph separately. */
    return waddch(win, (chtype)scalar);
#else
    /* Preserve the established UTF-8 ncurses/macOS text path. */
    (void)scalar;
    return waddstr(win, glyph);
#endif
}

int
jrogue_add_utf8_glyph(const char *glyph)
{
    return jrogue_wadd_utf8_glyph(stdscr, glyph);
}

/* Guard the physical rightmost/bottommost cell only while animating a
 * full-screen curtain. The Windows backend temporarily disables VT wrap.
 * Ordinary refreshes elsewhere in the game retain their original behavior. */
int
jrogue_curtain_update(int staged)
{
#ifdef _WIN32
    return jrogue_windows_curtain_update(staged);
#else
    return staged ? doupdate() : refresh();
#endif
}

static int
jrogue_color_index(chtype c, int *ccp)
{
    int ch;

    ch = (int) CCHAR(c);
    if (!jrogue_char_mode_pc && (ch == '|' || ch == '-' || ch == DOOR))
    {
	*ccp = JROGUE_PAIR_ASCII_WALL;
	return TRUE;
    }
    if (ch < 0x20 || ch >= 0x20 + (int)(sizeof ccolor / sizeof ccolor[0]))
	return FALSE;
    *ccp = ccolor[ch - 0x20];
    return TRUE;
}

static chtype
jrogue_color_attr_for_index(int cc, int reverse)
{
    chtype attr;

    switch (hcolor[cc])
    {
    case 2:
	attr = COLOR_PAIR(cc) | A_REVERSE | A_BLINK;
	break;
    case 3:
	attr = COLOR_PAIR(cc) | A_REVERSE;
	break;
    default:
	attr = (cc == 0) ? jrogue_normal_attr() : (COLOR_PAIR(cc) | A_NORMAL);
	break;
    }
    if (reverse)
	attr |= A_REVERSE;
    return attr;
}

static int
jrogue_player_highlight_cell(int y, int x, int ch, int map_context)
{
    if (!map_context || CCHAR(ch) != PLAYER)
	return FALSE;
    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
	return FALSE;

    /*
     * The DOS 1.48 code draws the player with the same standout attribute
     * when he is on a passage/maze cell and when was_trapped asks for a
     * temporary trap highlight.
     */
    return ((flat(y, x) & (F_PASS | F_MAZE)) != 0 || mystout);
}

static chtype
jrogue_player_highlight_attr(void)
{
    if (!jrogue_char_mode_pc)
	return COLOR_PAIR(ccolor[PLAYER - 0x20]) | A_REVERSE;
    return COLOR_PAIR(JROGUE_PAIR_PLAYER_PASSAGE) | A_NORMAL;
}

int
jrogue_should_reverse_map_cell(int y, int x, int ch)
{
    int fl;

    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
	return FALSE;
    fl = flat(y, x);
    if ((fl & (F_PASS | F_MAZE)) == 0)
	return FALSE;
    ch = CCHAR(ch);
    if (ch == PASSAGE || ch == ARMOR || ch == ' ')
	return FALSE;
    return TRUE;
}

int
jrogue_addch_map(const chtype c)
{
    int y, x, rc;

    getyx(stdscr, y, x);
    if (jrogue_should_reverse_map_cell(y, x, CCHAR(c)))
    {
	standout2();
	rc = addch2(c);
	standend2();
	return rc;
    }
    return addch2(c);
}

int
jrogue_mvaddch_map(int y, int x, const chtype c)
{
    int rc;

    if (jrogue_should_reverse_map_cell(y, x, CCHAR(c)))
    {
	standout2();
	rc = mvaddch2(y, x, c);
	standend2();
	return rc;
    }
    return mvaddch2(y, x, c);
}

static void
jrogue_init_screen_chars(void)
{
    int y, x;

    if (jrogue_screen_chars_inited)
        return;
    for (y = 0; y < NUMLINES; y++)
        for (x = 0; x < NUMCOLS; x++)
            jrogue_screen_chars[y][x] = ' ';
    jrogue_screen_chars_inited = TRUE;
}

void
jrogue_clear_screen_chars(void)
{
    int y, x;

    jrogue_init_screen_chars();
    for (y = 0; y < NUMLINES; y++)
        for (x = 0; x < NUMCOLS; x++)
            jrogue_screen_chars[y][x] = ' ';
}

void
jrogue_remember_ch(int y, int x, chtype c)
{
    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
        return;
    jrogue_init_screen_chars();
    jrogue_screen_chars[y][x] = CCHAR(c);
}

int
jrogue_mvinch_char(int y, int x)
{
    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
        return ' ';
    jrogue_init_screen_chars();
    return jrogue_screen_chars[y][x];
}

static int
jrogue_map_cell(int y, int x)
{
    if (y < 0 || y >= NUMLINES || x < 0 || x >= NUMCOLS)
        return ' ';
    return chat(y, x);
}

static int
jrogue_wall_like(int y, int x)
{
    int ch;

    ch = jrogue_map_cell(y, x);
    return ch == '|' || ch == '-' || ch == DOOR;
}

static const char *
jrogue_wall_glyph(int y, int x, int ch, int map_context)
{
    int up, down, left, right;

    if (ch == '|')
        return "║";
    if (ch != '-')
        return NULL;

    if (!map_context)
        return "═";

    up = jrogue_wall_like(y - 1, x);
    down = jrogue_wall_like(y + 1, x);
    left = jrogue_wall_like(y, x - 1);
    right = jrogue_wall_like(y, x + 1);

    if (right && down && !left && !up)
        return "╔";
    if (left && down && !right && !up)
        return "╗";
    if (right && up && !left && !down)
        return "╚";
    if (left && up && !right && !down)
        return "╝";
    return "═";
}

const char *
jrogue_color_glyph_for(int y, int x, int ch, int map_context)
{
    if (!jrogue_char_mode_pc || MB_CUR_MAX <= 1)
        return NULL;

    ch = CCHAR(ch);
    switch (ch)
    {
    case '|':
    case '-':
        return jrogue_wall_glyph(y, x, ch, map_context);
    case DOOR:
        return "╬";
    case PASSAGE:
        return "░";
    case STAIRS:
        return "≡";
    case FLOOR:
        return "·";
    case PLAYER:
        return "☺";
    case TRAP:
        return "♦";
    case GOLD:
        return "☼";
    case POTION:
        return "¡";
    case SCROLL:
        return "♪";
    case FOOD:
        return "♣";
    case STICK:
        return "τ";
    case ARMOR:
        return "◘";
    case AMULET:
        return "♀";
    case RING:
        return "○";
    case WEAPON:
        return "↑";
    default:
        return NULL;
    }
}

static int
jrogue_can_use_true_black_background(void)
{
    return COLORS > 16;
}

const char *
jrogue_black_background_ansi(void)
{
    return jrogue_can_use_true_black_background()
        ? "\033[48;5;16m"
        : "\033[40m";
}

static void
jrogue_use_true_black_background_pairs(void)
{
    int i;

    if (!jrogue_can_use_true_black_background())
        return;

    for (i = 1; i < (int)(sizeof fcolor / sizeof fcolor[0]); i++)
        if (bcolor[i] == COLOR_BLACK || bcolor[i] == -1)
            bcolor[i] = 16;
}


chtype
jrogue_normal_attr(void)
{
    return COLOR_PAIR(JROGUE_PAIR_NORMAL) | A_NORMAL;
}

void
jrogue_set_normal_attr(void)
{
    if (color_mode2 != FALSE)
	attrset(jrogue_normal_attr());
    else
	attrset(A_NORMAL);
}

void
jrogue_wset_normal_attr(WINDOW *wn)
{
    if (color_mode2 != FALSE)
	wattrset(wn, jrogue_normal_attr());
    else
	wattrset(wn, A_NORMAL);
}

void
jrogue_wset_map_attr(WINDOW *wn, int ch)
{
    int cc;

    if (color_mode2 != FALSE && jrogue_color_index(ch, &cc))
	wattrset(wn, jrogue_color_attr_for_index(cc, FALSE));
    else
	jrogue_wset_normal_attr(wn);
}

void
jrogue_prepare_input_attr(WINDOW *wn)
{
    if (wn != NULL) {
	jrogue_wset_normal_attr(wn);
	jrogue_refresh_window(wn);
    }
#ifndef _WIN32
    if (color_mode != FALSE && color_mode2 != FALSE) {
	write_ansi("\033]10;#c0c0c0\007");
	write_ansi("\033]11;#000000\007");
	write_ansi("\033[0m\033[37m");
	write_ansi(jrogue_black_background_ansi());
    }
#endif
}

static void
apply_color_background(void)
{
    if (color_mode) {
        /*
         * Match the color-mode behavior used by the Rogue Clone III macOS
         * port: only while color is enabled, ask terminal emulators that
         * understand OSC 11 to use a black background, force ANSI black
         * background, and make curses erase/blank cells use the normal
         * black-background color pair.
         */
#ifndef _WIN32
        write_ansi("\033]10;#c0c0c0\007");
        write_ansi("\033]11;#000000\007");
        color_background_changed = TRUE;
        write_ansi("\033[37m");
        write_ansi(jrogue_black_background_ansi());
#endif
        bkgd(' ' | jrogue_normal_attr());
        jrogue_set_normal_attr();
    }
    else {
        /*
         * In monochrome mode, do not force a black game background.
         * Leave normal cells as the terminal profile's own foreground and
         * background colors, and undo any OSC/ANSI black background that may
         * have been applied while color mode was enabled.
         */
        bkgd(' ' | A_NORMAL);
        attrset(A_NORMAL);
#ifndef _WIN32
        if (color_background_changed) {
            write_ansi("\033]110\007");
            write_ansi("\033]111\007");
            color_background_changed = FALSE;
        }
        write_ansi("\033[39;49m");
#endif
    }
}


void
jrogue_apply_color_mode(void)
{
    color_mode = color_mode ? TRUE : FALSE;
    color_mode2 = color_mode;
    apply_color_background();
    jrogue_set_normal_attr();
}


void
restore_terminal_background(void)
{
    if (!isendwin()) {
        bkgd(' ' | A_NORMAL);
        attrset(A_NORMAL);
    }
#ifndef _WIN32
    if (color_background_changed) {
        write_ansi("\033]110\007");
        write_ansi("\033]111\007");
        color_background_changed = FALSE;
    }
    write_ansi("\033[39;49m");
#endif
}

void
refresh2(void)
{
    int i, j;

    jrogue_apply_color_mode();
    /* The top message line may contain Japanese and emoji.  Rewriting
     * mvinch() values with & 0x7f destroys their UTF-8/scalar data.
     * Let PDCurses keep the existing line when switching color mode. */
#ifndef _WIN32
    for (i = 0; i < NUMCOLS; i++)
	mvaddch(0, i, mvinch(0, i) & 0x7f);
#endif
    for (i = 0; i < NUMCOLS; i++)
	for (j = 1; j < STATLINE; j++)
	    jrogue_mvaddch_map(j, i, jrogue_mvinch_char(j, i));

    /*
     * The status line is intentionally not redrawn by the map loop above.
     * When color is switched off, its old color-pair attributes can otherwise
     * remain visible until some numeric status value changes.
     */
    move(STATLINE, 0);
    jrogue_set_normal_attr();
    clrtoeol();
    force_status();
    status();
}

void
start_custom_color(void)
{
    int i;

    start_color();
    use_default_colors();
#ifdef _WIN32
    jrogue_windows_init_palette();
#endif

    /*
     * Some terminals display curses/ANSI color 0 as dark gray instead of
     * true black.  Also, cells drawn with the default background (-1) can
     * reveal the terminal profile background instead of the game background.
     * On 256-color terminals, use xterm color 16 for every black/default
     * background cell so color mode remains consistently black.
     */
    jrogue_use_true_black_background_pairs();

    /*
     * Color 17 is used as the blue background for floor-like cells.
     * Make it darker on terminals that allow palette changes.
     */
#ifndef _WIN32
    if (can_change_color() && COLORS > 16)
	init_color(16, 0, 0, 0);
    if (can_change_color() && COLORS > 17)
	init_color(17, 0, 0, 250);
#endif
/*
    init_color( 1,    0,    0,  500);
    init_color( 2,    0,  500,    0);
    init_color( 3,    0,  500,  500);
    init_color( 4,  500,    0,    0);
    init_color( 5,  500,    0,  500);
    init_color( 6,  500,  500,    0);
    init_color( 7,  500,  500,  500);
*/
    for (i = 1; i < (int)(sizeof fcolor / sizeof fcolor[0]); i++) {
#ifdef _WIN32
        init_pair((short)i, jrogue_windows_palette_color(fcolor[i]),
                  jrogue_windows_palette_color(bcolor[i]));
#else
	init_pair((short)i, (short)fcolor[i], (short)bcolor[i]);
#endif
    }

    apply_color_background();
}


void
mvaddch_projectile(int y, int x, int ch, int pair)
{
    if (color_mode2 != FALSE) {
	attrset(COLOR_PAIR(pair) | A_NORMAL);
	mvaddch(y, x, ch);
	jrogue_set_normal_attr();
    }
    else {
	/*
	 * Beam/projectile glyphs are temporary effects, not map cells.
	 * In monochrome mode, do not route them through mvaddch2(),
	 * because that applies the PC wall-glyph conversion and would
	 * turn '-' and '|' beams into the horizontal/vertical wall glyphs.
	 */
	attrset(A_NORMAL);
	mvaddch(y, x, ch);
    }
}

int
addch2(const chtype c)
{
    int cc, rc;
    int y, x;
    const char *glyph;

    getyx(stdscr, y, x);
    jrogue_remember_ch(y, x, c);
    if (color_mode2 != FALSE && jrogue_player_highlight_cell(y, x, CCHAR(c), TRUE))
	attrset(jrogue_player_highlight_attr());
    else if (color_mode2 != FALSE && jrogue_color_index(c, &cc))
	attrset(jrogue_color_attr_for_index(cc, mystout));
    else if (mystout)
	attron(A_REVERSE);
    glyph = jrogue_color_glyph_for(y, x, CCHAR(c), TRUE);
    if (glyph != NULL)
	rc = addstr(glyph);
    else
	rc = addch(c);
    if (color_mode2 == FALSE)
	standend();
    else
	jrogue_set_normal_attr();
    return rc;
}

int
waddch2(WINDOW *wn, const chtype c)
{
    int cc, rc;
    int y, x;
    const char *glyph;

    getyx(wn, y, x);
    if (wn == stdscr)
	jrogue_remember_ch(y, x, c);
    if (color_mode2 != FALSE && jrogue_player_highlight_cell(y, x, CCHAR(c), wn == stdscr))
	wattrset(wn, jrogue_player_highlight_attr());
    else if (color_mode2 != FALSE && jrogue_color_index(c, &cc))
	wattrset(wn, jrogue_color_attr_for_index(cc, mystout));
    else if (mystout)
	wattron(wn, A_REVERSE);
    glyph = jrogue_color_glyph_for(y, x, CCHAR(c), wn == stdscr);
    if (glyph != NULL)
	rc = waddstr(wn, glyph);
    else
	rc = waddch(wn, c);
    if (color_mode2 == FALSE)
	standend();
    else
	jrogue_wset_normal_attr(wn);
    return rc;
}

int
mvaddch2(int y, int x, const chtype c)
{
    int cc, rc;
    const char *glyph;

    jrogue_remember_ch(y, x, c);
    if (color_mode2 != FALSE && jrogue_player_highlight_cell(y, x, CCHAR(c), TRUE))
	attrset(jrogue_player_highlight_attr());
    else if (color_mode2 != FALSE && jrogue_color_index(c, &cc))
	attrset(jrogue_color_attr_for_index(cc, mystout));
    else if (mystout)
	attron(A_REVERSE);
    glyph = jrogue_color_glyph_for(y, x, CCHAR(c), TRUE);
    if (glyph != NULL) {
	move(y, x);
	rc = addstr(glyph);
    }
    else
	rc = mvaddch(y, x, c);
    if (color_mode2 == FALSE)
	standend();
    else
	jrogue_set_normal_attr();
    return rc;
}

int
mvwaddch2(WINDOW *wn, int y, int x, const chtype c)
{
    int cc, rc;
    const char *glyph;

    if (wn == stdscr)
	jrogue_remember_ch(y, x, c);
    if (color_mode2 != FALSE && jrogue_player_highlight_cell(y, x, CCHAR(c), wn == stdscr))
	wattrset(wn, jrogue_player_highlight_attr());
    else if (color_mode2 != FALSE && jrogue_color_index(c, &cc))
	wattrset(wn, jrogue_color_attr_for_index(cc, mystout));
    else if (mystout)
	wattron(wn, A_REVERSE);
    glyph = jrogue_color_glyph_for(y, x, CCHAR(c), wn == stdscr);
    if (glyph != NULL)
	rc = mvwaddstr(wn, y, x, glyph);
    else
	rc = mvwaddch(wn, y, x, c);
    if (color_mode2 == FALSE)
	standend();
    else
	jrogue_wset_normal_attr(wn);
    return rc;
}

int
standout2(void)
{
    int rc;

    if (color_mode2 != FALSE)
    {
	attron(A_REVERSE);
	rc = OK;
    }
    else
	rc = standout();
    mystout = TRUE;
    return rc;
}

int
wstandout2(WINDOW *wn)
{
    int rc;

    if (color_mode2 != FALSE)
    {
	wattron(wn, A_REVERSE);
	rc = OK;
    }
    else
	rc = wstandout(wn);
    mystout = TRUE;
    return rc;
}

int
standend2(void)
{
    int rc;

    if (color_mode2 != FALSE)
    {
	jrogue_set_normal_attr();
	rc = OK;
    }
    else
	rc = standend();
    mystout = FALSE;
    return rc;
}

int
wstandend2(WINDOW *wn)
{
    int rc;

    if (color_mode2 != FALSE)
    {
	jrogue_wset_normal_attr(wn);
	rc = OK;
    }
    else
	rc = wstandend(wn);
    mystout = FALSE;
    return rc;
}
