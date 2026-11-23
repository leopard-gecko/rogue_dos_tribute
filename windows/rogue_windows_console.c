/* Windows native terminal bootstrap, separate from DOS-style game input.
 * VT setup, Unicode input, private-console sizing, and score display stay
 * confined here; the portable game renders through common interfaces. */
#ifdef _WIN32
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <conio.h> /* Before curses.h: PDCurses defines getch as a macro. */
#undef MOUSE_MOVED /* windows.h and PDCurses both define this. */
#include <curses.h>
#include "rogue_windows_console.h"

/* Windows-only direct launches get a private 80x24 console.  Never resize
 * an interactive shell, including MSYS2 UCRT64 or a shared ConPTY session.
 * The classification and two resize paths follow Rogue 5.4.5 JEC. */
#define JROGUE_WIN_COLS 80
#define JROGUE_WIN_ROWS 24

/* PDCursesMod VT selects an underline on curs_set(1) and a block on
 * endwin().  Neither is necessarily the user's terminal cursor shape.
 * Restore the profile's default shape (DECSCUSR 0) after rendering, while
 * keeping cursor visibility a separate game option.  The cache avoids
 * resetting visibility on every game turn. */
static int game_cursor_visibility = -1;
static int cursor_resume_pending = 0;

static HANDLE
cursor_vt_output(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    const char *term;

    if (out == NULL || out == INVALID_HANDLE_VALUE)
        return NULL;
    if (GetConsoleMode(out, &mode)) {
        if ((mode & (ENABLE_PROCESSED_OUTPUT |
                     ENABLE_VIRTUAL_TERMINAL_PROCESSING)) ==
                    (ENABLE_PROCESSED_OUTPUT |
                     ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            return out;
        return NULL;
    }
    /* MSYS2/mintty stdout is a VT pipe, not a Win32 console.  Avoid
     * emitting escape codes when stdout is redirected to a regular file. */
    term = getenv("TERM");
    if (GetFileType(out) == FILE_TYPE_PIPE && getenv("MSYSTEM") != NULL &&
        term != NULL && *term != '\0' && strcmp(term, "dumb") != 0)
        return out;
    return NULL;
}

static void
cursor_write_vt(HANDLE out, const char *seq, DWORD len)
{
    DWORD written;
    fflush(stdout);  /* Drain pending CRT/PDCurses output first. */
    (void)WriteFile(out, seq, len, &written, NULL);
}

/* DECSCUSR 0 changes shape only, never the visible/hidden state. */
static void
cursor_restore_default_shape(void)
{
    static const char default_shape[] = "\033[0 q";
    HANDLE out = cursor_vt_output();
    if (out != NULL)
        cursor_write_vt(out, default_shape, sizeof default_shape - 1);
}

static void
cursor_resume_vt_processing(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    /* PDCursesMod disables VT processing in endwin().  Re-enable it before
     * the first curses drawing or visibility command after shell escape. */
    if (out != NULL && out != INVALID_HANDLE_VALUE &&
        GetConsoleMode(out, &mode))
        (void)SetConsoleMode(out, mode | ENABLE_PROCESSED_OUTPUT |
                                        ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void
jrogue_windows_set_cursor_visible(int show)
{
    int visible = show ? 1 : 0;
    if (cursor_resume_pending)
        cursor_resume_vt_processing();
    if (game_cursor_visibility == visible)
        return;
    (void)curs_set(visible);
    game_cursor_visibility = visible;
    /* curs_set(1) selects an underline on PDCursesMod's VT backend. */
    if (visible)
        cursor_restore_default_shape();
}

static int
jrogue_windows_refresh_impl(WINDOW *win, int repaint_options)
{
    static const char hide[] = "\033[?25l";
    static const char show[] = "\033[?25h";
    HANDLE out = NULL;
    int result;

    if (cursor_resume_pending)
        cursor_resume_vt_processing();

    /* Temporarily hide the physical cursor while PDCurses renders.
     * Do not toggle its internal curses visibility state: after rendering,
     * the cursor must still be visible at the correct logical position. */
    if (game_cursor_visibility == 1)
        out = cursor_vt_output();
    if (out != NULL)
        cursor_write_vt(out, hide, sizeof hide - 1);
    result = wrefresh(win);
    if (result != ERR) {
        (void)napms(0);  /* Drain startup palette's delayed repaint. */
        if (repaint_options)
            jrogue_windows_option_vt_restore_nonbmp(win);
    }
    if (out != NULL)
        cursor_write_vt(out, show, sizeof show - 1);
    if (result == ERR)
        return result;

    if (cursor_resume_pending) {
        /* First curses refresh after endwin can reset cursor visibility. */
        if (game_cursor_visibility >= 0)
            (void)curs_set(game_cursor_visibility);
        cursor_resume_pending = 0;
    }

    /* MUST follow the final curses redraw; the backend can set underline
     * again during refresh, including startup or shell resume. */
    if (game_cursor_visibility == 1)
        cursor_restore_default_shape();
    return result;
}

/* Option text may contain non-BMP emoji.  Keep the cursor hidden until
 * their supplementary VT repaint also finishes, as in the 3.6 redraw path. */
int
jrogue_windows_refresh_option_window(WINDOW *win)
{
    return jrogue_windows_refresh_impl(win, 1);
}

int
jrogue_windows_refresh_window(WINDOW *win)
{
    return jrogue_windows_refresh_impl(win, 0);
}

void
jrogue_windows_restore_terminal_cursor(void)
{
    static const char restore[] = "\033[0 q\033[?25h";
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, wanted;

    /* endwin() forces a block cursor, then disables VT mode.  Re-enable
     * VT temporarily so both Console Host and Windows Terminal honor the
     * profile-default shape; restore the old console mode afterwards. */
    if (out != NULL && out != INVALID_HANDLE_VALUE &&
        GetConsoleMode(out, &mode)) {
        wanted = mode | ENABLE_PROCESSED_OUTPUT |
                        ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (wanted == mode || SetConsoleMode(out, wanted)) {
            cursor_write_vt(out, restore, sizeof restore - 1);
            if (wanted != mode)
                (void)SetConsoleMode(out, mode);
        }
    } else {
        out = cursor_vt_output();
        if (out != NULL)
            cursor_write_vt(out, restore, sizeof restore - 1);
    }
    game_cursor_visibility = -1;
    cursor_resume_pending = 1;
}

/* 0 = not yet conclusive, 1 = direct launch, -1 = shared/shell. */
static int resize_permission = 0;

static int
jrogue_is_shell(const wchar_t *name)
{
    return lstrcmpiW(name, L"cmd.exe") == 0 ||
           lstrcmpiW(name, L"powershell.exe") == 0 ||
           lstrcmpiW(name, L"pwsh.exe") == 0 ||
           lstrcmpiW(name, L"bash.exe") == 0 ||
           lstrcmpiW(name, L"sh.exe") == 0 ||
           lstrcmpiW(name, L"mintty.exe") == 0 ||
           lstrcmpiW(name, L"wsl.exe") == 0 ||
           lstrcmpiW(name, L"zsh.exe") == 0 ||
           lstrcmpiW(name, L"fish.exe") == 0 ||
           lstrcmpiW(name, L"nu.exe") == 0;
}

/* 1: Explorer/Terminal launcher, -1: interactive shell, 0: unknown. */
static int
jrogue_launch_parent(void)
{
    HANDLE snapshot;
    PROCESSENTRY32W entry;
    DWORD parent = 0;
    int result = 0;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry))
        do {
            if (entry.th32ProcessID == GetCurrentProcessId()) {
                parent = entry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    if (parent && Process32FirstW(snapshot, &entry))
        do {
            const wchar_t *name = entry.szExeFile;
            if (entry.th32ProcessID != parent)
                continue;
            if (jrogue_is_shell(name))
                result = -1;
            else if (lstrcmpiW(name, L"explorer.exe") == 0 ||
                     lstrcmpiW(name, L"WindowsTerminal.exe") == 0 ||
                     lstrcmpiW(name, L"WindowsTerminalPreview.exe") == 0 ||
                     lstrcmpiW(name, L"OpenConsole.exe") == 0 ||
                     lstrcmpiW(name, L"conhost.exe") == 0 ||
                     lstrcmpiW(name, L"wt.exe") == 0)
                result = 1;
            break;
        } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return result;
}

static int
jrogue_only_console_client(void)
{
    DWORD pids[32];
    DWORD count = GetConsoleProcessList(pids, 32);
    return count == 1 && pids[0] == GetCurrentProcessId();
}

/* 1: shell present, 0: no shell, -1: inconclusive. */
static int
jrogue_console_has_shell(void)
{
    DWORD pids[64], count, i;
    HANDLE snapshot;
    PROCESSENTRY32W entry;
    int found = 0;

    count = GetConsoleProcessList(pids, 64);
    if (count == 0 || count > 64)
        return -1;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return -1;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry))
        do {
            if (!jrogue_is_shell(entry.szExeFile))
                continue;
            for (i = 0; i < count; ++i)
                if (pids[i] == entry.th32ProcessID) {
                    found = 1;
                    break;
                }
            if (found)
                break;
        } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return found;
}

static SHORT
jrogue_min_short(SHORT a, SHORT b)
{
    return a < b ? a : b;
}

static SHORT
jrogue_max_short(SHORT a, SHORT b)
{
    return a > b ? a : b;
}

static void
jrogue_resize_console_host(HANDLE output)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD desired, maximum, enlarged;
    SMALL_RECT rectangle;

    if (!GetConsoleScreenBufferInfo(output, &info))
        return;
    maximum = GetLargestConsoleWindowSize(output);
    if (maximum.X < JROGUE_WIN_COLS || maximum.Y < JROGUE_WIN_ROWS)
        return;
    desired.X = JROGUE_WIN_COLS;
    desired.Y = JROGUE_WIN_ROWS;
    rectangle.Left = 0;
    rectangle.Top = 0;
    rectangle.Right = jrogue_min_short(
        (SHORT)(info.srWindow.Right - info.srWindow.Left + 1), desired.X) - 1;
    rectangle.Bottom = jrogue_min_short(
        (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1), desired.Y) - 1;
    if (!SetConsoleWindowInfo(output, TRUE, &rectangle))
        return;
    enlarged.X = jrogue_max_short(info.dwSize.X, desired.X);
    enlarged.Y = jrogue_max_short(info.dwSize.Y, desired.Y);
    if (!SetConsoleScreenBufferSize(output, enlarged))
        return;
    if (!SetConsoleScreenBufferSize(output, desired))
        return;
    rectangle.Right = desired.X - 1;
    rectangle.Bottom = desired.Y - 1;
    (void)SetConsoleWindowInfo(output, TRUE, &rectangle);
}

static void
jrogue_resize_conpty(HANDLE output, DWORD mode)
{
    static const char sequence[] = "\033[8;24;80t";
    DWORD written;
    if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) &&
        !SetConsoleMode(output, mode | ENABLE_PROCESSED_OUTPUT |
                             ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return;
    (void)WriteFile(output, sequence, (DWORD)(sizeof sequence - 1),
                    &written, NULL);
}

static void
jrogue_try_direct_resize(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    HWND window;
    int parent, only_client, shell_present;

    if (resize_permission != 0 || output == NULL ||
        output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode))
        return;
    parent = jrogue_launch_parent();
    if (parent < 0) {
        resize_permission = -1;
        return;
    }
    only_client = jrogue_only_console_client();
    shell_present = jrogue_console_has_shell();
    if (shell_present > 0) {
        resize_permission = -1;
        return;
    }
    if (shell_present < 0 || (parent == 0 && !only_client))
        return; /* Windows Terminal handoff may not yet be complete. */

    resize_permission = 1;
    window = GetConsoleWindow();
    if (window != NULL && IsWindowVisible(window) && only_client)
        jrogue_resize_console_host(output);
    jrogue_resize_conpty(output, mode);
}

