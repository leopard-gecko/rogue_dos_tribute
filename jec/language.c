#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "language.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

int jrogue_lang_is_japanese = DEFAULT_JAPANESE;

/* Windows UCRT defaults to the user's ANSI code page when the empty
 * locale string is used.  Rogue's Japanese messages and DOS PC glyphs
 * are UTF-8, so select an explicitly UTF-8 C locale before initscr().
 * Keep macOS/Linux's original environment-based locale selection.
 * The UI language itself is selected separately by ROGUEOPTS/OS settings. */
void
jrogue_init_locale(void)
{
#ifdef _WIN32
    if (setlocale(LC_ALL, ".UTF8") == NULL)
        (void)setlocale(LC_ALL, "");
#else
    (void)setlocale(LC_ALL, "");
#endif
}

static int
lang_starts_ja(const char *lang)
{
    return lang != NULL && (lang[0] == 'j' || lang[0] == 'J')
	&& (lang[1] == 'a' || lang[1] == 'A');
}

static int
lang_is_ja(const char *lang)
{
    return lang_starts_ja(lang) && lang[2] == '\0';
}

static int
lang_is_en(const char *lang)
{
    return lang != NULL && (lang[0] == 'e' || lang[0] == 'E')
	&& (lang[1] == 'n' || lang[1] == 'N') && lang[2] == '\0';
}

static int
lang_is_auto(const char *lang)
{
    return lang != NULL && (lang[0] == 'a' || lang[0] == 'A')
	&& (lang[1] == 'u' || lang[1] == 'U')
	&& (lang[2] == 't' || lang[2] == 'T')
	&& (lang[3] == 'o' || lang[3] == 'O')
	&& lang[4] == '\0';
}

static int
jrogue_locale_is_japanese(void)
{
#ifdef _WIN32
    LANGID langid;

    langid = GetUserDefaultUILanguage();
    return PRIMARYLANGID(langid) == LANG_JAPANESE;
#else
    const char *lang;

    lang = getenv("LC_ALL");
    if (lang == NULL || *lang == '\0')
	lang = getenv("LC_MESSAGES");
    if (lang == NULL || *lang == '\0')
	lang = getenv("LANG");

    return lang_starts_ja(lang);
#endif
}

static int
jrogue_rogueopts_language_mode(const char *opts, int fallback)
{
    const char *p, *end, *eq, *value;
    size_t keylen, vlen;
    int mode;

    mode = fallback;
    if (opts == NULL)
	return mode;

    p = opts;
    while (*p != '\0')
    {
	while (*p == ',' || isspace((unsigned char)*p))
	    p++;
	if (*p == '\0')
	    break;

	end = strchr(p, ',');
	if (end == NULL)
	    end = p + strlen(p);

	eq = memchr(p, '=', (size_t)(end - p));
	if (eq != NULL)
	{
	    keylen = (size_t)(eq - p);
	    while (keylen > 0 && isspace((unsigned char)p[keylen - 1]))
		keylen--;

	    if (keylen == 8 && strncmp(p, "language", 8) == 0)
	    {
		value = eq + 1;
		while (value < end && (*value == '=' || isspace((unsigned char)*value)))
		    value++;
		vlen = (size_t)(end - value);
		while (vlen > 0 && isspace((unsigned char)value[vlen - 1]))
		    vlen--;

		if (vlen == 2 && (value[0] == 'j' || value[0] == 'J')
		    && (value[1] == 'a' || value[1] == 'A'))
		    mode = 1;
		else if (vlen == 2 && (value[0] == 'e' || value[0] == 'E')
		    && (value[1] == 'n' || value[1] == 'N'))
		    mode = 2;
		else if (vlen == 4 && (value[0] == 'a' || value[0] == 'A')
		    && (value[1] == 'u' || value[1] == 'U')
		    && (value[2] == 't' || value[2] == 'T')
		    && (value[3] == 'o' || value[3] == 'O'))
		    mode = 0;
	    }
	}

	p = end;
    }

    return mode;
}

void
jrogue_detect_language(const char *opts)
{
    int mode;

    /*
     * Runtime language selection is intentionally centralized in ROGUEOPTS.
     * language=ja/en/auto overrides the build-time default.  "auto" is an
     * internal startup mode only; the in-game option remains a J/E choice.
     */
    mode = jrogue_rogueopts_language_mode(opts, DEFAULT_LANGUAGE);

    if (mode == 1)
	jrogue_lang_is_japanese = TRUE;
    else if (mode == 2)
	jrogue_lang_is_japanese = FALSE;
    else
	jrogue_lang_is_japanese = jrogue_locale_is_japanese();
}

const char *
jrogue_str(const char *ja, const char *en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

int
jrogue_chr(int ja, int en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

const char *
jrogue_language_code(void)
{
    return jrogue_lang_is_japanese ? "ja" : "en";
}

int
jrogue_set_language(const char *lang)
{
    if (lang_is_ja(lang))
    {
	jrogue_lang_is_japanese = TRUE;
	return TRUE;
    }
    if (lang_is_en(lang))
    {
	jrogue_lang_is_japanese = FALSE;
	return TRUE;
    }
    if (lang_is_auto(lang))
    {
	jrogue_lang_is_japanese = jrogue_locale_is_japanese();
	return TRUE;
    }
    return FALSE;
}
