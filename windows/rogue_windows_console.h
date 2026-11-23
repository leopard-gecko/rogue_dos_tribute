/* Early Windows console initialization for the PDCursesMod VT backend.
 * Keep Win32 API details outside portable game sources. */
#ifndef ROGUE148_WINDOWS_CONSOLE_H
#define ROGUE148_WINDOWS_CONSOLE_H
#ifdef _WIN32
/* Include after curses.h (or after conio.h) in the calling file. */
#include <curses.h>
void jrogue_windows_prepare_console(void);
/* Classify an Explorer launch before curses; never resize a live shell. */
void jrogue_windows_resize_direct_launch(void);
void jrogue_windows_retry_direct_launch_resize(int restoring);
/* Synchronize PDCurses after a direct-launch ConPTY resize; OK or ERR. */
int jrogue_windows_sync_direct_launch_screen(void);
/* Curtain-only refresh/doupdate with terminal autowrap temporarily off.
 * Pass 0 for refresh(), 1 for doupdate().  Restores wrap after each frame. */
int jrogue_windows_curtain_update(int staged);
void jrogue_windows_sleep_ms(int milliseconds);
/* Keep PDCurses' visibility state, but respect the terminal profile's shape. */
void jrogue_windows_set_cursor_visible(int show);
int jrogue_windows_refresh_window(WINDOW *win);
/* Options additionally repaint Unicode glyphs before showing the cursor. */
int jrogue_windows_refresh_option_window(WINDOW *win);
void jrogue_windows_restore_terminal_cursor(void);
/* Raw scoreboard is printed after endwin(), not through PDCurses. */
void jrogue_windows_score_prepare(void);
int jrogue_windows_score_uses_vt(void);
void jrogue_windows_score_clear_plain_screen(void);

/* Returns 1 when the native console supplied a character/reading result;
 * returns 0 for non-console streams so the editor can use curses input. */
int jrogue_windows_read_option_key(int *result);
int jrogue_windows_encode_unicode(unsigned int cp, char out[5]);
/* Keep supplementary-plane characters intact when drawing an option. */
int jrogue_windows_waddstr_utf8(WINDOW *win, const char *text);
void jrogue_windows_option_vt_restore_nonbmp(WINDOW *win);
/* ANSI/xterm color numbers to PDCursesMod color IDs; before init_pair(). */
short jrogue_windows_palette_color(int color);
void jrogue_windows_init_palette(void);
#endif
#endif