void
jrogue_windows_resize_direct_launch(void)
{
    resize_permission = 0;
    jrogue_try_direct_resize();
}

void
jrogue_windows_retry_direct_launch_resize(int restoring)
{
    HANDLE output;
    DWORD mode;
    if (resize_permission == -1)
        return;
    if (resize_permission == 0) {
        jrogue_try_direct_resize();
        if (resize_permission != 1)
            return;
    }
    if (restoring)
        Sleep(350); /* Give Windows Terminal time to attach the tab. */
    output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output != NULL && output != INVALID_HANDLE_VALUE &&
        GetConsoleMode(output, &mode))
        jrogue_resize_conpty(output, mode);
}

/* Windows Terminal may resize its ConPTY after PDCurses' initscr() has
 * already sampled the old rows and columns.  Drawing an 80x24 game into
 * the stale screen can write below the newly shortened viewport and scroll
 * the dungeon out of sight.  Synchronize before the first game drawing.
 *
 * resize_permission is set only for a private, approved direct launch.
 * Never change an existing cmd/PowerShell/MSYS2 terminal's curses geometry.
 */
int
jrogue_windows_sync_direct_launch_screen(void)
{
    if (resize_permission != 1)
        return OK;

    if ((LINES != JROGUE_WIN_ROWS || COLS != JROGUE_WIN_COLS) &&
        resize_term(JROGUE_WIN_ROWS, JROGUE_WIN_COLS) == ERR)
        return ERR;

    /* Do not trust the pre-resize physical-screen cache. */
    clearok(stdscr, TRUE);
    return OK;
}

