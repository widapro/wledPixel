#ifndef BASE64_UTILS_H
#define BASE64_UTILS_H

#include <Arduino.h>
#include <cstring>

namespace base64_utils {
static const char pgm_b64[] PROGMEM =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline String encode(const String &in) {
  String out;
  out.reserve(4 * ((in.length() + 2) / 3));
  uint32_t val = 0;
  int valb = -6;
  for (unsigned int i = 0; i < in.length(); i++) {
    val = (val << 8) + static_cast<uint8_t>(in[i]);
    valb += 8;
    while (valb >= 0) {
      out += (char)pgm_read_byte(&pgm_b64[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out += (char)pgm_read_byte(&pgm_b64[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.length() % 4)
    out += '=';
  return out;
}

inline String decode(const String &in) {
  String out;
  int8_t table[256];
  memset(table, -1, sizeof(table));
  for (int i = 0; i < 64; i++) {
    table[(uint8_t)pgm_read_byte(&pgm_b64[i])] = i;
  }

  uint32_t val = 0;
  int valb = -8;
  for (unsigned int i = 0; i < in.length(); i++) {
    unsigned char c = in[i];
    if (table[c] == -1)
      break;
    val = (val << 6) + table[c];
    valb += 6;
    if (valb >= 0) {
      out += char((val >> valb) & 0xFF);
      valb -= 8;
    }
  }
  return out;
}
} // namespace base64_utils
#endif
