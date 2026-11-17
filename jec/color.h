/*
 * JEC color, screen glyph and drawing helpers for Rogue DOS Tribute.
 * Implementations are moved from extern.c without changing rendering rules.
 * This header is included by extern.h after curses types are available.
 */
#ifndef JROGUE_COLOR_H
#define JROGUE_COLOR_H

#define JROGUE_PAIR_NORMAL 7
#define JROGUE_PAIR_PROJECTILE_RED 44
#define JROGUE_PAIR_PROJECTILE_CYAN 45
#define JROGUE_PAIR_PROJECTILE_YELLOW 46
#define JROGUE_PAIR_STATUS 49
#define JROGUE_PAIR_PLAYER_PASSAGE 54
#define JROGUE_PAIR_ASCII_WALL 55

extern int color_mode;
extern int color_mode2;
extern int jrogue_char_mode_pc;
extern int ccolor[0x60];
extern int fcolor[];
extern int bcolor[];
extern int hcolor[];
extern int mystout;

void refresh2(void);
void start_custom_color(void);
void jrogue_apply_color_mode(void);
void restore_terminal_background(void);
const char *jrogue_black_background_ansi(void);
chtype jrogue_normal_attr(void);
void jrogue_set_normal_attr(void);
void jrogue_wset_normal_attr(WINDOW *wn);
void jrogue_wset_map_attr(WINDOW *wn, int ch);
void jrogue_prepare_input_attr(WINDOW *wn);
void mvaddch_projectile(int y, int x, int ch, int pair);
void jrogue_clear_screen_chars(void);
void jrogue_remember_ch(int y, int x, chtype c);
int jrogue_mvinch_char(int y, int x);
const char *jrogue_color_glyph_for(int y, int x, int ch, int map_context);
/* Draw one UTF-8 scalar on the specified window or on stdscr.
 * Windows passes one Unicode scalar to PDCursesMod; macOS/Linux retain
 * their UTF-8 waddstr() path. Current attributes and cursor are preserved. */
int jrogue_wadd_utf8_glyph(WINDOW *win, const char *glyph);
int jrogue_add_utf8_glyph(const char *glyph);
/* Update a curtain frame: staged = 1 for doupdate(), 0 for refresh().
 * Windows guards VT terminal auto-wrap at the bottom-right screen cell. */
int jrogue_curtain_update(int staged);
int addch2(const chtype c);
int waddch2(WINDOW *wn, const chtype c);
int mvaddch2(int y, int x, const chtype c);
int mvwaddch2(WINDOW *wn, int y, int x, const chtype c);
int jrogue_should_reverse_map_cell(int y, int x, int ch);
int jrogue_addch_map(const chtype c);
int jrogue_mvaddch_map(int y, int x, const chtype c);
int standout2(void);
int wstandout2(WINDOW *wn);
int standend2(void);
int wstandend2(WINDOW *wn);
#endif /* JROGUE_COLOR_H */
