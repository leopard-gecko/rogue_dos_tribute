/* Common UTF-8 editing boundaries and terminal display-width estimates.
 * Based on the JEC implementation shared with Rogue 5.4.5.
 * Copyright (c) 2026 leopardgecko. See LICENSE.TXT.
 *
 * A compact subset of Unicode UAX #29, including common combining marks,
 * variation selectors, emoji modifiers, RI pairs, ZWJ emoji, and Hangul.
 * Not a complete grapheme segmenter (e.g. Indic conjunct clusters).
 * Terminal/font rendering of complex emoji is not guaranteed.
 * No dependence on curses, OS APIs, or process locale.
 */
#include "unicode_text.h"
#include <stdint.h>
#include <string.h>

struct u_range { uint32_t lo, hi; };

static int
in_ranges(uint32_t c, const struct u_range *r, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++)
        if (c >= r[i].lo && c <= r[i].hi)
            return 1;
    return 0;
}
#define IN(c, r) in_ranges((c), (r), sizeof(r) / sizeof((r)[0]))

static const struct u_range extend_ranges[] = {
    {0x0300,0x036f}, {0x0483,0x0489}, {0x0591,0x05bd},
    {0x05bf,0x05bf}, {0x05c1,0x05c2}, {0x05c4,0x05c5},
    {0x0610,0x061a}, {0x064b,0x065f}, {0x0670,0x0670},
    {0x06d6,0x06ed}, {0x0711,0x0711}, {0x0730,0x074a},
    {0x07a6,0x07b0}, {0x0900,0x0902}, {0x093a,0x093c},
    {0x0941,0x0948}, {0x094d,0x094d}, {0x0951,0x0957},
    {0x0962,0x0963}, {0x0981,0x0981}, {0x09bc,0x09bc},
    {0x09c1,0x09c4}, {0x09cd,0x09cd}, {0x0a01,0x0a02},
    {0x0a3c,0x0a3c}, {0x0a41,0x0a42}, {0x0a47,0x0a48},
    {0x0a4b,0x0a4d}, {0x0abc,0x0abc}, {0x0ac1,0x0ac8},
    {0x0acd,0x0acd}, {0x0b3c,0x0b3c}, {0x0b3f,0x0b3f},
    {0x0b41,0x0b44}, {0x0b4d,0x0b4d}, {0x0c3e,0x0c40},
    {0x0c46,0x0c48}, {0x0c4a,0x0c4d}, {0x0d41,0x0d44},
    {0x0d4d,0x0d4d}, {0x0e31,0x0e31}, {0x0e34,0x0e3a},
    {0x0e47,0x0e4e}, {0x0eb1,0x0eb1}, {0x0eb4,0x0ebc},
    {0x0ec8,0x0ecd}, {0x0f71,0x0f84}, {0x0f86,0x0f87},
    {0x102d,0x1030}, {0x1032,0x1037}, {0x1039,0x103a},
    {0x135d,0x135f}, {0x1712,0x1714}, {0x1732,0x1734},
    {0x1752,0x1753}, {0x1772,0x1773}, {0x17b4,0x17b5},
    {0x17b7,0x17bd}, {0x17c6,0x17c6}, {0x17c9,0x17d3},
    {0x180b,0x180f}, {0x1a17,0x1a18}, {0x1ab0,0x1aff},
    {0x1dc0,0x1dff}, {0x200c,0x200c}, {0x20d0,0x20ff}, {0x302a,0x302f},
    {0x3099,0x309a}, {0xfe00,0xfe0f}, {0xfe20,0xfe2f},
    {0x1f3fb,0x1f3ff}, {0xe0020,0xe007f}, {0xe0100,0xe01ef}
};

static int is_extend(uint32_t c) { return IN(c, extend_ranges); }
static int is_ri(uint32_t c) { return c >= 0x1f1e6 && c <= 0x1f1ff; }

static int
is_pictographic(uint32_t c)
{
    return (c >= 0x1f000 && c <= 0x1faff) ||
           (c >= 0x2300 && c <= 0x23ff) ||
           (c >= 0x2600 && c <= 0x27bf) ||
           c == 0x00a9 || c == 0x00ae || c == 0x203c ||
           c == 0x2049 || c == 0x2122 || c == 0x2139 ||
           c == 0x3030 || c == 0x303d || c == 0x3297 || c == 0x3299;
}