/* Painting the bottom-right cell of an exact 80x24 VT viewport can
 * scroll Windows Terminal through DECAWM (automatic line wrap).  Keep
 * PDCursesMod responsible for every cell, including the bottom-right
 * curtain corner; only bracket each physical screen update with the
 * terminal's VT auto-wrap setting.  The ending CSI ? 7 h re-enables the
 * normal wrap mode; it does not query the previous terminal setting.
 * Do not change ordinary curses refreshes outside the startup curtain.
 *
 * The Win32 console can be absent (redirected output or mintty); in that
 * case use the normal curses update without sending raw escape codes.
 */
int
jrogue_windows_curtain_update(int staged)
{
    static const char wrap_off[] = "\033[?7l";
    static const char wrap_on[] = "\033[?7h";
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, written;
    int guarded = 0;
    int result;

    if (output != NULL && output != INVALID_HANDLE_VALUE &&
        GetConsoleMode(output, &mode) &&
        (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
    {
        guarded = WriteFile(output, wrap_off, sizeof wrap_off - 1,
                            &written, NULL) &&
                  written == sizeof wrap_off - 1;
    }
    result = staged ? doupdate() : refresh();
    if (guarded)
        (void)WriteFile(output, wrap_on, sizeof wrap_on - 1,
                        &written, NULL);
    return result;
}

int
jrogue_windows_is_direct_launch(void)
{
    int parent = jrogue_launch_parent();
    if (parent < 0 || resize_permission < 0)
        return 0;
    if (jrogue_console_has_shell() != 0)
        return 0; /* Treat unknown status conservatively. */
    return resize_permission == 1 || parent == 1 ||
           jrogue_only_console_client();
}

/* Called only after endwin() when ordinary stdout owns the screen. */
static int jrogue_score_vt = 0;

void
jrogue_windows_score_prepare(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    jrogue_score_vt = 0;
    if (out != INVALID_HANDLE_VALUE && out != NULL &&
        GetConsoleMode(out, &mode))
    {
        if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ||
            SetConsoleMode(out, mode | ENABLE_PROCESSED_OUTPUT |
                                ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            jrogue_score_vt = 1;
    }
}

int
jrogue_windows_score_uses_vt(void)
{
    return jrogue_score_vt;
}

void
jrogue_windows_score_clear_plain_screen(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    DWORD written;
    COORD first;
    DWORD count;

    if (jrogue_score_vt)
    {
        fputs("\033[0m\033[H\033[2J", stdout);
        fflush(stdout);
        return;
    }
    /* Legacy ConsoleHost: clear only the visible window.  For redirected
     * stdout do not emit escape codes into a log file. */
    if (out == INVALID_HANDLE_VALUE || out == NULL ||
        !GetConsoleScreenBufferInfo(out, &info))
        return;
    first.X = info.srWindow.Left;
    first.Y = info.srWindow.Top;
    count = (DWORD)(info.srWindow.Right - info.srWindow.Left + 1);
    (void)fflush(stdout);
    {
        SHORT row;
        for (row = info.srWindow.Top; row <= info.srWindow.Bottom; ++row)
        {
            COORD at = { first.X, row };
            (void)FillConsoleOutputCharacterW(out, L' ', count, at, &written);
            (void)FillConsoleOutputAttribute(out, info.wAttributes,
                                             count, at, &written);
        }
    }
    (void)SetConsoleCursorPosition(out, first);
}

void
jrogue_windows_sleep_ms(int milliseconds)
{
    Sleep(milliseconds > 0 ? (DWORD)milliseconds : 0);
}

void
jrogue_windows_prepare_console(void)
{
    const char *vt = getenv("PDC_VT");
    /* Preserve any PDC_VT options, adding RGB when it is not requested.
     * This is required to preserve the DOS-era floor background color. */
    if (vt == NULL || *vt == '\0')
        (void)_putenv("PDC_VT=RGB");
    else if (strstr(vt, "RGB") == NULL)
    {
        size_t size = strlen(vt) + sizeof "PDC_VT= RGB";
        char *buf = (char *)malloc(size);
        if (buf != NULL)
        {
            (void)snprintf(buf, size, "PDC_VT=%s RGB", vt);
            (void)_putenv(buf);
            free(buf);
        }
    }
    (void)SetConsoleOutputCP(CP_UTF8);
    (void)SetConsoleCP(CP_UTF8);
}
#endif

#ifdef _WIN32


/* Windows Terminal/ConPTY can deliver cursor keys to ReadConsoleInputW()
 * as VT bytes (ESC [ D/C, etc.) rather than VK_LEFT/VK_RIGHT events.
 * When editing options, an unrecognized ESC would otherwise cancel the
 * entire option screen.  Peek ahead for a complete VT key sequence before
 * treating ESC as the user's explicit cancel key.
 *
 * Leave unrelated key-down events in the console queue: a genuine ESC
 * followed by ordinary typing must not eat the next character.  We do
 * consume key-up/non-key events during this short peek, as the option
 * reader ignores those events anyway.
 */
#define JROGUE_OPTION_ESC_TIMEOUT_MS 100

static int
jrogue_windows_peek_option_char(HANDLE input, unsigned int *value)
{
    DWORD start = GetTickCount();
    INPUT_RECORD record;
    DWORD count;

    for (;;)
    {
        DWORD elapsed = GetTickCount() - start;
        if (elapsed >= JROGUE_OPTION_ESC_TIMEOUT_MS ||
            WaitForSingleObject(input, JROGUE_OPTION_ESC_TIMEOUT_MS - elapsed)
                != WAIT_OBJECT_0 ||
            !PeekConsoleInputW(input, &record, 1, &count) || count != 1)
            return 0;
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown)
        {
            if (!ReadConsoleInputW(input, &record, 1, &count) || count != 1)
                return 0;
            continue;
        }
        /* A non-text special key belongs to the next input request. */
        if (record.Event.KeyEvent.uChar.UnicodeChar == 0)
            return 0;
        *value = (unsigned int)record.Event.KeyEvent.uChar.UnicodeChar;
        return 1;
    }
}

static int
jrogue_windows_take_option_char(HANDLE input, unsigned int *value)
{
    INPUT_RECORD record;
    DWORD count;

    if (!jrogue_windows_peek_option_char(input, value))
        return 0;
    return ReadConsoleInputW(input, &record, 1, &count) && count == 1;
}

static int
jrogue_windows_option_escape_key(HANDLE input)
{
    unsigned int c;
    int i, first = -1, modifiers = 0;

    if (!jrogue_windows_peek_option_char(input, &c))
        return 27; /* Genuine Escape with no following sequence. */
    if (c >= '0' && c <= '9')
    {
        /* Preserve the existing Meta-prefixed keypad digit behavior. */
        return jrogue_windows_take_option_char(input, &c) ? (int)c : ERR;
    }
    if (c != '[' && c != 'O')
        return 27; /* Leave the subsequent ordinary key unread. */
    if (!jrogue_windows_take_option_char(input, &c))
        return ERR;
    if (c == 'O')
    {
        if (!jrogue_windows_take_option_char(input, &c))
            return ERR;
        if (c == 'D') return KEY_LEFT;
        if (c == 'C') return KEY_RIGHT;
        if (c >= 'p' && c <= 'y') return '0' + (int)(c - 'p');
        return ERR;
    }
    for (i = 0; i < 16; i++)
    {
        if (!jrogue_windows_take_option_char(input, &c))
            return ERR;
        if (c >= 0x40 && c <= 0x7e) /* End of CSI sequence. */
        {
            if (c == 'D') return KEY_LEFT;
            if (c == 'C') return KEY_RIGHT;
            if (c == '~' && first == 3) return KEY_DC;
            return ERR; /* Up/down/unknown keys do not cancel options. */
        }
        if (c == ';')
            modifiers = 1;
        else if (!modifiers && c >= '0' && c <= '9')
        {
            if (first < 0)
                first = 0;
            if (first < 100)
                first = first * 10 + (int)(c - '0');
        }
    }
    return ERR;
}

/* Windows option editor: obtain Unicode scalars from KEY_EVENT_RECORD.
 * Unlike narrow wgetch(), this preserves Japanese IME input and non-BMP
 * characters.  Read from the console only when the stdin handle actually
 * supports Win32 console events; mintty/pipe input falls back to curses.
 *
 * Some ConPTY/IME combinations supply UTF-16 surrogate halves on KeyUp
 * instead of KeyDown.  Prefer KeyDown, and use KeyUp halves only when they
 * did not already occur on KeyDown (as in Rogue 5.4.5 JEC).
 *
 * Only the option/string editor calls this function.  Game commands
 * continue through md_readchar() so DOS-specific bindings are retained.
 */
int
jrogue_windows_read_option_key(int *result)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;
    INPUT_RECORD record;
    static WCHAR down_high, down_low;
    unsigned int high = 0;

    if (result == NULL || input == NULL || input == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(input, &mode))
        return 0;

    for (;;)
    {
        KEY_EVENT_RECORD *key;
        unsigned int cp;
        int keydown;

        if (!ReadConsoleInputW(input, &record, 1, &count) || count != 1)
        {
            *result = ERR;
            return 1;
        }
        if (record.EventType != KEY_EVENT)
            continue;
        key = &record.Event.KeyEvent;
        keydown = (key->bKeyDown != FALSE);
        cp = (unsigned int)key->uChar.UnicodeChar;

        /* Special keys are read by their virtual-key codes. */
        if (keydown)
        {
            switch (key->wVirtualKeyCode)
            {
                case VK_LEFT:   *result = KEY_LEFT; return 1;
                case VK_RIGHT:  *result = KEY_RIGHT; return 1;
                case VK_DELETE: *result = KEY_DC; return 1;
                case VK_BACK:   *result = KEY_BACKSPACE; return 1;
                case VK_RETURN: *result = '\n'; return 1;
                case VK_ESCAPE:
                    *result = jrogue_windows_option_escape_key(input);
                    return 1;
            }
        }
        if (cp >= 0xd800 && cp <= 0xdbff)
        {
            if (keydown)
                down_high = (WCHAR)cp;
            else if (down_high == (WCHAR)cp)
            {
                down_high = 0;
                continue;
            }
            high = cp;
            continue;
        }
        if (cp >= 0xdc00 && cp <= 0xdfff)
        {
            if (keydown)
                down_low = (WCHAR)cp;
            else if (down_low == (WCHAR)cp)
            {
                down_low = 0;
                continue;
            }
            if (!high)
                continue;
            *result = (int)(0x10000U + ((high - 0xd800U) << 10)
                         + (cp - 0xdc00U));
            return 1;
        }
        if (!keydown || cp == 0)
            continue;
        high = 0;
        /* A VT ESC byte can have VK=0 instead of VK_ESCAPE. */
        *result = cp == 27 ? jrogue_windows_option_escape_key(input) :
                  cp == '\r' ? '\n' : (int)cp;
        return 1;
    }
}

/* Encode the scalar returned by ReadConsoleInputW or wide PDCurses into
 * the UTF-8 byte buffer used by the portable option/string editor. */
int
jrogue_windows_encode_unicode(unsigned int cp, char out[5])
{
    if (out == NULL || cp > 0x10ffffU ||
        (cp >= 0xd800U && cp <= 0xdfffU))
        return 0;
    if (cp < 0x80U)
    {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800U)
    {
        out[0] = (char)(0xc0U | (cp >> 6));
        out[1] = (char)(0x80U | (cp & 0x3fU));
        return 2;
    }
    if (cp < 0x10000U)
    {
        out[0] = (char)(0xe0U | (cp >> 12));
        out[1] = (char)(0x80U | ((cp >> 6) & 0x3fU));
        out[2] = (char)(0x80U | (cp & 0x3fU));
        return 3;
    }
    out[0] = (char)(0xf0U | (cp >> 18));
    out[1] = (char)(0x80U | ((cp >> 12) & 0x3fU));
    out[2] = (char)(0x80U | ((cp >> 6) & 0x3fU));
    out[3] = (char)(0x80U | (cp & 0x3fU));
    return 4;
}

/* PDCursesMod's narrow string writer can split 4-byte UTF-8 sequences.
 * Supply each non-BMP scalar as one chtype, preserving its character.
 * Ordinary text remains on the existing UTF-8 waddstr() path. */
int
jrogue_windows_waddstr_utf8(WINDOW *win, const char *text)
{
    const unsigned char *start = (const unsigned char *)text;
    const unsigned char *p;
    size_t total, used = 0;
    char *segment;
    int result = OK;

    if (win == NULL || text == NULL)
        return ERR;
    total = strlen(text);
    segment = malloc(total + 1);
    if (segment == NULL)
        return waddstr(win, text);
    p = start;
    while (*p)
    {
        unsigned int cp = 0;
        if (p[0] >= 0xf0 && p[0] <= 0xf4 &&
            (size_t)(p - start) + 4 <= total &&
            (p[1] & 0xc0) == 0x80 && (p[2] & 0xc0) == 0x80 &&
            (p[3] & 0xc0) == 0x80)
            cp = ((unsigned int)(p[0] & 7) << 18) |
                 ((unsigned int)(p[1] & 0x3f) << 12) |
                 ((unsigned int)(p[2] & 0x3f) << 6) |
                 (unsigned int)(p[3] & 0x3f);
        if (cp >= 0x10000U && cp <= 0x10ffffU)
        {
            if (used)
            {
                segment[used] = '\0';
                if (waddstr(win, segment) == ERR)
                    result = ERR;
                used = 0;
            }
            if (waddch(win, (chtype)cp) == ERR)
                result = ERR;
            p += 4;
        }
        else
            segment[used++] = (char)*p++;
    }
    if (used)
    {
        segment[used] = '\0';
        if (waddstr(win, segment) == ERR)
            result = ERR;
    }
    free(segment);
    return result;
}


/* PDCursesMod VT can hold the full scalar in chtype, yet output it through
 * a 16-bit conversion.  Repair only affected option/editor cells with a
 * direct UTF-8 VT repaint, as Rogue 5.4.5 JEC does.  Never write escapes
 * to a redirected stream or a console that lacks VT processing. */
void
jrogue_windows_option_vt_restore_nonbmp(WINDOW *win)
{
    HANDLE out = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode, written;
    int rows, cols, beg_y, beg_x, cur_y, cur_x, y, x;
    char output[80];

    if (win == NULL || out == NULL || out == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(out, &mode) ||
        !(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return;
    getmaxyx(win, rows, cols);
    getbegyx(win, beg_y, beg_x);
    getyx(win, cur_y, cur_x);
    for (y = 0; y < rows; ++y)
    {
        for (x = 0; x < cols; ++x)
        {
            unsigned int cp = (unsigned int)(mvwinch(win, y, x) & A_CHARTEXT);
            char utf8[5];
            int count, n;
            if (cp < 0x10000U || cp > 0x10ffffU || beg_x + x >= COLS - 1)
                continue;
            count = jrogue_windows_encode_unicode(cp, utf8);
            if (count != 4)
                continue;
            n = snprintf(output, sizeof output, "\033[%d;%dH%c%c%c%c",
                         beg_y + y + 1, beg_x + x + 1,
                         utf8[0], utf8[1], utf8[2], utf8[3]);
            if (n > 0 && (size_t)n < sizeof output)
                (void)WriteFile(out, output, (DWORD)n, &written, NULL);
        }
    }
    (void)wmove(win, cur_y, cur_x);
    {
        int n = snprintf(output, sizeof output, "\033[%d;%dH",
                         beg_y + cur_y + 1, beg_x + cur_x + 1);
        if (n > 0 && (size_t)n < sizeof output)
            (void)WriteFile(out, output, (DWORD)n, &written, NULL);
    }
}

#endif /* _WIN32 */

/*
 * Rogue's fcolor[] and bcolor[] use ANSI/xterm palette indices.  The
 * PDCursesMod VT backend uses a different order for the first 16 colors.
 * Adapt ONLY the Windows pair definitions; the portable game palette and
 * the macOS/Linux rendering are unchanged.
 *
 * Based on the tested Rogue 5.4.5 JEC Windows palette mapping.
 */
static const short jrogue_ansi_to_pdc[16] = {
    COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
    COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_BLACK + 8, COLOR_RED + 8, COLOR_GREEN + 8,
    COLOR_YELLOW + 8, COLOR_BLUE + 8, COLOR_MAGENTA + 8,
    COLOR_CYAN + 8, COLOR_WHITE + 8
};

static const unsigned char jrogue_ansi_rgb[16][3] = {
    {  0,   0,   0}, {128,   0,   0}, {  0, 128,   0}, {128, 128,   0},
    {  0,   0, 128}, {128,   0, 128}, {  0, 128, 128}, {192, 192, 192},
    {128, 128, 128}, {255,   0,   0}, {  0, 255,   0}, {255, 255,   0},
    {  0,   0, 255}, {255,   0, 255}, {  0, 255, 255}, {255, 255, 255}
};

static void
jrogue_xterm_rgb(int index, int *red, int *green, int *blue)
{
    static const unsigned char steps[6] = { 0, 95, 135, 175, 215, 255 };
    int n;

    if (index >= 0 && index < 16) {
        *red = jrogue_ansi_rgb[index][0];
        *green = jrogue_ansi_rgb[index][1];
        *blue = jrogue_ansi_rgb[index][2];
    } else if (index >= 16 && index < 232) {
        n = index - 16;
        *red = steps[n / 36];
        *green = steps[(n / 6) % 6];
        *blue = steps[n % 6];
    } else if (index >= 232 && index < 256) {
        *red = *green = *blue = 8 + (index - 232) * 10;
    } else {
        *red = *green = *blue = 0;
    }
    /* The DOS 1.48 floor uses the same dark blue as 5.4.5 JEC. */
    if (index == 17)
        *blue = 64;
}

static short
jrogue_nearest_basic_color(int index)
{
    int red, green, blue, best = 0, best_distance = 0x7fffffff;
    int i, dr, dg, db, distance;
    int count = COLORS >= 16 ? 16 : COLORS >= 8 ? 8 : COLORS;

    jrogue_xterm_rgb(index, &red, &green, &blue);
    for (i = 0; i < count; i++) {
        dr = red - jrogue_ansi_rgb[i][0];
        dg = green - jrogue_ansi_rgb[i][1];
        db = blue - jrogue_ansi_rgb[i][2];
        distance = dr * dr + dg * dg + db * db;
        if (distance < best_distance) {
            best_distance = distance;
            best = i;
        }
    }
    return jrogue_ansi_to_pdc[best];
}

short
jrogue_windows_palette_color(int color)
{
    if (color == -1)
        return -1;
    if (color < 0)
        return COLOR_BLACK;
    if (color < 16)
        return color < COLORS ? jrogue_ansi_to_pdc[color]
                              : jrogue_nearest_basic_color(color);
    if (color >= COLORS)
        return jrogue_nearest_basic_color(color);
    return (short)color;
}

void
jrogue_windows_init_palette(void)
{
    int index, red, green, blue;
    int count = COLORS < 256 ? COLORS : 256;

    if (!can_change_color())
        return;
    /* PDCursesMod owns the VT stream; never emit raw escape codes here. */
    for (index = 0; index < count; index++) {
        jrogue_xterm_rgb(index, &red, &green, &blue);
        (void)init_color(index < 16 ? jrogue_ansi_to_pdc[index] : (short)index,
                         (short)((red * 1000 + 127) / 255),
                         (short)((green * 1000 + 127) / 255),
                         (short)((blue * 1000 + 127) / 255));
    }
}
