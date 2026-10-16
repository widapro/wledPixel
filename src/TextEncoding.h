#pragma once

#include <Arduino.h>

// Convert UTF-8 text to the single-byte slots used by the bundled fonts.
// The destination is always NUL-terminated when dst is non-null and
// dstSize is greater than zero.
void utf8ToFontEncoding(const char *src, char *dst, size_t dstSize,
                        const String &fontName);