static int is_control(uint32_t c) { return c < 0x20 || (c >= 0x7f && c <= 0x9f); }

static int
hangul_class(uint32_t c)
{
    if ((c >= 0x1100 && c <= 0x115f) ||
        (c >= 0xa960 && c <= 0xa97c)) return 1;
    if ((c >= 0x1160 && c <= 0x11a7) ||
        (c >= 0xd7b0 && c <= 0xd7c6)) return 2;
    if ((c >= 0x11a8 && c <= 0x11ff) ||
        (c >= 0xd7cb && c <= 0xd7fb)) return 3;
    if (c >= 0xac00 && c <= 0xd7a3)
        return ((c - 0xac00) % 28 == 0) ? 4 : 5;
    return 0;
}

static int
hangul_join(int a, int b)
{
    return (a == 1 && (b == 1 || b == 2 || b == 4 || b == 5)) ||
           ((a == 2 || a == 4) && (b == 2 || b == 3)) ||
           ((a == 3 || a == 5) && b == 3);
}

/* Strict Unicode scalar decoding; invalid bytes are separate units. */
static size_t
read_scalar(const unsigned char *s, size_t at, size_t limit, uint32_t *cp)
{
    uint32_t v;
    unsigned int count, i;
    unsigned char b = s[at];
    if (b < 0x80) { *cp = b; return at + 1; }
    if (b >= 0xc2 && b <= 0xdf) { v = b & 0x1f; count = 2; }
    else if (b >= 0xe0 && b <= 0xef) { v = b & 0x0f; count = 3; }
    else if (b >= 0xf0 && b <= 0xf4) { v = b & 7; count = 4; }
    else { *cp = 0xfffd; return at + 1; }
    if (limit - at < count) { *cp = 0xfffd; return at + 1; }
    for (i = 1; i < count; i++)
    {
        b = s[at + i];
        if ((b & 0xc0) != 0x80) { *cp = 0xfffd; return at + 1; }
        v = (v << 6) | (b & 0x3f);
    }
    if ((count == 2 && v < 0x80) ||
        (count == 3 && v < 0x800) ||
        (count == 4 && v < 0x10000) ||
        (v >= 0xd800 && v <= 0xdfff) || v > 0x10ffff)
    { *cp = 0xfffd; return at + 1; }
    *cp = v;
    return at + count;
}

int
jrogue_utf8_single_scalar(const char *glyph, unsigned int *scalar)
{
    size_t length, consumed;
    uint32_t cp;

    if (glyph == NULL || scalar == NULL || *glyph == '\0')
        return 0;
    length = strlen(glyph);
    consumed = read_scalar((const unsigned char *)glyph, 0, length, &cp);
    /* On malformed leading bytes read_scalar() returns U+FFFD after one
     * byte; a correctly encoded U+FFFD always takes three bytes. */
    if (consumed != length || (length == 1 && cp == 0xfffd))
        return 0;
    *scalar = (unsigned int)cp;
    return 1;
}

static size_t
cluster_end(const unsigned char *s, size_t at, size_t limit)
{
    uint32_t first, cp, previous;
    size_t next = read_scalar(s, at, limit, &first);
    int last_ep = is_pictographic(first);
    int zwj_after_ep = 0;
    int ri_count = is_ri(first) ? 1 : 0;
    int hc = hangul_class(first);

    previous = first;
    while (next < limit)
    {
        size_t end = read_scalar(s, next, limit, &cp);
        int hc_next = hangul_class(cp);
        if (previous == '\r' && cp == '\n') { }
        else if (is_control(previous) || is_control(cp)) break;
        else if (hangul_join(hc, hc_next)) { }
        else if (is_extend(cp)) { }
        else if (cp == 0x200d) { }
        else if (zwj_after_ep && is_pictographic(cp)) { }
        else if (ri_count == 1 && is_ri(cp)) { }
        else break;

        if (cp == 0x200d)
            zwj_after_ep = last_ep;
        else if (!is_extend(cp))
        {
            zwj_after_ep = 0;
            last_ep = is_pictographic(cp);
        }
        if (is_ri(cp)) ri_count++;
        else ri_count = 0;
        previous = cp;
        hc = hc_next;
        next = end;
    }
    return next;
}

