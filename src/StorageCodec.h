#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace storageCodec {
constexpr size_t headerSize = 10;
inline bool encoded(const uint8_t *data, size_t size) {
  return size >= headerSize && memcmp(data, "RLE1", 4) == 0;
}
inline uint32_t checksum(const uint8_t *data, size_t size) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; ++i) hash = (hash ^ data[i]) * 16777619u;
  return hash;
}
inline size_t encode(const uint8_t *data, size_t size, uint8_t *out, size_t capacity) {
  if (!data || !out || size > 65535 || capacity < headerSize) return 0;
  memcpy(out, "RLE1", 4);
  out[4] = static_cast<uint8_t>(size); out[5] = static_cast<uint8_t>(size >> 8);
  uint32_t hash = checksum(data, size);
  for (size_t i = 0; i < 4; ++i) out[6 + i] = static_cast<uint8_t>(hash >> (8 * i));
  size_t pos = 0, written = headerSize;
  while (pos < size) {
    size_t run = 1;
    while (run < 128 && pos + run < size && data[pos + run] == data[pos]) ++run;
    if (run >= 3) {
      if (written + 2 > capacity) return 0;
      out[written++] = static_cast<uint8_t>(0x80 | (run - 1));
      out[written++] = data[pos]; pos += run;
    } else {
      const size_t start = pos++;
      while (pos < size && pos - start < 128) {
        if (pos + 2 < size && data[pos] == data[pos + 1] && data[pos] == data[pos + 2]) break;
        ++pos;
      }
      const size_t count = pos - start;
      if (written + count + 1 > capacity) return 0;
      out[written++] = static_cast<uint8_t>(count - 1);
      memcpy(out + written, data + start, count); written += count;
    }
  }
  return written;
}
inline bool decode(const uint8_t *data, size_t size, uint8_t *out, size_t expected) {
  if (!data || !out || !encoded(data, size) ||
      (static_cast<size_t>(data[4]) | (static_cast<size_t>(data[5]) << 8)) != expected) return false;
  uint32_t hash = 0;
  for (size_t i = 0; i < 4; ++i) hash |= static_cast<uint32_t>(data[6 + i]) << (8 * i);
  size_t pos = headerSize, written = 0;
  while (pos < size) {
    const uint8_t token = data[pos++];
    const size_t count = (token & 0x7f) + 1;
    if (count > expected - written) return false;
    if (token & 0x80) {
      if (pos == size) return false;
      memset(out + written, data[pos++], count);
    } else {
      if (count > size - pos) return false;
      memcpy(out + written, data + pos, count); pos += count;
    }
    written += count;
  }
  return written == expected && checksum(out, written) == hash;
}
} // namespace storageCodec
