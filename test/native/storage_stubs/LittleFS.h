#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>
namespace fakeFs {
inline std::map<std::string, std::vector<uint8_t>> disk;
inline std::set<std::string> dirs;
inline int writesLeft = -1;
inline bool writeAllowed() {
  if (!writesLeft) return false;
  if (writesLeft > 0) --writesLeft;
  return true;
}
}
class File {
public:
  File() = default;
  File(std::string path, bool valid) : path_(std::move(path)), valid_(valid) {}
  explicit operator bool() const { return valid_; }
  size_t size() { return valid_ ? fakeFs::disk[path_].size() : 0; }
  size_t read(uint8_t *out, size_t capacity) {
    if (!valid_) return 0;
    auto &bytes = fakeFs::disk[path_];
    const size_t length = std::min(capacity, bytes.size());
    memcpy(out, bytes.data(), length); return length;
  }
  size_t write(const uint8_t *data, size_t length) {
    if (!valid_ || !fakeFs::writeAllowed()) return 0;
    fakeFs::disk[path_] = std::vector<uint8_t>(data, data + length);
    return length;
  }
  void flush() {}
  void close() { valid_ = false; }
private:
  std::string path_;
  bool valid_ = false;
};
class FakeLittleFS {
public:
  bool begin(bool) { return true; }
  bool exists(const char *path) { return fakeFs::disk.count(path) || fakeFs::dirs.count(path); }
  bool mkdir(const char *path) {
    return fakeFs::writeAllowed() && fakeFs::dirs.insert(path).second;
  }
  File open(const char *path, const char *mode) {
    if (*mode == 'r') return File(path, fakeFs::disk.count(path));
    if (!fakeFs::writeAllowed()) return File();
    fakeFs::disk[path] = {}; return File(path, true);
  }
  bool rename(const char *from, const char *to) {
    if (!fakeFs::disk.count(from) || !fakeFs::writeAllowed()) return false;
    fakeFs::disk[to] = fakeFs::disk[from]; fakeFs::disk.erase(from); return true;
  }
  bool remove(const char *path) {
    if (!fakeFs::disk.count(path) || !fakeFs::writeAllowed()) return false;
    fakeFs::disk.erase(path); return true;
  }
};
inline FakeLittleFS LittleFS;
