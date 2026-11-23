/* Windows environment services for Rogue DOS Tribute.
 * Keep Win32/UTF-16 conversion outside the portable option parser.
 * The implementation mirrors the tested Rogue 5.4.5 JEC approach. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <conio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include "rogue_windows.h"
#include "jec/language.h"

/* Convert Windows' UTF-16 strings to the portable UTF-8 representation.
 * Used by ROGUEOPTS, command-line save paths and the file picker.
 * The caller frees the result; never use the ANSI code page for these values. */
static char *
wide_to_utf8(const wchar_t *wide)
{
    int bytes;
    char *text;

    if (wide == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                wide, -1, NULL, 0, NULL, NULL);
    if (bytes <= 0)
    {
        errno = EILSEQ;
        return NULL;
    }
    text = (char *)malloc((size_t)bytes);
    if (text == NULL)
        return NULL;
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            wide, -1, text, bytes, NULL, NULL) != bytes)
    {
        free(text);
        errno = EILSEQ;
        return NULL;
    }
    return text;
}

/* Windows' narrow getenv() need not be UTF-8.  The original option parser
 * expects a writable UTF-8 string, which the caller must free. */
char *
jrogue_rogueopts_utf8(void)
{
    DWORD needed = GetEnvironmentVariableW(L"ROGUEOPTS", NULL, 0);
    DWORD got;
    wchar_t *wide;
    char *utf8;

    if (needed == 0 || needed > 32767)
        return NULL;
    wide = (wchar_t *)malloc((size_t)needed * sizeof *wide);
    if (wide == NULL)
        return NULL;
    got = GetEnvironmentVariableW(L"ROGUEOPTS", wide, needed);
    if (got == 0 || got >= needed)
    {
        free(wide);
        return NULL;
    }
    utf8 = wide_to_utf8(wide);
    free(wide);
    return utf8;
}

/* MinGW's argv[] may be in the ANSI code page, even when a Japanese save
 * path was passed to the EXE.  Always read it from the original UTF-16
 * command line.  The caller owns the returned UTF-8 string. */
char *
jrogue_windows_argument_utf8(int argument_index)
{
    LPWSTR *wide_argv;
    int count;
    char *utf8;

    if (argument_index < 0)
    {
        errno = EINVAL;
        return NULL;
    }
    wide_argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (wide_argv == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    if (argument_index >= count)
    {
        LocalFree(wide_argv);
        errno = EINVAL;
        return NULL;
    }
    utf8 = wide_to_utf8(wide_argv[argument_index]);
    LocalFree(wide_argv);
    return utf8;
}

/* All caller paths are UTF-8, not the process ANSI code page. */
static wchar_t *
utf8_to_wide(const char *text)
{
    int chars;
    wchar_t *wide;
    if (text == NULL)
    {
        errno = EINVAL;
        return NULL;
    }
    chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                text, -1, NULL, 0);
    if (chars <= 0)
    {
        errno = EINVAL;
        return NULL;
    }
    wide = (wchar_t *)malloc((size_t)chars * sizeof *wide);
    if (wide == NULL)
        return NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            text, -1, wide, chars) != chars)
    {
        free(wide);
        errno = EINVAL;
        return NULL;
    }
    return wide;
}

/* The startup menu checks the resolved default path, including FILEDIR
 * and any ROGUEOPTS file override.  Do not change the working directory. */
