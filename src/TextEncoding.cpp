#include "TextEncoding.h"

namespace {
bool isContinuation(uint8_t byte) { return (byte & 0xC0) == 0x80; }
} // namespace

void utf8ToFontEncoding(const char *src, char *dst, size_t dstSize,
                        const String &fontName) {
  if (!dst || dstSize == 0)
    return;
  dst[0] = '\0';
  if (!src)
    return;

  size_t si = 0;
  size_t di = 0;
  while (src[si] && di + 1 < dstSize) {
    uint8_t b = static_cast<uint8_t>(src[si]);

    if (b < 0x80) {
      if (fontName == "wledFont_CompactSymbols" && src[si] == '~') {
        si++;
        if (src[si] == '~') {
          dst[di++] = '~';
          si++;
        } else if (src[si] >= 32 && src[si] <= 127) {
          if (src[si] == '@')
            dst[di++] = static_cast<char>(230);
          else if (src[si] == 'P')
            dst[di++] = static_cast<char>(224);
          else
            dst[di++] = static_cast<char>(128 + (src[si] - 32));
          si++;
        } else {
          dst[di++] = '~';
        }
      } else {
        dst[di++] = src[si++];
      }
      continue;
    }

    if ((b & 0xE0) == 0xC0) {
      uint8_t b1 = static_cast<uint8_t>(src[si + 1]);
      if (!isContinuation(b1)) {
        dst[di++] = '?';
        si++;
        continue;
      }

      uint16_t codepoint =
          (static_cast<uint16_t>(b & 0x1F) << 6) | (b1 & 0x3F);
      if (codepoint >= 0x0080 && codepoint <= 0x00FF) {
        dst[di++] = static_cast<char>(codepoint);
      } else if (fontName == "wledFont_cyrillic" && codepoint >= 0x0410 &&
                 codepoint <= 0x044F) {
        if (codepoint == 0x0430)
          dst[di++] = static_cast<char>(194);
        else if (codepoint == 0x0440)
          dst[di++] = static_cast<char>(195);
        else
          dst[di++] = static_cast<char>(128 + (codepoint - 0x0410));
      } else if (fontName == "wledFont_cyrillic" && codepoint == 0x0401) {
        dst[di++] = static_cast<char>(192);
      } else if (fontName == "wledFont_cyrillic" && codepoint == 0x0451) {
        dst[di++] = static_cast<char>(193);
      } else {
        dst[di++] = '?';
      }
      si += 2;
      continue;
    }

    if ((b & 0xF0) == 0xE0) {
      uint8_t b1 = static_cast<uint8_t>(src[si + 1]);
      uint8_t b2 = b1 ? static_cast<uint8_t>(src[si + 2]) : 0;
      dst[di++] = '?';
      si += isContinuation(b1) && isContinuation(b2) ? 3 : 1;
      continue;
    }

    if ((b & 0xF8) == 0xF0) {
      uint8_t b1 = static_cast<uint8_t>(src[si + 1]);
      uint8_t b2 = b1 ? static_cast<uint8_t>(src[si + 2]) : 0;
      uint8_t b3 = b2 ? static_cast<uint8_t>(src[si + 3]) : 0;
      dst[di++] = '?';
      si += isContinuation(b1) && isContinuation(b2) && isContinuation(b3)
                ? 4
                : 1;
      continue;
    }

    // Preserve already encoded single-byte font indices such as 0xA0.
    dst[di++] = src[si++];
  }
  dst[di] = '\0';
}
