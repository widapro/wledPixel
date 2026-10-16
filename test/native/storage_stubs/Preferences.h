#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
class Preferences {
public:
  inline static std::map<std::string, std::vector<uint8_t>> disk;
  inline static int writesLeft = -1;
  bool begin(const char *name, bool = false) { ns = std::string(name) + '/'; return true; }
  void end() {}
  bool isKey(const char *key) { return disk.count(ns + key) != 0; }
  size_t getBytesLength(const char *key) { return isKey(key) ? disk[ns + key].size() : 0; }
  size_t getBytes(const char *key, void *out, size_t capacity) {
    if (!isKey(key)) return 0;
    auto &bytes = disk[ns + key];
    if (bytes.size() > capacity) return 0;
    memcpy(out, bytes.data(), bytes.size()); return bytes.size();
  }
  size_t putBytes(const char *key, const void *value, size_t size) {
    if (writesLeft == 0) return 0;
    if (writesLeft > 0) --writesLeft;
    auto p = static_cast<const uint8_t *>(value);
    disk[ns + key] = std::vector<uint8_t>(p, p + size); return size;
  }
  uint8_t getUChar(const char *key, uint8_t fallback) {
    uint8_t value; return getBytes(key, &value, 1) == 1 ? value : fallback;
  }
  size_t putUChar(const char *key, uint8_t value) { return putBytes(key, &value, 1); }
  bool remove(const char *key) {
    if (writesLeft == 0) return false;
    if (writesLeft > 0) --writesLeft;
    return disk.erase(ns + key) > 0;
  }
private:
  std::string ns;
};
