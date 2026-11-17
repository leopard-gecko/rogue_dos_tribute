#ifndef JROGUE_LANGUAGE_H
#define JROGUE_LANGUAGE_H

/*
 * Runtime language selection.
 *
 * ROGUEOPTS language=ja/en/auto overrides the build-time default.
 * DEFAULT_LANGUAGE controls the build-time default:
 *   0 = automatic from the OS locale
 *   1 = Japanese
 *   2 = English
 *
 * In auto mode, Unix-like systems check LC_ALL, LC_MESSAGES, then LANG;
 * native Windows uses the user's UI language.  Unknown locales fall back to
 * English.  The interactive option screen remains a Japanese/English choice;
 * auto is used only to choose the initial language.
 *
 * The plain "make" build uses automatic detection.  The "make japanese"
 * and "make english" targets set DEFAULT_LANGUAGE to 1 or 2, respectively.
 */
#ifndef DEFAULT_LANGUAGE
#define DEFAULT_LANGUAGE 0
#endif

/*
 * DEFAULT_JAPANESE is derived internally from DEFAULT_LANGUAGE.
 * External build rules should define DEFAULT_LANGUAGE only; deriving this
 * helper here avoids inconsistent language defaults in static initializers.
 */
#ifdef DEFAULT_JAPANESE
#undef DEFAULT_JAPANESE
#endif
#if DEFAULT_LANGUAGE == 1
#define DEFAULT_JAPANESE 1
#else
#define DEFAULT_JAPANESE 0
#endif

extern int jrogue_lang_is_japanese;

/* Initialize the C locale before initializing curses (UTF-8 on Windows). */
void jrogue_init_locale(void);

/* opts must be UTF-8; on Windows main() obtains it from the Win32 API. */
void jrogue_detect_language(const char *opts);
void jrogue_apply_language(void);
void jrogue_apply_options_language(void);
void jrogue_apply_rip_language(void);
void jrogue_apply_potions_language(void);
void jrogue_apply_fight_language(void);
void jrogue_apply_init_language(void);
const char *jrogue_str(const char *ja, const char *en);
int jrogue_chr(int ja, int en);
const char *jrogue_language_code(void);
int jrogue_set_language(const char *lang);

/* Use these in normal code. */
#define JSTR(ja, en) jrogue_str((ja), (en))
#define JCHR(ja, en) jrogue_chr((ja), (en))

/*
 * Use these only in static initializers, where C requires compile-time
 * constant expressions.
 */
#if DEFAULT_JAPANESE
# define JSTR_STATIC(ja, en) ja
# define JCHR_STATIC(ja, en) ja
#else
# define JSTR_STATIC(ja, en) en
# define JCHR_STATIC(ja, en) en
#endif

#endif /* JROGUE_LANGUAGE_H */