size_t
jrogue_text_next(const char *text, size_t pos)
{
    size_t start = 0, len;
    if (!text) return 0;
    len = strlen(text);
    if (pos >= len) return len;
    while (start < len)
    {
        size_t end = cluster_end((const unsigned char *)text, start, len);
        if (end > pos) return end;
        start = end;
    }
    return len;
}

size_t
jrogue_text_prev(const char *text, size_t pos)
{
    size_t start = 0, len;
    if (!text || pos == 0) return 0;
    len = strlen(text);
    if (pos > len) pos = len;
    while (start < len)
    {
        size_t end = cluster_end((const unsigned char *)text, start, len);
        if (end >= pos) return start;
        start = end;
    }
    return start;
}

size_t
jrogue_text_prefix(const char *text, size_t max_bytes)
{
    size_t start = 0, len;
    if (!text) return 0;
    len = strlen(text);
    if (max_bytes >= len) return len;
    while (start < len)
    {
        size_t end = cluster_end((const unsigned char *)text, start, len);
        if (end > max_bytes) break;
        start = end;
    }
    return start;
}

/* Input handlers use this to collect one UTF-8 byte sequence. */
int
jrogue_utf8_sequence_bytes(unsigned char c)
{
    if (c < 0x80) return 1;
    if (c >= 0xc2 && c <= 0xdf) return 2;
    if (c >= 0xe0 && c <= 0xef) return 3;
    if (c >= 0xf0 && c <= 0xf4) return 4;
    return 1;
}

static int
scalar_cells(uint32_t cp)
{
    if (cp == 0x200d || is_extend(cp)) return 0;
    if (cp < 0x1100) return 1;
    if ((cp >= 0x1100 && cp <= 0x115f) ||
        (cp >= 0x2329 && cp <= 0x232a) ||
        (cp >= 0x2e80 && cp <= 0xa4cf) ||
        (cp >= 0xac00 && cp <= 0xd7a3) ||
        (cp >= 0xf900 && cp <= 0xfaff) ||
        (cp >= 0xfe10 && cp <= 0xfe6f) ||
        (cp >= 0xff01 && cp <= 0xff60) ||
        (cp >= 0xffe0 && cp <= 0xffe6) ||
        (cp >= 0x1f000 && cp <= 0x1faff) ||
        (cp >= 0x20000 && cp <= 0x3fffd)) return 2;
    return 1;
}

static int
cluster_cells(const unsigned char *s, size_t start, size_t end)
{
    int width = 0, ri = 0, ep = 0, vs16 = 0, keycap = 0, zwj = 0;
    while (start < end)
    {
        uint32_t cp;
        int w;
        start = read_scalar(s, start, end, &cp);
        if (is_ri(cp)) ri++;
        if (is_pictographic(cp)) ep = 1;
        if (cp == 0xfe0f) vs16 = 1;
        if (cp == 0x20e3) keycap = 1;
        if (cp == 0x200d) zwj = 1;
        w = scalar_cells(cp);
        if (w > width) width = w;
    }
    if (ri >= 2 || keycap || (vs16 && ep) || (zwj && ep)) return 2;
    return width;
}

int
jrogue_text_width_n(const char *text, size_t max_bytes)
{
    size_t start = 0, len;
    int cells = 0;
    if (!text) return 0;
    len = strlen(text);
    while (start < len)
    {
        size_t end = cluster_end((const unsigned char *)text, start, len);
        if (end > max_bytes) break;
        cells += cluster_cells((const unsigned char *)text, start, end);
        start = end;
    }
    return cells;
}

int
jrogue_text_width(const char *text)
{
    return text ? jrogue_text_width_n(text, strlen(text)) : 0;
}

int
mbstrlen(const char *text)
{
    return jrogue_text_width(text);
}
