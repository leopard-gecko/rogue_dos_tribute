/* Windows-only file and environment services.
 * UTF-8 is the character encoding used by the portable game code. */
#ifndef ROGUE148_WINDOWS_H
#define ROGUE148_WINDOWS_H

#ifdef _WIN32
#include <stdio.h>
#include <wchar.h>
#include <sys/stat.h>
/* Return a writable UTF-8 copy of ROGUEOPTS, or NULL if unavailable.
 * The caller owns the returned allocation and must free it. */
char *jrogue_rogueopts_utf8(void);
/* Unicode command-line paths for saved games; caller frees the result. */
char *jrogue_windows_argument_utf8(int argument_index);
/* Shared with the console layer: never classify a shell as a direct launch. */
int jrogue_windows_is_direct_launch(void);
/* Explorer-style launch: return a selected UTF-8 save path, or NULL for new. */
char *jrogue_windows_launch_save(const char *default_save_path);
/* Prompt after a manual save or early failure in a private console. */
void jrogue_windows_wait_for_direct_exit(void);

/* UTF-8 paths; the portable game and score format stay unchanged. */
FILE *jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode);
int jrogue_windows_stat_utf8(const char *path, struct stat *st);
int jrogue_windows_unlink_utf8(const char *path);
int jrogue_windows_chmod_utf8(const char *path, int mode);
/* O_EXCL creation is used for the shared scoreboard/lockfile. */
FILE *jrogue_windows_create_exclusive_utf8(const char *path, int permissions,
                                            const wchar_t *mode);

#endif

#endif /* ROGUE148_WINDOWS_H */
