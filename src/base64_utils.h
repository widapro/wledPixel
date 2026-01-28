#ifndef BASE64_UTILS_H
#define BASE64_UTILS_H

#include <Arduino.h>

namespace base64_utils {
static const char pgm_b64[] PROGMEM =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline String encode(String in) {
  String out;
  out.reserve(4 * ((in.length() + 2) / 3));
  int val = 0, valb = -6;
  for (unsigned int i = 0; i < in.length(); i++) {
    val = (val << 8) + in[i];
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

inline String decode(String in) {
  String out;
  int T[256];
  for (int i = 0; i < 256; i++)
    T[i] = -1;
  for (int i = 0; i < 64; i++)
    T[pgm_read_byte(&pgm_b64[i])] = i;

  int val = 0, valb = -8;
  for (unsigned int i = 0; i < in.length(); i++) {
    unsigned char c = in[i];
    if (T[c] == -1)
      break;
    val = (val << 6) + T[c];
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
