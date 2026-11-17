/* Locale-independent UTF-8 text boundaries and terminal cell widths.
 * Copyright (c) 2026 leopardgecko. See LICENSE.TXT.
 */
#ifndef JROGUE_UNICODE_TEXT_H
#define JROGUE_UNICODE_TEXT_H
#include <stddef.h>

/* Positions and returned offsets are UTF-8 byte offsets.  Whole graphemes. */
size_t jrogue_text_next(const char *text, size_t pos);
size_t jrogue_text_prev(const char *text, size_t pos);
size_t jrogue_text_prefix(const char *text, size_t max_bytes);
/* Width is terminal columns, not bytes or code points. */
int jrogue_text_width(const char *text);
int jrogue_text_width_n(const char *text, size_t max_bytes);
int mbstrlen(const char *text);   /* Legacy width interface. */
/* Expected byte count of a UTF-8 sequence (1 for ASCII/invalid lead). */
int jrogue_utf8_sequence_bytes(unsigned char lead);
/* Decode exactly one Unicode scalar, rejecting truncated, malformed,
 * overlong, surrogate and multi-character UTF-8 strings. Returns 1 on
 * success; valid U+FFFD is accepted. No locale dependency. */
int jrogue_utf8_single_scalar(const char *glyph, unsigned int *scalar);
#endif