static int
jrogue_windows_save_exists(const char *path)
{
    wchar_t *wide;
    DWORD attributes;
    if (path == NULL || *path == '\0')
        return 0;
    wide = utf8_to_wide(path);
    if (wide == NULL)
        return 0;
    attributes = GetFileAttributesW(wide);
    free(wide);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static char *
jrogue_windows_select_save(void)
{
    OPENFILENAMEW dialog;
    wchar_t *buffer;
    char *path = NULL;
    static const wchar_t filters[] =
        L"Saved games (*.save)\0*.save\0All files (*.*)\0*.*\0";

    buffer = (wchar_t *)calloc(32768, sizeof *buffer);
    if (buffer == NULL)
        return NULL;
    memset(&dialog, 0, sizeof dialog);
    dialog.lStructSize = sizeof dialog;
    dialog.lpstrFilter = filters;
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = 32768;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                   OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
    dialog.lpstrTitle = jrogue_lang_is_japanese ?
        L"セーブファイルを選択" : L"Select a saved game";
    if (GetOpenFileNameW(&dialog))
        path = wide_to_utf8(buffer);
    free(buffer);
    return path;
}

/* Return an allocated UTF-8 path, or NULL for a new adventure.
 * The menu is only shown for a direct launch when the normal save exists.
 * ROGUEOPTS file=... and an explicit command-line argument take priority. */
char *
jrogue_windows_launch_save(const char *default_path)
{
    int c;
    char *selected;
    if (!jrogue_windows_is_direct_launch() ||
        !jrogue_windows_save_exists(default_path))
        return NULL;
    for (;;) {
        printf("\n                 Rogue DOS Tribute JEC.092\n\n");
        printf("%s\n\n", JSTR("セーブした冒険が見つかりました。",
                                "A saved adventure was found."));
        printf("[R/Enter] %s\n", JSTR("セーブした冒険を再開する",
                                        "Resume saved adventure"));
        printf("[N]       %s\n", JSTR("新しい冒険を始める",
                                        "Start a new adventure"));
        printf("[F]       %s\n", JSTR("別のセーブファイルを選ぶ",
                                        "Choose another save file"));
        printf("[Q]       %s\n\n", JSTR("終了する", "Quit"));
        printf("%s", JSTR("キーを押してください: ", "Press a key: "));
        fflush(stdout);
        c = _getwch();
        if (c == 0 || c == 0xe0) {
            (void)_getwch();
            continue;
        }
        putchar('\n');
        if (c == 'r' || c == 'R' || c == '\r' || c == '\n')
            return _strdup(default_path);
        if (c == 'n' || c == 'N')
            return NULL;
        if (c == 'q' || c == 'Q' || c == 27)
            exit(0);
        if (c == 'f' || c == 'F') {
            selected = jrogue_windows_select_save();
            if (selected != NULL)
                return selected;
            /* Cancelled file dialog: return to the start menu. */
        }
    }
}

/* Show the result of a direct Explorer launch before its terminal closes.
 * Command-line launches never receive a second prompt. */
void
jrogue_windows_wait_for_direct_exit(void)
{
    wint_t key;
    if (!jrogue_windows_is_direct_launch())
        return;
    fputs(JSTR("［リターンを押すと終了します］",
               "[Press Enter to exit]"), stdout);
    fflush(stdout);
    for (;;) {
        key = _getwch();
        if (key == '\r' || key == '\n' || key == WEOF)
            break;
        if (key == 0 || key == 0xe0)
            (void)_getwch();
    }
    putchar('\n');
}

FILE *
jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode)
{
    wchar_t *wide = utf8_to_wide(path);
    FILE *fp = NULL;
    if (wide != NULL)
    {
        fp = _wfopen(wide, mode);
        free(wide);
    }
    return fp;
}

/* The portable restore check needs st_mode, st_nlink and st_mtime. */
int
jrogue_windows_stat_utf8(const char *path, struct stat *st)
{
    wchar_t *wide = utf8_to_wide(path);
    struct _stat64 found;
    int rc = -1;
    if (wide == NULL)
        return -1;
    if (st == NULL)
    {
        free(wide);
        errno = EINVAL;
        return -1;
    }
    if (_wstat64(wide, &found) == 0)
    {
        memset(st, 0, sizeof *st);
        st->st_mode = found.st_mode;
        st->st_nlink = found.st_nlink;
        st->st_mtime = (time_t)found.st_mtime;
        rc = 0;
    }
    free(wide);
    return rc;
}

int
jrogue_windows_unlink_utf8(const char *path)
{
    wchar_t *wide = utf8_to_wide(path);
    int rc = -1;
    if (wide != NULL)
    {
        rc = _wunlink(wide);
        free(wide);
    }
    return rc;
}

int
jrogue_windows_chmod_utf8(const char *path, int mode)
{
    wchar_t *wide = utf8_to_wide(path);
    int rc = -1;
    if (wide != NULL)
    {
        rc = _wchmod(wide, mode);
        free(wide);
    }
    return rc;
}

/* Exclusive creation preserves the existing cross-OS lockfile protocol.
 * Also use it for the first creation of an empty scoreboard to avoid
 * truncating a scoreboard another process has just created. */
FILE *
jrogue_windows_create_exclusive_utf8(const char *path, int permissions,
                                             const wchar_t *mode)
{
    wchar_t *wide = utf8_to_wide(path);
    int fd, saved_errno;
    FILE *fp;
    if (wide == NULL)
        return NULL;
    fd = _wopen(wide, _O_BINARY | _O_CREAT | _O_EXCL | _O_RDWR,
                permissions);
    if (fd < 0)
    {
        free(wide);
        return NULL;
    }
    fp = _wfdopen(fd, mode);
    if (fp == NULL)
    {
        saved_errno = errno;
        _close(fd);
        (void)_wunlink(wide);
        errno = saved_errno;
    }
    free(wide);
    return fp;
}
#endif
